#pragma once

#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>

#include <memory>
#include <vector>

#include "GameManager.h"
#include "WeaponSystem.h"
#include "EnemyEntity.h"
#include "EnemyDirector.h"

class QMediaPlayer;

class PlayerEntity;
class BulletItem;
class EnemyBulletItem;
class PickupItem;
class CollisionManager;
class FeedbackSystem;
class LightDirector;
class TrapItem;

class CombatLoop final : public QObject {
    Q_OBJECT

public:
    explicit CombatLoop(QObject* parent = nullptr);
    ~CombatLoop() override;

    void start();
    void reset();
    void update(double deltaTimeSeconds);

    void setGameMode(GameMode mode);
    GameMode gameMode() const;

    void setMovementInput(bool up, bool down, bool left, bool right);
    void shootTowards(const QPointF& worldTarget);
    void switchWeaponByDigit(int digit);
    void setSpawnerPaused(bool paused);
    bool isSpawnerPaused() const;
    void setNarrativeCombatModifier(double enemyDensityBias,
                                    double dropBias,
                                    double specialEventChance);
    void setTacticalUpgrade(double fireRateMultiplier,
                            int penetrationBonus,
                            double damageReductionRatio,
                            const QString& title,
                            const QString& description);

    QString currentWeaponName() const;
    int currentAmmo() const;
    void setScreenShakeEnabled(bool enabled);
    bool isScreenShakeEnabled() const;
    QString directorDebugText() const;

    bool isStarted() const;
    bool isEnded() const;
    bool isVictory() const;
    int playerHp() const;
    int enemiesRemaining() const;
    int hitsTaken() const;
    int shotsFired() const;
    int shotsHit() const;
    int pickupsCollected() const;
    int iFrameBlocks() const;
    double iFrameRemainingSeconds() const;
    bool isPlayerInIFrame() const;
    double elapsedSeconds() const;
    double remainingSeconds() const;
    int enemiesKilled() const;
    int totalEnemiesSpawned() const;
    void stopMusic();

    PlayerEntity* player() const;
    const std::vector<std::unique_ptr<EnemyEntity>>& enemies() const;
    const std::vector<std::unique_ptr<BulletItem>>& bullets() const;
    const std::vector<std::unique_ptr<EnemyBulletItem>>& enemyBullets() const;
    const std::vector<std::unique_ptr<PickupItem>>& pickups() const;
    const std::vector<std::unique_ptr<TrapItem>>& traps() const;

signals:
    void combatLoopStarted();
    void combatLoopFinished(bool victory,
                            double clearTimeSeconds,
                            int hitsTaken,
                            double accuracy,
                            int pickupsCollected,
                            int iFrameBlocks,
                            int enemiesKilled,
                            int totalEnemiesSpawned,
                            int shotsFired);
    void combatFeedbackEvent(const QString& eventText);
    void feedbackHitStopTriggered();
    void feedbackHurtTriggered(double blinkAlpha, const QPointF& hurtDirection);
    void feedbackKillTriggered(double shakeSeconds, double shakeAmplitude);

private:
    void spawnWave();
    void spawnEnemies(const EnemyDirector::SpawnRequest& req);
    void updateEnemies(double deltaTimeSeconds);
    void updateBullets(double deltaTimeSeconds);
    void updateSpawning();
    void updateTraps(double deltaTimeSeconds);
    void updateEndlessSpawning(double deltaTimeSeconds);
    void resolveCollisions(double deltaTimeSeconds);
    void cleanupDeadObjects();
    void cleanupTraps();
    void spawnTraps();
    void finishCombat(bool victory);
    void forceSpawnNextWave();
    void spawnEndlessWave();

    bool containsEnemyPointer(const QVector<EnemyEntity*>& list, EnemyEntity* value) const;
    bool containsBulletPointer(const QVector<BulletItem*>& list, BulletItem* value) const;
    bool containsPickupPointer(const QVector<PickupItem*>& list, PickupItem* value) const;

    QRectF m_worldBounds {0.0, 0.0, 1920.0, 1080.0};

    std::unique_ptr<PlayerEntity> m_player;
    std::vector<std::unique_ptr<EnemyEntity>> m_enemies;
    std::vector<std::unique_ptr<BulletItem>> m_bullets;
    std::vector<std::unique_ptr<EnemyBulletItem>> m_enemyBullets;
    std::vector<std::unique_ptr<PickupItem>> m_pickups;
    std::vector<std::unique_ptr<TrapItem>> m_traps;

    std::unique_ptr<CollisionManager> m_collisionManager;
    std::unique_ptr<FeedbackSystem> m_feedbackSystem;
    std::unique_ptr<EnemyDirector> m_enemyDirector;
    std::unique_ptr<LightDirector> m_lightDirector;
    std::unique_ptr<QMediaPlayer> m_bgmPlayer;
    QString m_directorDebugText;

    EnemyEntity::AIConfig m_meleeConfig;
    EnemyEntity::AIConfig m_rangedConfig;

    GameMode m_gameMode {GameMode::Normal};
    bool m_started {false};
    bool m_ended {false};
    bool m_victory {false};

    int m_hitsTaken {0};
    int m_shotsFired {0};
    int m_shotsHit {0};
    int m_pickupsCollected {0};
    int m_iFrameBlocks {0};
    int m_enemiesKilled {0};
    int m_totalEnemiesSpawned {0};
    int m_playerMaxHp {100};
    int m_ammo {100};
    int m_dropCounter {0};
    int m_currentWeaponIndex {0};
    int m_currentWave {0};
    int m_pickupHealAmount {10};
    double m_elapsedSeconds {0.0};
    double m_timeLimit {60.0};
    double m_playerDamageCooldownRemaining {0.0};
    double m_lastSpawnTime {0.0};

    double m_modifierEnemyDensityBias {1.0};
    double m_modifierDropBias {1.0};
    double m_modifierSpecialEventChance {0.10};

    double m_upgradeFireRateMultiplier {1.0};
    int m_upgradePenetrationBonus {0};
    double m_upgradeDamageReductionRatio {0.0};
    QString m_upgradeTitle;
    QString m_upgradeDescription;

    std::vector<std::unique_ptr<BaseWeapon>> m_weapons;

    double m_trapSpawnTimer {0.0};
    int m_currentSpawnEventIndex {0};

    static constexpr double kPlayerIFrameSeconds {0.55};
    static constexpr double kTrapSpawnInterval {10.0};
    static constexpr double kTrapRadius {40.0};
    static constexpr int kTrapsPerSpawn {3};
    static constexpr double kEndlessSpawnInterval {20.0};
};
