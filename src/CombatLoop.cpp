#include "CombatLoop.h"

#include <QAudioOutput>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMediaPlayer>
#include <QRandomGenerator>
#include <QStringList>
#include <QtMath>
#include <QUrl>

#include <algorithm>
#include <cmath>

#include <QGraphicsScene>

#include "BulletItem.h"
#include "CollisionManager.h"
#include "EnemyEntity.h"
#include "EnemyBulletItem.h"
#include "FeedbackSystem.h"
#include "EnemyDirector.h"
#include "LightDirector.h"
#include "PickupItem.h"
#include "PlayerEntity.h"
#include "TrapItem.h"
#include "WeaponSystem.h"

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
} // namespace

CombatLoop::CombatLoop(QObject* parent)
    : QObject(parent)
    , m_collisionManager(std::make_unique<CollisionManager>())
    , m_feedbackSystem(std::make_unique<FeedbackSystem>())
    , m_bgmPlayer(std::make_unique<QMediaPlayer>()) {
    qInfo() << "[CombatLoop] Constructed";
}

CombatLoop::~CombatLoop() {
    if (m_bgmPlayer != nullptr) {
        m_bgmPlayer->stop();
    }
    qInfo() << "[CombatLoop] Destroyed"
            << "started=" << m_started
            << "ended=" << m_ended;
}

void CombatLoop::setGameMode(GameMode mode) {
    m_gameMode = mode;
}

GameMode CombatLoop::gameMode() const {
    return m_gameMode;
}

void CombatLoop::start() {
    if (m_started) {
        qWarning() << "[CombatLoop] start() ignored because loop is already started";
        return;
    }

    m_started = true;
    m_ended = false;
    m_victory = false;
    m_elapsedSeconds = 0.0;
    m_hitsTaken = 0;
    m_shotsFired = 0;
    m_shotsHit = 0;
    m_pickupsCollected = 0;
    m_iFrameBlocks = 0;
    m_enemiesKilled = 0;
    m_ammo = m_gameMode == GameMode::Endless ? 99999 : 150;
    m_dropCounter = 0;
    m_currentWeaponIndex = 0;
    m_playerDamageCooldownRemaining = 0.0;
    m_upgradeFireRateMultiplier = 1.0;
    m_upgradePenetrationBonus = 0;
    m_upgradeDamageReductionRatio = 0.0;
    m_upgradeTitle.clear();
    m_upgradeDescription.clear();
    m_trapSpawnTimer = 0.0;
    m_currentSpawnEventIndex = 0;
    m_currentWave = 0;
    m_pickupHealAmount = 10;
    m_lastSpawnTime = 0.0;

    m_weapons.clear();

    const QString resolvedPath = firstExistingAssetPath({
        QStringLiteral("assets/bgm/taffy_battle.mp3"),
        QStringLiteral("assets/bgm/taffy_battle.m4a"),
        QStringLiteral("assets/bgm/taffy_battle.ogg"),
        QStringLiteral("assets/bgm/taffy_battle.wav"),
        QStringLiteral("assets/bgm/taffy_yun_genshin.m4a"),
        QStringLiteral("assets/bgm/taffy_yun_genshin.mp3"),
        QStringLiteral("assets/bgm/taffy_yun_genshin.ogg"),
        QStringLiteral("assets/bgm/taffy_yun_genshin.wav"),
        QStringLiteral("assets/bgm/combat.ogg"),
    });

    if (!resolvedPath.isEmpty()) {
        auto audioOutput = new QAudioOutput(this);
        audioOutput->setVolume(0.35);
        m_bgmPlayer->setAudioOutput(audioOutput);
        m_bgmPlayer->setSource(QUrl::fromLocalFile(resolvedPath));
        m_bgmPlayer->setLoops(QMediaPlayer::Infinite);
        m_bgmPlayer->play();
        qInfo() << "[CombatLoop] BGM started:" << resolvedPath;
    } else {
        qWarning() << "[CombatLoop] BGM file not found";
    }

    const WeaponConfig rifleFallback {QStringLiteral("Rifle"), 8.8, 16, 920.0, 3.0, 0.08, 1, 1};
    const WeaponConfig pistolFallback {QStringLiteral("Pistol"), 4.2, 24, 860.0, 0.8, 0.04, 1, 1};
    const WeaponConfig shotgunFallback {QStringLiteral("Shotgun"), 1.35, 12, 760.0, 17.0, 0.30, 3, 7};
    const WeaponConfig rifleCfg = WeaponConfigLoader::loadFromJson(QStringLiteral("data/weapons/rifle.json"), rifleFallback);
    const WeaponConfig pistolCfg = WeaponConfigLoader::loadFromJson(QStringLiteral("data/weapons/pistol.json"), pistolFallback);
    const WeaponConfig shotgunCfg = WeaponConfigLoader::loadFromJson(QStringLiteral("data/weapons/shotgun.json"), shotgunFallback);

    m_weapons.push_back(std::make_unique<RifleWeapon>(rifleCfg));
    m_weapons.push_back(std::make_unique<PistolWeapon>(pistolCfg));
    m_weapons.push_back(std::make_unique<ShotgunWeapon>(shotgunCfg));

    m_player = std::make_unique<PlayerEntity>(m_worldBounds);
    m_playerMaxHp = m_player->hp();

    const EnemyEntity::AIConfig meleeFallback {2500.0, 170.0, 54.0, 0.0, 0.95, 10, 30, 0.70};
    const EnemyEntity::AIConfig rangedFallback {2500.0, 108.0, 1500.0, 320.0, 1.25, 7, 26, 0.65};
    m_meleeConfig = EnemyEntity::loadConfigFromJson(QStringLiteral("data/enemies/melee.json"), meleeFallback);
    m_rangedConfig = EnemyEntity::loadConfigFromJson(QStringLiteral("data/enemies/ranged.json"), rangedFallback);

    if (m_gameMode == GameMode::Normal) {
        EnemyDirector::Config stageFallback;
        stageFallback.maxOnScreenEnemies = 30;
        stageFallback.events = {
            EnemyDirector::SpawnEvent {0.7, false, QRectF(), 9, 0, false},
            EnemyDirector::SpawnEvent {10.0, false, QRectF(), 9, 3, false},
            EnemyDirector::SpawnEvent {20.0, false, QRectF(), 6, 6, false},
            EnemyDirector::SpawnEvent {40.0, false, QRectF(), 6, 6, false},
        };

        const EnemyDirector::Config stageCfg = EnemyDirector::loadConfigFromJson(QStringLiteral("data/stages/stage01.json"), stageFallback);
        m_enemyDirector = std::make_unique<EnemyDirector>(stageCfg);
        m_enemyDirector->setPaused(false);
        m_enemyDirector->reset();
    } else {
        m_enemyDirector = nullptr;
    }

    m_lightDirector = std::make_unique<LightDirector>();
    m_lightDirector->reset();
    m_directorDebugText = QStringLiteral("Director: Stable");

    spawnWave();

    emit combatFeedbackEvent(QStringLiteral("Weapon: %1").arg(currentWeaponName()));
    emit combatLoopStarted();
}

void CombatLoop::reset() {
    m_started = false;
    m_ended = false;
    m_victory = false;

    m_player.reset();
    m_enemies.clear();
    m_bullets.clear();
    m_pickups.clear();
    m_traps.clear();
    m_weapons.clear();

    m_hitsTaken = 0;
    m_shotsFired = 0;
    m_shotsHit = 0;
    m_pickupsCollected = 0;
    m_iFrameBlocks = 0;
    m_ammo = 150;
    m_currentWeaponIndex = 0;
    m_elapsedSeconds = 0.0;
    m_playerDamageCooldownRemaining = 0.0;
    m_enemiesKilled = 0;
    m_playerMaxHp = 100;
    m_dropCounter = 0;
    m_modifierEnemyDensityBias = 1.0;
    m_modifierDropBias = 1.0;
    m_modifierSpecialEventChance = 0.10;
    m_upgradeFireRateMultiplier = 1.0;
    m_upgradePenetrationBonus = 0;
    m_upgradeDamageReductionRatio = 0.0;
    m_upgradeTitle.clear();
    m_upgradeDescription.clear();
    m_lightDirector.reset();
    m_directorDebugText.clear();
    m_trapSpawnTimer = 0.0;
    m_currentSpawnEventIndex = 0;
}

void CombatLoop::update(double deltaTimeSeconds) {
    if (!m_started || m_ended || m_player == nullptr) {
        return;
    }

    const double clampedDelta = qBound(0.0, deltaTimeSeconds, 0.05);
    FeedbackSystem::FrameEffects fx;
    if (m_feedbackSystem != nullptr) {
        fx = m_feedbackSystem->update(clampedDelta);
    }

    const double simulationDelta = clampedDelta * qBound(0.05, fx.timeScale, 1.0);

    m_elapsedSeconds += simulationDelta;
    m_playerDamageCooldownRemaining = qMax(0.0, m_playerDamageCooldownRemaining - simulationDelta);

    if (m_player != nullptr) {
        m_player->setOpacity(qBound(0.25, fx.playerBlinkAlpha, 1.0));
    }

    m_player->update(simulationDelta);
    updateSpawning();
    updateTraps(simulationDelta);
    updateEnemies(simulationDelta);
    updateBullets(simulationDelta);
    resolveCollisions(simulationDelta);
    cleanupDeadObjects();
    cleanupTraps();

    if (m_player->hp() <= 0) {
        finishCombat(false);
        return;
    }

    if (m_gameMode == GameMode::Normal) {
        if (m_elapsedSeconds >= m_timeLimit) {
            finishCombat(false);
            return;
        }

        if (m_enemies.empty() && (m_enemyDirector == nullptr || !m_enemyDirector->hasPendingEvents())) {
            finishCombat(true);
            return;
        }
    } else {
        updateEndlessSpawning(simulationDelta);
    }
}

void CombatLoop::updateSpawning() {
    if (m_enemyDirector == nullptr || m_player == nullptr) {
        return;
    }

    if (m_lightDirector != nullptr) {
        LightDirector::Metrics metrics;
        metrics.elapsedSeconds = m_elapsedSeconds;
        metrics.playerHp = m_player->hp();
        metrics.maxPlayerHp = qMax(1, m_playerMaxHp);
        metrics.hitsTakenTotal = m_hitsTaken;
        metrics.killsTotal = m_enemiesKilled;

        const LightDirector::Decision decision = m_lightDirector->update(metrics);
        EnemyDirector::RuntimeTuning tuning;
        tuning.spawnIntervalScale = decision.spawnIntervalScale;
        tuning.densityScale = qBound(0.80,
                                     decision.densityScale * m_modifierEnemyDensityBias,
                                     1.25);
        tuning.allowElite = decision.allowElite || (m_modifierSpecialEventChance > 0.16);
        m_enemyDirector->setRuntimeTuning(tuning);
        m_directorDebugText = QStringLiteral("%1 | Mod[Densx%2 Dropx%3]")
                                  .arg(m_lightDirector->debugText())
                                  .arg(m_modifierEnemyDensityBias, 0, 'f', 2)
                                  .arg(m_modifierDropBias, 0, 'f', 2);
    }

    bool hasActiveWave = false;
    if (m_enemyDirector != nullptr) {
        const EnemyDirector::SpawnRequest req = m_enemyDirector->update(
            m_elapsedSeconds,
            m_player->sceneBoundingRect().center(),
            static_cast<int>(m_enemies.size()));

        if (req.meleeCount > 0 || req.rangedCount > 0) {
            spawnEnemies(req);
            hasActiveWave = true;
        }
    }

    if (!hasActiveWave && m_enemies.empty() && m_enemyDirector != nullptr && m_enemyDirector->hasPendingEvents()) {
        forceSpawnNextWave();
    }
}

void CombatLoop::forceSpawnNextWave() {
    if (m_enemyDirector == nullptr) {
        return;
    }

    EnemyDirector::Config& config = const_cast<EnemyDirector::Config&>(m_enemyDirector->getConfig());
    for (EnemyDirector::SpawnEvent& e : config.events) {
        if (!e.triggered) {
            e.triggered = true;
            
            int wantMelee = qMax(0, static_cast<int>(qRound(static_cast<double>(e.meleeCount) * m_modifierEnemyDensityBias)));
            int wantRanged = qMax(0, static_cast<int>(qRound(static_cast<double>(e.rangedCount) * m_modifierEnemyDensityBias)));
            if (wantMelee <= 0 && e.meleeCount > 0) wantMelee = e.meleeCount;
            if (wantRanged <= 0 && e.rangedCount > 0) wantRanged = e.rangedCount;

            EnemyDirector::SpawnRequest req;
            req.meleeCount = wantMelee;
            req.rangedCount = wantRanged;

            if (req.meleeCount > 0 || req.rangedCount > 0) {
                spawnEnemies(req);
                qInfo() << "[CombatLoop] Forced spawn next wave"
                        << "melee=" << req.meleeCount
                        << "ranged=" << req.rangedCount;
            }
            break;
        }
    }
}

void CombatLoop::spawnEnemies(const EnemyDirector::SpawnRequest& req) {
    const double minY = m_worldBounds.top() + 40.0;
    const double maxY = m_worldBounds.bottom() - 40.0;
    const double spanY = qMax(1.0, maxY - minY);

    auto spawnEnemy = [&](EnemyEntity::Archetype type, int index) {
        const double t = std::fmod((m_elapsedSeconds * 97.0) + (index * 53.0), spanY);
        const double y = minY + t;

        bool spawnOnLeft = (index % 2 == 0);
        double x;
        if (spawnOnLeft) {
            x = m_worldBounds.left() + 80.0 + (index % 3) * 36.0;
        } else {
            x = m_worldBounds.right() - 80.0 - (index % 3) * 36.0;
        }

        const EnemyEntity::AIConfig& cfg = (type == EnemyEntity::Archetype::Melee)
            ? m_meleeConfig
            : m_rangedConfig;

        m_enemies.push_back(std::make_unique<EnemyEntity>(QPointF(x, y), type, cfg));
        ++m_totalEnemiesSpawned;
    };

    int spawnIndex = 0;
    for (int i = 0; i < req.meleeCount; ++i) {
        spawnEnemy(EnemyEntity::Archetype::Melee, spawnIndex++);
    }
    for (int i = 0; i < req.rangedCount; ++i) {
        spawnEnemy(EnemyEntity::Archetype::Ranged, spawnIndex++);
    }

    emit combatFeedbackEvent(QStringLiteral("Spawned: %1 melee, %2 ranged")
                                 .arg(req.meleeCount)
                                 .arg(req.rangedCount));
}

void CombatLoop::updateEndlessSpawning(double deltaTimeSeconds) {
    Q_UNUSED(deltaTimeSeconds);
    
    const bool timeToSpawn = (m_elapsedSeconds - m_lastSpawnTime) >= kEndlessSpawnInterval;
    const bool waveClear = m_enemies.empty();
    
    if (timeToSpawn || waveClear) {
        spawnEndlessWave();
    }
}

void CombatLoop::spawnEndlessWave() {
    ++m_currentWave;
    
    int meleeCount = 2;
    int rangedCount = 0;
    
    const int waveMultiplier = 1 << ((m_currentWave - 1) / 10);
    meleeCount *= waveMultiplier;
    
    if (m_currentWave >= 10) {
        rangedCount = 1 * waveMultiplier;
    }
    
    m_pickupHealAmount = 10 + (m_currentWave / 10);
    
    EnemyDirector::SpawnRequest req;
    req.meleeCount = meleeCount;
    req.rangedCount = rangedCount;
    
    spawnEnemies(req);
    m_lastSpawnTime = m_elapsedSeconds;
    
    emit combatFeedbackEvent(QStringLiteral("Wave %1: %2 melee, %3 ranged")
                                 .arg(m_currentWave)
                                 .arg(meleeCount)
                                 .arg(rangedCount));
}

void CombatLoop::updateTraps(double deltaTimeSeconds) {
    m_trapSpawnTimer += deltaTimeSeconds;

    if (m_trapSpawnTimer >= kTrapSpawnInterval) {
        m_trapSpawnTimer = 0.0;
        spawnTraps();
    }

    for (auto& trap : m_traps) {
        if (trap != nullptr) {
            trap->update(deltaTimeSeconds);
        }
    }
}

void CombatLoop::spawnTraps() {
    if (m_player == nullptr) {
        return;
    }

    const QPointF playerPos = m_player->sceneBoundingRect().center();
    const double minDist = 200.0;
    const double maxDist = 700.0;

    for (int i = 0; i < 4; ++i) {
        const double angle = QRandomGenerator::global()->generateDouble() * 2.0 * M_PI;
        const double dist = minDist + QRandomGenerator::global()->generateDouble() * (maxDist - minDist);
        const double x = playerPos.x() + dist * std::cos(angle);
        const double y = playerPos.y() + dist * std::sin(angle);

        const double clampedX = qBound(50.0, x, m_worldBounds.width() - 50.0);
        const double clampedY = qBound(50.0, y, m_worldBounds.height() - 50.0);

        m_traps.push_back(std::make_unique<TrapItem>(QPointF(clampedX, clampedY), 35.0));
    }

    emit combatFeedbackEvent(QStringLiteral("Traps spawned: 4"));
}

void CombatLoop::cleanupTraps() {
    auto it = std::remove_if(m_traps.begin(), m_traps.end(), [](const std::unique_ptr<TrapItem>& trap) {
        return trap == nullptr || trap->isExpired();
    });
    m_traps.erase(it, m_traps.end());
}

void CombatLoop::setMovementInput(bool up, bool down, bool left, bool right) {
    if (m_player == nullptr) {
        return;
    }

    m_player->setMovementInput(up, down, left, right);
}

void CombatLoop::shootTowards(const QPointF& worldTarget) {
    if (!m_started || m_ended || m_player == nullptr) {
        return;
    }

    if (m_weapons.empty()) {
        emit combatFeedbackEvent(QStringLiteral("No weapon equipped"));
        return;
    }

    if (m_currentWeaponIndex < 0 || m_currentWeaponIndex >= static_cast<int>(m_weapons.size())) {
        m_currentWeaponIndex = 0;
    }

    BaseWeapon* activeWeapon = m_weapons[static_cast<size_t>(m_currentWeaponIndex)].get();
    if (activeWeapon == nullptr) {
        return;
    }

    const QPointF spawnPos = m_player->sceneBoundingRect().center();
    const QPointF dir = worldTarget - spawnPos;

    BaseWeapon::FireResult fire = activeWeapon->tryFire(spawnPos,
                                                        dir,
                                                        m_elapsedSeconds,
                                                        m_ammo,
                                                        m_worldBounds,
                                                        m_upgradeFireRateMultiplier,
                                                        m_upgradePenetrationBonus);

    if (!fire.feedbackEvent.isEmpty()) {
        if (fire.feedbackEvent == QStringLiteral("weapon_on_cooldown")) {
            emit combatFeedbackEvent(QStringLiteral("%1 cooling down").arg(activeWeapon->weaponName()));
        } else if (fire.feedbackEvent == QStringLiteral("weapon_no_ammo")) {
            emit combatFeedbackEvent(QStringLiteral("Out of ammo"));
        }
    }

    if (!fire.fired) {
        return;
    }

    int spawnedBulletCount = 0;
    for (auto& bullet : fire.bullets) {
        if (bullet != nullptr) {
            m_bullets.push_back(std::move(bullet));
            ++spawnedBulletCount;
        }
    }

    m_ammo = qMax(0, m_ammo - fire.ammoConsumed);
    m_shotsFired += qMax(1, spawnedBulletCount);
}

bool CombatLoop::isStarted() const {
    return m_started;
}

bool CombatLoop::isEnded() const {
    return m_ended;
}

bool CombatLoop::isVictory() const {
    return m_victory;
}

int CombatLoop::playerHp() const {
    return (m_player != nullptr) ? m_player->hp() : 0;
}

int CombatLoop::enemiesRemaining() const {
    return static_cast<int>(m_enemies.size());
}

int CombatLoop::hitsTaken() const {
    return m_hitsTaken;
}

int CombatLoop::shotsFired() const {
    return m_shotsFired;
}

int CombatLoop::shotsHit() const {
    return m_shotsHit;
}

int CombatLoop::pickupsCollected() const {
    return m_pickupsCollected;
}

int CombatLoop::iFrameBlocks() const {
    return m_iFrameBlocks;
}

double CombatLoop::iFrameRemainingSeconds() const {
    return m_playerDamageCooldownRemaining;
}

bool CombatLoop::isPlayerInIFrame() const {
    return m_playerDamageCooldownRemaining > 0.0;
}

double CombatLoop::elapsedSeconds() const {
    return m_elapsedSeconds;
}

double CombatLoop::remainingSeconds() const {
    return qMax(0.0, m_timeLimit - m_elapsedSeconds);
}

int CombatLoop::enemiesKilled() const {
    return m_enemiesKilled;
}

int CombatLoop::totalEnemiesSpawned() const {
    return m_totalEnemiesSpawned;
}

void CombatLoop::stopMusic() {
    if (m_bgmPlayer != nullptr) {
        m_bgmPlayer->stop();
        qInfo() << "[CombatLoop] Music stopped by request";
    }
}

QString CombatLoop::currentWeaponName() const {
    if (m_weapons.empty()) {
        return QStringLiteral("None");
    }

    if (m_currentWeaponIndex < 0 || m_currentWeaponIndex >= static_cast<int>(m_weapons.size())) {
        return QStringLiteral("None");
    }

    const BaseWeapon* weapon = m_weapons[static_cast<size_t>(m_currentWeaponIndex)].get();
    return (weapon != nullptr) ? weapon->weaponName() : QStringLiteral("None");
}

int CombatLoop::currentAmmo() const {
    return m_ammo;
}

void CombatLoop::setScreenShakeEnabled(bool enabled) {
    if (m_feedbackSystem != nullptr) {
        m_feedbackSystem->setScreenShakeEnabled(enabled);
    }
}

bool CombatLoop::isScreenShakeEnabled() const {
    return (m_feedbackSystem != nullptr) ? m_feedbackSystem->isScreenShakeEnabled() : true;
}

QString CombatLoop::directorDebugText() const {
    return m_directorDebugText;
}

void CombatLoop::switchWeaponByDigit(int digit) {
    if (digit < 1 || digit > static_cast<int>(m_weapons.size())) {
        return;
    }

    const int nextIndex = digit - 1;
    if (nextIndex == m_currentWeaponIndex) {
        return;
    }

    m_currentWeaponIndex = nextIndex;
    emit combatFeedbackEvent(QStringLiteral("Weapon: %1").arg(currentWeaponName()));
}

void CombatLoop::setSpawnerPaused(bool paused) {
    if (m_enemyDirector == nullptr) {
        return;
    }

    m_enemyDirector->setPaused(paused);
    emit combatFeedbackEvent(paused
                                 ? QStringLiteral("Spawner paused")
                                 : QStringLiteral("Spawner resumed"));
}

bool CombatLoop::isSpawnerPaused() const {
    return (m_enemyDirector != nullptr) ? m_enemyDirector->isPaused() : false;
}

void CombatLoop::setNarrativeCombatModifier(double enemyDensityBias,
                                            double dropBias,
                                            double specialEventChance) {
    m_modifierEnemyDensityBias = qBound(0.85, enemyDensityBias, 1.20);
    m_modifierDropBias = qBound(0.85, dropBias, 1.20);
    m_modifierSpecialEventChance = qBound(0.0, specialEventChance, 1.0);

    emit combatFeedbackEvent(QStringLiteral("Modifier: Densx%1 Dropx%2")
                                 .arg(m_modifierEnemyDensityBias, 0, 'f', 2)
                                 .arg(m_modifierDropBias, 0, 'f', 2));
}

void CombatLoop::setTacticalUpgrade(double fireRateMultiplier,
                                    int penetrationBonus,
                                    double damageReductionRatio,
                                    const QString& title,
                                    const QString& description) {
    m_upgradeFireRateMultiplier = qBound(0.90, fireRateMultiplier, 1.25);
    m_upgradePenetrationBonus = qBound(0, penetrationBonus, 3);
    m_upgradeDamageReductionRatio = qBound(0.0, damageReductionRatio, 0.20);
    m_upgradeTitle = title;
    m_upgradeDescription = description;

    emit combatFeedbackEvent(QStringLiteral("Upgrade: %1").arg(m_upgradeTitle.isEmpty() ? QStringLiteral("None") : m_upgradeTitle));
}

PlayerEntity* CombatLoop::player() const {
    return m_player.get();
}

const std::vector<std::unique_ptr<EnemyEntity>>& CombatLoop::enemies() const {
    return m_enemies;
}

const std::vector<std::unique_ptr<BulletItem>>& CombatLoop::bullets() const {
    return m_bullets;
}

const std::vector<std::unique_ptr<EnemyBulletItem>>& CombatLoop::enemyBullets() const {
    return m_enemyBullets;
}

const std::vector<std::unique_ptr<PickupItem>>& CombatLoop::pickups() const {
    return m_pickups;
}

const std::vector<std::unique_ptr<TrapItem>>& CombatLoop::traps() const {
    return m_traps;
}

void CombatLoop::spawnWave() {
    m_enemies.clear();
    m_pickups.clear();
}

void CombatLoop::updateEnemies(double deltaTimeSeconds) {
    if (m_player == nullptr) {
        return;
    }

    const QPointF target = m_player->sceneBoundingRect().center();
    for (auto& enemy : m_enemies) {
        if (enemy == nullptr || !enemy->isAlive()) {
            continue;
        }

        enemy->update(deltaTimeSeconds, target);

        const int pendingDamage = enemy->consumePendingAttackDamage();
        if (pendingDamage > 0 && m_playerDamageCooldownRemaining <= 0.0) {
            m_player->applyDamage(pendingDamage);
            ++m_hitsTaken;
            m_playerDamageCooldownRemaining = kPlayerIFrameSeconds;

            if (m_feedbackSystem != nullptr) {
                m_feedbackSystem->onHurt(pendingDamage, -enemy->attackDirectionToPlayer());
            }

            emit feedbackHurtTriggered(0.52, -enemy->attackDirectionToPlayer());
            emit combatFeedbackEvent(QStringLiteral("Enemy attack: -%1 HP").arg(pendingDamage));
        }

        auto bullets = enemy->shoot(m_worldBounds);
        for (auto& bullet : bullets) {
            m_enemyBullets.push_back(std::move(bullet));
        }
    }
}

void CombatLoop::updateBullets(double deltaTimeSeconds) {
    for (auto& bullet : m_bullets) {
        bullet->update(deltaTimeSeconds);
    }

    for (auto& bullet : m_enemyBullets) {
        bullet->update(deltaTimeSeconds);
    }
}

void CombatLoop::resolveCollisions(double deltaTimeSeconds) {
    if (m_player == nullptr || m_collisionManager == nullptr) {
        return;
    }

    const CollisionManager::FrameResult frame = m_collisionManager->detectCollisions(
        m_player.get(),
        m_enemies,
        m_bullets,
        m_enemyBullets,
        m_pickups,
        m_traps,
        m_playerDamageCooldownRemaining <= 0.0,
        deltaTimeSeconds);

    if (frame.playerDamage > 0) {
        const int hpBefore = m_player->hp();
        const double clampedReduction = qBound(0.0, m_upgradeDamageReductionRatio, 0.50);
        const int reducedDamage = qMax(1,
                                       static_cast<int>(qRound(static_cast<double>(frame.playerDamage)
                                                               * (1.0 - clampedReduction))));
        m_player->applyDamage(reducedDamage);
        m_hitsTaken += frame.playerEnemyCollisions;
        m_playerDamageCooldownRemaining = kPlayerIFrameSeconds;

        if (m_feedbackSystem != nullptr) {
            m_feedbackSystem->onHurt(reducedDamage, QPointF(-1.0, 0.0));
        }
        emit feedbackHurtTriggered(0.45, QPointF(-1.0, 0.0));

        const int hpAfter = m_player->hp();
        if (hpAfter < hpBefore) {
            emit combatFeedbackEvent(QStringLiteral("Player hurt -%1 HP").arg(hpBefore - hpAfter));
        }
    }

    if (frame.trapPlayerCollisions > 0) {
        m_player->applyTrapDamage(frame.trapPlayerCollisions);
        emit combatFeedbackEvent(QStringLiteral("Trap damage! -%1 HP, stunned 3s").arg(frame.trapPlayerCollisions));
    }

    if (frame.playerEnemyBlockedByIFrame > 0) {
        m_iFrameBlocks += frame.playerEnemyBlockedByIFrame;
    }

    m_shotsHit += frame.shotsHit;

    if (frame.shotsHit > 0) {
        if (m_feedbackSystem != nullptr) {
            const int representativeDamage = frame.shotsHit >= 2 ? 30 : 16;
            m_feedbackSystem->onHit(representativeDamage);
            emit feedbackHitStopTriggered();
            m_feedbackSystem->onKill(representativeDamage);
            emit feedbackKillTriggered(0.10, representativeDamage >= 26 ? 10.0 : 5.0);
        }
    }

    for (auto& enemy : m_enemies) {
        if (enemy == nullptr) {
            continue;
        }

        if (containsEnemyPointer(frame.enemiesToRecycle, enemy.get())) {
            const QPointF deathPos = enemy->sceneBoundingRect().center();
            enemy->kill();
            ++m_enemiesKilled;

            ++m_dropCounter;
            const int baseEvery = 5;
            const int adjustedEvery = qMax(2, static_cast<int>(qRound(static_cast<double>(baseEvery) / m_modifierDropBias)));
            const bool shouldDrop = (m_dropCounter % adjustedEvery) == 0;
            if (shouldDrop) {
                m_pickups.push_back(std::make_unique<PickupItem>(deathPos, m_pickupHealAmount));
            }
        }
    }

    for (auto& bullet : m_bullets) {
        if (bullet == nullptr || !bullet->isAlive()) {
            continue;
        }

        if (containsBulletPointer(frame.bulletsToRecycle, bullet.get())) {
            bullet->kill();
        }
    }

    for (auto& pickup : m_pickups) {
        if (pickup == nullptr || !pickup->isAlive()) {
            continue;
        }

        if (!containsPickupPointer(frame.pickupsToRecycle, pickup.get())) {
            continue;
        }

        pickup->collect();
        ++m_pickupsCollected;

        if (m_player != nullptr) {
            m_player->applyDamage(-pickup->healAmount());
        }
    }
}

void CombatLoop::cleanupDeadObjects() {
    auto enemyIt = std::remove_if(m_enemies.begin(),
                                  m_enemies.end(),
                                  [](const std::unique_ptr<EnemyEntity>& enemy) {
                                      return !enemy->isAlive();
                                  });
    m_enemies.erase(enemyIt, m_enemies.end());

    auto bulletIt = std::remove_if(m_bullets.begin(),
                                   m_bullets.end(),
                                   [](const std::unique_ptr<BulletItem>& bullet) {
                                       return !bullet->isAlive();
                                   });
    m_bullets.erase(bulletIt, m_bullets.end());

    auto enemyBulletIt = std::remove_if(m_enemyBullets.begin(),
                                        m_enemyBullets.end(),
                                        [](const std::unique_ptr<EnemyBulletItem>& bullet) {
                                            return !bullet->isAlive();
                                        });
    m_enemyBullets.erase(enemyBulletIt, m_enemyBullets.end());

    auto pickupIt = std::remove_if(m_pickups.begin(),
                                   m_pickups.end(),
                                   [](const std::unique_ptr<PickupItem>& pickup) {
                                       return !pickup->isAlive();
                                   });
    m_pickups.erase(pickupIt, m_pickups.end());
}

void CombatLoop::finishCombat(bool victory) {
    if (m_ended) {
        return;
    }

    m_ended = true;
    m_victory = victory;

    if (m_bgmPlayer != nullptr) {
        m_bgmPlayer->stop();
        qInfo() << "[CombatLoop] BGM stopped";
    }

    double accuracy = 0.0;
    if (m_shotsFired > 0) {
        accuracy = static_cast<double>(m_shotsHit) / static_cast<double>(m_shotsFired);
    }

    emit combatLoopFinished(victory,
                            m_elapsedSeconds,
                            m_hitsTaken,
                            accuracy,
                            m_pickupsCollected,
                            m_iFrameBlocks,
                            m_enemiesKilled,
                            m_totalEnemiesSpawned,
                            m_shotsFired);
}

bool CombatLoop::containsEnemyPointer(const QVector<EnemyEntity*>& list, EnemyEntity* value) const {
    if (value == nullptr) {
        return false;
    }

    for (EnemyEntity* item : list) {
        if (item == value) {
            return true;
        }
    }

    return false;
}

bool CombatLoop::containsBulletPointer(const QVector<BulletItem*>& list, BulletItem* value) const {
    if (value == nullptr) {
        return false;
    }

    for (BulletItem* item : list) {
        if (item == value) {
            return true;
        }
    }

    return false;
}

bool CombatLoop::containsPickupPointer(const QVector<PickupItem*>& list, PickupItem* value) const {
    if (value == nullptr) {
        return false;
    }

    for (PickupItem* item : list) {
        if (item == value) {
            return true;
        }
    }

    return false;
}
