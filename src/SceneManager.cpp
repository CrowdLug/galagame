#include "SceneManager.h"

#include <QAudioOutput>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QGraphicsPixmapItem>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QLabel>
#include <QMediaPlayer>
#include <QMovie>
#include <QStringList>
#include <QUrl>

#include "TransitionController.h"
#include "CombatScene.h"
#include "RunContext.h"
#include "ResourceManager.h"

namespace {
QString resolveAssetPath(const QString& relativePath) {
    const QString appDir = QCoreApplication::applicationDirPath();

    const QString direct = QDir(appDir).filePath(relativePath);
    if (QFile::exists(direct)) {
        return direct;
    }

    const QString parent = QDir(appDir).filePath(QStringLiteral("../") + relativePath);
    if (QFile::exists(parent)) {
        return parent;
    }

    const QString parent2 = QDir(appDir).filePath(QStringLiteral("../../") + relativePath);
    if (QFile::exists(parent2)) {
        return parent2;
    }

    return QString();
}

QString firstExistingAssetPath(const QStringList& candidates) {
    for (const QString& candidate : candidates) {
        const QString resolved = resolveAssetPath(candidate);
        if (!resolved.isEmpty()) {
            return resolved;
        }
    }
    return QString();
}

void addCoverPixmap(QGraphicsScene* scene, const QPixmap& pixmap) {
    if (scene == nullptr || pixmap.isNull()) {
        return;
    }

    auto* item = scene->addPixmap(pixmap);
    item->setZValue(-1000.0);
    item->setTransformationMode(Qt::SmoothTransformation);

    const QRectF r = scene->sceneRect();
    const QSizeF s = pixmap.size();
    if (s.width() > 1.0 && s.height() > 1.0) {
        const qreal sx = r.width() / s.width();
        const qreal sy = r.height() / s.height();
        item->setScale(qMax(sx, sy));
    }
    item->setPos(r.topLeft());
}

void playSceneAudio(QGraphicsScene* scene, const QString& path, qreal volume, int loops = 1) {
    if (scene == nullptr || path.isEmpty()) {
        return;
    }

    auto* player = new QMediaPlayer(scene);
    auto* output = new QAudioOutput(player);
    output->setVolume(volume);
    player->setAudioOutput(output);
    player->setSource(QUrl::fromLocalFile(path));
    player->setLoops(loops);
    player->play();
}

void addDefeatBackground(QGraphicsScene* scene) {
    const QString path = resolveAssetPath(QStringLiteral("assets/ui/taffy_defeat_bg.png"));
    if (!path.isEmpty()) {
        addCoverPixmap(scene, QPixmap(path));
    }
}
} // namespace

SceneManager::SceneManager(QObject* parent)
    : QObject(parent)
    , m_transitionController(new TransitionController(this)) {
    qInfo() << "[SceneManager] Constructed";
}

void SceneManager::setRunContext(RunContext* runContext) {
    m_runContext = runContext;
}

void SceneManager::setResourceManager(ResourceManager* resourceManager) {
    m_resourceManager = resourceManager;
}

void SceneManager::setGameMode(GameMode mode) {
    m_gameMode = mode;
}

GameMode SceneManager::gameMode() const {
    return m_gameMode;
}

SceneManager::~SceneManager() {
    qInfo() << "[SceneManager] Destroyed"
            << "createdCount=" << m_createdCount
            << "destroyedCount=" << m_destroyedCount;
}

void SceneManager::setFailNextBootstrap(bool shouldFail) {
    m_failNextBootstrap = shouldFail;
    qInfo() << "[SceneManager] setFailNextBootstrap" << "shouldFail=" << shouldFail;
}

void SceneManager::onCombatResolved(bool victory,
                                     double clearTimeSeconds,
                                     int hitsTaken,
                                     double accuracy,
                                     int pickupsCollected,
                                     int iFrameBlocks,
                                     int enemiesKilled,
                                     int totalEnemiesSpawned,
                                     int shotsFired) {
    Q_UNUSED(hitsTaken);
    Q_UNUSED(accuracy);
    Q_UNUSED(pickupsCollected);
    Q_UNUSED(iFrameBlocks);

    m_combatVictory = victory;
    m_combatClearTime = clearTimeSeconds;
    m_combatEnemiesKilled = enemiesKilled;
    m_combatTotalEnemies = totalEnemiesSpawned;
    m_combatShotsFired = shotsFired;

    emit combatResolvedSignal(victory, clearTimeSeconds, hitsTaken, accuracy,
                              pickupsCollected, iFrameBlocks, enemiesKilled,
                              totalEnemiesSpawned, shotsFired);
}

void SceneManager::bootstrapScene() {
    if (m_failNextBootstrap) {
        m_failNextBootstrap = false;
        qWarning() << "[SceneManager] Forced bootstrap failure for debug";
        emit sceneBootstrapped(nullptr);
        return;
    }

    switchScene(QStringLiteral("Menu"), QStringLiteral("InitialBootstrap"));
    emit sceneBootstrapped(m_scene.data());
}

void SceneManager::switchScene(const QString& sceneKey, const QString& reason) {
    QElapsedTimer timer;
    timer.start();

    const QString fromKey = (m_scene == nullptr) ? QStringLiteral("None") : m_scene->property("sceneKey").toString();

    if (m_transitionController != nullptr) {
        m_transitionController->beginTransition(fromKey, sceneKey, reason);
    }

    if (m_resourceManager != nullptr) {
        m_resourceManager->preloadForSceneKey(sceneKey);
    }

    scheduleCurrentSceneDestroy();

    if (m_resourceManager != nullptr && fromKey != sceneKey) {
        m_resourceManager->releaseForSceneKey(fromKey);
    }

    QGraphicsScene* newScene = createSceneForKey(sceneKey);
    if (newScene == nullptr) {
        qWarning() << "[SceneManager] switchScene failed: unknown scene key"
                   << "sceneKey=" << sceneKey
                   << "reason=" << reason;
        if (m_transitionController != nullptr) {
            m_transitionController->endTransition();
        }
        emit sceneSwitched(nullptr, sceneKey);
        return;
    }

    m_scene = newScene;

    if (m_transitionController != nullptr) {
        m_transitionController->endTransition();
    }

    qInfo() << "[SceneManager] Scene switched"
            << "from=" << fromKey
            << "to=" << sceneKey
            << "reason=" << reason
            << "createdCount=" << m_createdCount
            << "destroyedCount=" << m_destroyedCount
            << "elapsedMs=" << timer.elapsed();

    emit sceneSwitched(m_scene.data(), sceneKey);
}

QGraphicsScene* SceneManager::createSceneForKey(const QString& sceneKey) {
    auto* scene = new QGraphicsScene(this);
    scene->setSceneRect(0.0, 0.0, 1920.0, 1080.0);
    scene->setProperty("sceneKey", sceneKey);

    QColor bg = QColor(24, 26, 34);
    QString text = QStringLiteral("Unknown Scene");

    if (sceneKey == QStringLiteral("Menu")) {
        bg = QColor(24, 26, 34);
        text = QStringLiteral("Menu Scene");
    } else if (sceneKey == QStringLiteral("Combat")) {
        scene->deleteLater();
        auto* combatScene = new CombatScene(m_gameMode, this);
        if (m_runContext != nullptr) {
            const auto& mods = m_runContext->combatModifiers();
            combatScene->setCombatModifiers(mods.enemyWeightBias,
                                            mods.dropBias,
                                            mods.specialEventChance);

            const auto& upg = m_runContext->activeUpgrade();
            combatScene->setTacticalUpgrade(upg.fireRateMultiplier,
                                            upg.penetrationBonus,
                                            upg.damageReductionRatio,
                                            upg.title,
                                            upg.description);
        }
        combatScene->setProperty("sceneKey", sceneKey);
        connect(combatScene,
                &CombatScene::combatFinished,
                this,
                &SceneManager::onCombatResolved,
                Qt::UniqueConnection);
        ++m_createdCount;
        return combatScene;
    } else if (sceneKey == QStringLiteral("Result")) {
        bg = QColor(28, 42, 35);
        text = QStringLiteral("Result Scene");
    } else {
        scene->deleteLater();
        return nullptr;
    }

    scene->setBackgroundBrush(QBrush(bg));

    if (sceneKey == QStringLiteral("Menu") && m_resourceManager != nullptr) {
        const QPixmap bgPix = m_resourceManager->loadTexture(QStringLiteral("assets/ui/menu_bg.png"));
        if (!bgPix.isNull()) {
            addCoverPixmap(scene, bgPix);
        }

        const QString menuAudioPath = firstExistingAssetPath({
            QStringLiteral("assets/bgm/taffy_menu.mp3"),
            QStringLiteral("assets/bgm/taffy_menu.m4a"),
            QStringLiteral("assets/bgm/taffy_menu.ogg"),
            QStringLiteral("assets/bgm/taffy_menu.wav"),
            QStringLiteral("assets/bgm/menu.ogg"),
        });
        playSceneAudio(scene, menuAudioPath, 0.30, QMediaPlayer::Infinite);
        
        auto* titleText = scene->addText(QStringLiteral("拯救塔菲喵！！！！"));
        QFont titleFont = titleText->font();
        titleFont.setPointSize(72);
        titleFont.setBold(true);
        titleText->setFont(titleFont);
        titleText->setDefaultTextColor(QColor(0xF9, 0xD8, 0x9C));
        titleText->setPos(scene->sceneRect().width()/2 - titleText->boundingRect().width()/2, 200);
        titleText->setZValue(100);
        
        auto* startText = scene->addText(QStringLiteral("开始游戏"));
        QFont startFont = startText->font();
        startFont.setPointSize(56);
        startFont.setBold(true);
        startText->setFont(startFont);
        startText->setDefaultTextColor(QColor(0xE8, 0xE4, 0xF0));
        startText->setPos(scene->sceneRect().width()/2 - startText->boundingRect().width()/2, 550);
        startText->setZValue(100);
        startText->setFlag(QGraphicsItem::ItemIsSelectable, true);
        startText->setFlag(QGraphicsItem::ItemIsFocusable, true);
        startText->setData(0, QStringLiteral("StartGame"));
        
        auto* endlessText = scene->addText(QStringLiteral("无尽模式"));
        QFont endlessFont = endlessText->font();
        endlessFont.setPointSize(48);
        endlessFont.setBold(true);
        endlessText->setFont(endlessFont);
        endlessText->setDefaultTextColor(QColor(0x00, 0xFF, 0x88));
        endlessText->setPos(scene->sceneRect().width()/2 - endlessText->boundingRect().width()/2, 650);
        endlessText->setZValue(100);
        endlessText->setFlag(QGraphicsItem::ItemIsSelectable, true);
        endlessText->setFlag(QGraphicsItem::ItemIsFocusable, true);
        endlessText->setData(0, QStringLiteral("EndlessMode"));
        
        auto* quitText = scene->addText(QStringLiteral("退出游戏"));
        QFont quitFont = quitText->font();
        quitFont.setPointSize(56);
        quitFont.setBold(true);
        quitText->setFont(quitFont);
        quitText->setDefaultTextColor(QColor(0xE8, 0xE4, 0xF0));
        quitText->setPos(scene->sceneRect().width()/2 - quitText->boundingRect().width()/2, 750);
        quitText->setZValue(100);
        quitText->setFlag(QGraphicsItem::ItemIsSelectable, true);
        quitText->setFlag(QGraphicsItem::ItemIsFocusable, true);
        quitText->setData(0, QStringLiteral("QuitGame"));
    }

    if (sceneKey == QStringLiteral("Result")) {
        scene->setBackgroundBrush(QBrush(QColor(28, 42, 35)));

        const double sceneWidth = scene->sceneRect().width();

        if (m_gameMode == GameMode::Endless) {
            addDefeatBackground(scene);

            const QString defeatAudioPath = firstExistingAssetPath({
                QStringLiteral("assets/bgm/taffy_defeat.mp3"),
                QStringLiteral("assets/bgm/taffy_defeat.m4a"),
                QStringLiteral("assets/bgm/taffy_defeat.ogg"),
                QStringLiteral("assets/bgm/taffy_defeat.wav"),
            });
            playSceneAudio(scene, defeatAudioPath, 0.45);

            auto* titleText = scene->addText(QStringLiteral("无尽模式 - 游戏结束"));
            QFont titleFont = titleText->font();
            titleFont.setPointSize(48);
            titleFont.setBold(true);
            titleText->setFont(titleFont);
            titleText->setDefaultTextColor(QColor(0x00, 0xFF, 0x88));
            titleText->setPos(180, 180);
            titleText->setZValue(100);

            const QString killCountText = QStringLiteral("击杀敌人: %1 只").arg(m_combatEnemiesKilled);
            auto* killStats = scene->addText(killCountText);
            QFont statsFont = killStats->font();
            statsFont.setPointSize(36);
            killStats->setFont(statsFont);
            killStats->setDefaultTextColor(QColor(185, 232, 198));
            killStats->setPos(200, 330);
            killStats->setZValue(100);

            const int minutes = static_cast<int>(m_combatClearTime) / 60;
            const int seconds = static_cast<int>(m_combatClearTime) % 60;
            const QString timeText = QStringLiteral("存活时间: %1分%2秒")
                                         .arg(minutes)
                                         .arg(seconds, 2, 10, QChar('0'));
            auto* timeStats = scene->addText(timeText);
            timeStats->setFont(statsFont);
            timeStats->setDefaultTextColor(QColor(185, 232, 198));
            timeStats->setPos(200, 420);
            timeStats->setZValue(100);

            auto* defeatText = scene->addText(QStringLiteral("你的塔菲已经被孫笑川带走了。"));
            QFont defeatFont = defeatText->font();
            defeatFont.setPointSize(34);
            defeatFont.setBold(true);
            defeatText->setFont(defeatFont);
            defeatText->setDefaultTextColor(QColor(255, 120, 120));
            defeatText->setPos(180, 540);
            defeatText->setZValue(100);
        } else {
            double killRate = 0.0;
            if (m_combatTotalEnemies > 0) {
                killRate = static_cast<double>(m_combatEnemiesKilled) / m_combatTotalEnemies;
            }

            QString rankText;
            QString rankColorHex;
            QString descriptionText;
            bool isExVictory = false;

            if (m_combatVictory || killRate >= 1.0) {
                rankText = QStringLiteral("EX");
                rankColorHex = QStringLiteral("#FFD700");
                descriptionText = QStringLiteral("你成功拯救塔菲喵，她可以留在地球陪你了！！");
                isExVictory = true;
            } else if (killRate >= 0.6) {
                rankText = QStringLiteral("A");
                rankColorHex = QStringLiteral("#FFFFFF");
                descriptionText = QStringLiteral("你已经尽全力了，但还是没能救下塔菲喵。\n你的塔菲已经被孫笑川带走了。");
            } else {
                rankText = QStringLiteral("B");
                rankColorHex = QStringLiteral("#FF4444");
                descriptionText = QStringLiteral("你什么都做不到！！！\n你的塔菲已经被孫笑川带走了。");
            }

            if (!isExVictory) {
                addDefeatBackground(scene);
            }

            QColor rankColor;
            rankColor.setNamedColor(rankColorHex);

            auto* rank = scene->addText(rankText);
            QFont rankFont = rank->font();
            rankFont.setPointSize(120);
            rankFont.setBold(true);
            rank->setFont(rankFont);
            rank->setDefaultTextColor(rankColor);
            rank->setPos(260, 140);
            rank->setZValue(100);

            auto* desc = scene->addText(descriptionText);
            QFont descFont = desc->font();
            descFont.setPointSize(32);
            descFont.setBold(true);
            desc->setFont(descFont);
            desc->setDefaultTextColor(QColor(232, 228, 240));
            desc->setPos(180, 315);
            desc->setZValue(100);

            const int minutes = static_cast<int>(m_combatClearTime) / 60;
            const int seconds = static_cast<int>(m_combatClearTime) % 60;
            const QString statsText = QStringLiteral("战斗用时: %1分%2秒\n击杀数: %3 / %4\n使用子弹: %5")
                                          .arg(minutes)
                                          .arg(seconds, 2, 10, QChar('0'))
                                          .arg(m_combatEnemiesKilled)
                                          .arg(m_combatTotalEnemies)
                                          .arg(m_combatShotsFired);

            auto* stats = scene->addText(statsText);
            QFont statsFont = stats->font();
            statsFont.setPointSize(28);
            stats->setFont(statsFont);
            stats->setDefaultTextColor(QColor(185, 232, 198));
            stats->setPos(200, 480);
            stats->setZValue(100);

            if (isExVictory) {
                const QString gifPath = resolveAssetPath(QStringLiteral("assets/ui/taffy_tangxiao.gif"));
                if (!gifPath.isEmpty()) {
                    auto* label = new QLabel();
                    label->setAttribute(Qt::WA_TranslucentBackground, true);
                    label->setStyleSheet(QStringLiteral("background: transparent;"));
                    label->setFixedSize(520, 520);

                    auto* movie = new QMovie(gifPath, QByteArray(), label);
                    movie->setScaledSize(QSize(520, 520));
                    label->setMovie(movie);

                    auto* proxy = scene->addWidget(label);
                    proxy->setPos(1180, 210);
                    proxy->setZValue(80);
                    movie->start();
                }

                const QString victoryAudioPath = firstExistingAssetPath({
                    QStringLiteral("assets/bgm/taffy_victory_laugh.m4a"),
                    QStringLiteral("assets/bgm/taffy_victory_laugh.mp3"),
                    QStringLiteral("assets/bgm/taffy_victory_laugh.ogg"),
                    QStringLiteral("assets/bgm/taffy_victory_laugh.wav"),
                });
                playSceneAudio(scene, victoryAudioPath, 0.45);
            } else {
                const QString defeatAudioPath = firstExistingAssetPath({
                    QStringLiteral("assets/bgm/taffy_defeat.mp3"),
                    QStringLiteral("assets/bgm/taffy_defeat.m4a"),
                    QStringLiteral("assets/bgm/taffy_defeat.ogg"),
                    QStringLiteral("assets/bgm/taffy_defeat.wav"),
                });
                playSceneAudio(scene, defeatAudioPath, 0.45);
            }
        }

        auto* menuBtn = scene->addText(QStringLiteral("返回主菜单"));
        QFont menuFont = menuBtn->font();
        menuFont.setPointSize(36);
        menuFont.setBold(true);
        menuBtn->setFont(menuFont);
        menuBtn->setDefaultTextColor(QColor(232, 228, 240));
        menuBtn->setPos(sceneWidth / 2 - menuBtn->boundingRect().width() / 2, 700);
        menuBtn->setZValue(100);
        menuBtn->setFlag(QGraphicsItem::ItemIsSelectable, true);
        menuBtn->setFlag(QGraphicsItem::ItemIsFocusable, true);
        menuBtn->setData(0, QStringLiteral("ReturnToMenu"));
    }

    ++m_createdCount;

    return scene;
}

void SceneManager::scheduleCurrentSceneDestroy() {
    if (m_scene == nullptr) {
        return;
    }

    QPointer<QGraphicsScene> oldScene = m_scene;
    m_scene.clear();

    if (oldScene == nullptr) {
        return;
    }

    ++m_destroyedCount;

    qInfo() << "[SceneManager] schedule deleteLater for old scene"
            << "sceneKey=" << oldScene->property("sceneKey").toString()
            << "destroyedCount=" << m_destroyedCount;

    oldScene->deleteLater();
}
