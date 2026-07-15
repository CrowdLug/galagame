#include "CombatScene.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSimpleTextItem>
#include <QKeyEvent>
#include <QPen>
#include <QRandomGenerator>
#include <QtGlobal>

#include "BulletItem.h"
#include "EnemyBulletItem.h"
#include "EnemyEntity.h"
#include "PickupItem.h"
#include "PlayerEntity.h"
#include "TrapItem.h"

CombatScene::CombatScene(GameMode gameMode, QObject* parent)
    : QGraphicsScene(parent)
    , m_gameMode(gameMode) {
    setSceneRect(0.0, 0.0, 1920.0, 1080.0);

    QString bgPath = QStringLiteral("assets/combat_bg.png");
    QString appDir = QCoreApplication::applicationDirPath();
    QString resolvedPath = QDir(appDir).filePath(bgPath);
    if (!QFile::exists(resolvedPath)) {
        resolvedPath = QDir(appDir).filePath(QStringLiteral("../") + bgPath);
    }
    if (!QFile::exists(resolvedPath)) {
        resolvedPath = QDir(appDir).filePath(QStringLiteral("../../") + bgPath);
    }

    QPixmap bgPix(resolvedPath);
    if (!bgPix.isNull()) {
        auto* bgItem = addPixmap(bgPix);
        bgItem->setZValue(-1000.0);
        bgItem->setPos(sceneRect().topLeft());
        if (bgPix.size().width() > 1.0 && bgPix.size().height() > 1.0) {
            qreal sx = sceneRect().width() / bgPix.size().width();
            qreal sy = sceneRect().height() / bgPix.size().height();
            bgItem->setScale(qMax(sx, sy));
        }
    } else {
        setBackgroundBrush(QBrush(QColor(41, 23, 29)));
    }

    m_baseSceneTopLeft = sceneRect().topLeft();

    m_hud = addSimpleText(QStringLiteral("血量: 0  敌人: 0  弹药: 0"));
    m_hud->setPos(22.0, 18.0);
    m_hud->setBrush(QBrush(Qt::white));
    QFont hudFont = m_hud->font();
    hudFont.setPointSize(24);
    m_hud->setFont(hudFont);

    connect(&m_tickTimer,
            &QTimer::timeout,
            this,
            &CombatScene::onTick,
            Qt::UniqueConnection);

    connect(&m_loop,
            &CombatLoop::combatLoopFinished,
            this,
            &CombatScene::onCombatFinished,
            Qt::UniqueConnection);

    connect(&m_loop,
            &CombatLoop::combatFeedbackEvent,
            this,
            [this](const QString& text) {
                m_feedbackText = text;
            });

    connect(&m_loop,
            &CombatLoop::feedbackHitStopTriggered,
            this,
            [this]() {
                m_feedbackText = QStringLiteral("Hit stop");
            });

    connect(&m_loop,
            &CombatLoop::feedbackHurtTriggered,
            this,
            [this](double blinkAlpha, const QPointF&) {
                m_flashAlpha = qMax(m_flashAlpha, blinkAlpha);
                m_feedbackText = QStringLiteral("Hurt");
            });

    connect(&m_loop,
            &CombatLoop::feedbackKillTriggered,
            this,
            [this](double, double) {
                m_flashAlpha = qMax(m_flashAlpha, 0.55);
                m_feedbackText = QStringLiteral("Kill");
            });

    m_tickTimer.setInterval(16);

    m_loop.setGameMode(m_gameMode);
    m_loop.setScreenShakeEnabled(m_screenShakeEnabled);
    m_loop.start();

    m_flashOverlay = addRect(sceneRect(), QPen(Qt::NoPen), QBrush(QColor(255, 255, 255, 0)));
    if (m_flashOverlay != nullptr) {
        m_flashOverlay->setZValue(9999.0);
        m_flashOverlay->setVisible(true);
    }

    if (PlayerEntity* player = m_loop.player()) {
        addItem(player);
    }
    for (const auto& enemy : m_loop.enemies()) {
        addItem(enemy.get());
    }
    for (const auto& pickup : m_loop.pickups()) {
        addItem(pickup.get());
    }
    for (const auto& trap : m_loop.traps()) {
        addItem(trap.get());
    }

    m_frameTimer.start();
    m_tickTimer.start();
    updateHud();
}

CombatScene::~CombatScene() {
    m_tickTimer.stop();
}

void CombatScene::setCombatModifiers(double enemyDensityBias,
                                     double dropBias,
                                     double specialEventChance) {
    m_loop.setNarrativeCombatModifier(enemyDensityBias,
                                      dropBias,
                                      specialEventChance);
}

void CombatScene::setTacticalUpgrade(double fireRateMultiplier,
                                     int penetrationBonus,
                                     double damageReductionRatio,
                                     const QString& title,
                                     const QString& description) {
    m_loop.setTacticalUpgrade(fireRateMultiplier,
                              penetrationBonus,
                              damageReductionRatio,
                              title,
                              description);
}

void CombatScene::keyPressEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) {
        QGraphicsScene::keyPressEvent(event);
        return;
    }

    if (event->key() == Qt::Key_W) {
        m_up = true;
    } else if (event->key() == Qt::Key_S) {
        m_down = true;
    } else if (event->key() == Qt::Key_A) {
        m_left = true;
    } else if (event->key() == Qt::Key_D) {
        m_right = true;
    } else if (event->key() == Qt::Key_1) {
        m_loop.switchWeaponByDigit(1);
    } else if (event->key() == Qt::Key_2) {
        m_loop.switchWeaponByDigit(2);
    } else if (event->key() == Qt::Key_3) {
        m_loop.switchWeaponByDigit(3);
    } else if (event->key() == Qt::Key_V) {
        m_screenShakeEnabled = !m_screenShakeEnabled;
        m_loop.setScreenShakeEnabled(m_screenShakeEnabled);
        m_feedbackText = m_screenShakeEnabled
                             ? QStringLiteral("Screen shake: ON")
                             : QStringLiteral("Screen shake: OFF");
    } else if (event->key() == Qt::Key_P) {
        const bool nextPaused = !m_loop.isSpawnerPaused();
        m_loop.setSpawnerPaused(nextPaused);
        m_feedbackText = nextPaused
                             ? QStringLiteral("Spawner: PAUSED")
                             : QStringLiteral("Spawner: RUNNING");
    }

    syncInputToLoop();
    QGraphicsScene::keyPressEvent(event);
}

void CombatScene::keyReleaseEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) {
        QGraphicsScene::keyReleaseEvent(event);
        return;
    }

    if (event->key() == Qt::Key_W) {
        m_up = false;
    } else if (event->key() == Qt::Key_S) {
        m_down = false;
    } else if (event->key() == Qt::Key_A) {
        m_left = false;
    } else if (event->key() == Qt::Key_D) {
        m_right = false;
    }

    syncInputToLoop();
    QGraphicsScene::keyReleaseEvent(event);
}

void CombatScene::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_isFiring = true;
        m_aimTarget = event->scenePos();
        m_loop.shootTowards(m_aimTarget);

        for (const auto& bullet : m_loop.bullets()) {
            if (bullet->scene() == nullptr) {
                addItem(bullet.get());
            }
        }
    }

    QGraphicsScene::mousePressEvent(event);
}

void CombatScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_isFiring = false;
    }

    QGraphicsScene::mouseReleaseEvent(event);
}

void CombatScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    m_aimTarget = event->scenePos();
    QGraphicsScene::mouseMoveEvent(event);
}

void CombatScene::onTick() {
    const double delta = static_cast<double>(m_frameTimer.restart()) / 1000.0;
    m_loop.update(delta);

    if (m_isFiring) {
        m_loop.shootTowards(m_aimTarget);
    }

    m_flashAlpha = qMax(0.0, m_flashAlpha - delta * 4.5);
    if (m_flashOverlay != nullptr) {
        const int alpha = qBound(0, static_cast<int>(m_flashAlpha * 210.0), 210);
        m_flashOverlay->setRect(sceneRect());
        m_flashOverlay->setBrush(QBrush(QColor(255, 255, 255, alpha)));
    }

    if (m_screenShakeEnabled) {
        const double amp = m_flashAlpha > 0.01 ? 6.0 : 0.0;
        const auto randomSigned = [](double limit) {
            const double u = QRandomGenerator::global()->generateDouble();
            return (u * 2.0 - 1.0) * limit;
        };
        m_screenShakeOffset.setX(randomSigned(amp));
        m_screenShakeOffset.setY(randomSigned(amp));
        setSceneRect(m_baseSceneTopLeft.x() + m_screenShakeOffset.x(),
                     m_baseSceneTopLeft.y() + m_screenShakeOffset.y(),
                     1920.0,
                     1080.0);
    } else {
        m_screenShakeOffset = QPointF(0.0, 0.0);
        setSceneRect(m_baseSceneTopLeft.x(), m_baseSceneTopLeft.y(), 1920.0, 1080.0);
    }

    for (const auto& bullet : m_loop.bullets()) {
        if (bullet->scene() == nullptr) {
            addItem(bullet.get());
        }
    }

    for (const auto& enemy : m_loop.enemies()) {
        if (enemy->scene() == nullptr) {
            addItem(enemy.get());
        }
    }

    for (const auto& pickup : m_loop.pickups()) {
        if (pickup->scene() == nullptr) {
            addItem(pickup.get());
        }
    }

    for (const auto& trap : m_loop.traps()) {
        if (trap->scene() == nullptr) {
            addItem(trap.get());
        }
    }

    for (const auto& bullet : m_loop.enemyBullets()) {
        if (bullet->scene() == nullptr) {
            addItem(bullet.get());
        }
    }

    updateHud();
}

void CombatScene::onCombatFinished(bool victory,
                                   double clearTimeSeconds,
                                   int hitsTaken,
                                   double accuracy,
                                   int pickupsCollected,
                                   int iFrameBlocks,
                                   int enemiesKilled,
                                   int totalEnemiesSpawned,
                                   int shotsFired) {
    m_tickTimer.stop();
    emit combatFinished(victory,
                        clearTimeSeconds,
                        hitsTaken,
                        accuracy,
                        pickupsCollected,
                        iFrameBlocks,
                        enemiesKilled,
                        totalEnemiesSpawned,
                        shotsFired);
}

void CombatScene::syncInputToLoop() {
    m_loop.setMovementInput(m_up, m_down, m_left, m_right);
}

void CombatScene::updateHud() {
    if (m_hud == nullptr) {
        return;
    }

    if (m_gameMode == GameMode::Endless) {
        const int elapsed = static_cast<int>(m_loop.elapsedSeconds());
        const int minutes = elapsed / 60;
        const int seconds = elapsed % 60;
        m_hud->setText(QStringLiteral("血量: %1  敌人: %2  弹药: %3  武器: %4  时间: %5:%6")
                           .arg(m_loop.playerHp())
                           .arg(m_loop.enemiesRemaining())
                           .arg(m_loop.currentAmmo())
                           .arg(m_loop.currentWeaponName())
                           .arg(minutes)
                           .arg(seconds, 2, 10, QChar('0')));
    } else {
        const int remaining = static_cast<int>(m_loop.remainingSeconds());
        m_hud->setText(QStringLiteral("血量: %1  敌人: %2  弹药: %3  武器: %4  倒计时: %5秒")
                           .arg(m_loop.playerHp())
                           .arg(m_loop.enemiesRemaining())
                           .arg(m_loop.currentAmmo())
                           .arg(m_loop.currentWeaponName())
                           .arg(remaining));
    }
}
