#pragma once

#include <QVector>

#include <memory>
#include <vector>

class PlayerEntity;
class EnemyEntity;
class BulletItem;
class EnemyBulletItem;
class PickupItem;
class TrapItem;

class CollisionManager final {
public:
    struct FrameResult {
        int playerDamage {0};
        int shotsHit {0};

        QVector<EnemyEntity*> enemiesToRecycle;
        QVector<BulletItem*> bulletsToRecycle;
        QVector<EnemyBulletItem*> enemyBulletsToRecycle;
        QVector<PickupItem*> pickupsToRecycle;

        int playerEnemyCollisions {0};
        int playerEnemyBlockedByIFrame {0};
        int bulletEnemyCollisions {0};
        int pickupPlayerCollisions {0};
        int trapPlayerCollisions {0};
        int enemyBulletPlayerCollisions {0};
    };

    FrameResult detectCollisions(PlayerEntity* player,
                                const std::vector<std::unique_ptr<EnemyEntity>>& enemies,
                                const std::vector<std::unique_ptr<BulletItem>>& bullets,
                                const std::vector<std::unique_ptr<EnemyBulletItem>>& enemyBullets,
                                const std::vector<std::unique_ptr<PickupItem>>& pickups,
                                const std::vector<std::unique_ptr<TrapItem>>& traps,
                                bool playerCanTakeDamage,
                                double deltaTimeSeconds);

private:
    void flushPerSecondStatsIfNeeded(double deltaTimeSeconds,
                                    int playerEnemyCollisions,
                                    int bulletEnemyCollisions,
                                    int pickupPlayerCollisions);

    double m_statWindowSeconds {0.0};
    int m_statPlayerEnemy {0};
    int m_statBulletEnemy {0};
    int m_statPickupPlayer {0};
};
