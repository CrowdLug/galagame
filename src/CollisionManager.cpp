#include "CollisionManager.h"

#include <QDebug>
#include <QSet>
#include <QtGlobal>

#include "BulletItem.h"
#include "EnemyBulletItem.h"
#include "EnemyEntity.h"
#include "PickupItem.h"
#include "PlayerEntity.h"
#include "TrapItem.h"

CollisionManager::FrameResult CollisionManager::detectCollisions(
    PlayerEntity* player,
    const std::vector<std::unique_ptr<EnemyEntity>>& enemies,
    const std::vector<std::unique_ptr<BulletItem>>& bullets,
    const std::vector<std::unique_ptr<EnemyBulletItem>>& enemyBullets,
    const std::vector<std::unique_ptr<PickupItem>>& pickups,
    const std::vector<std::unique_ptr<TrapItem>>& traps,
    bool playerCanTakeDamage,
    double deltaTimeSeconds) {
    FrameResult frame;

    if (player == nullptr) {
        return frame;
    }

    QSet<EnemyEntity*> uniqueEnemyRecycle;
    QSet<BulletItem*> uniqueBulletRecycle;
    QSet<EnemyBulletItem*> uniqueEnemyBulletRecycle;

    for (const auto& enemyPtr : enemies) {
        EnemyEntity* enemy = enemyPtr.get();
        if (enemy == nullptr || !enemy->isAlive()) {
            continue;
        }

        if (player->collidesWithItem(enemy)) {
            if (playerCanTakeDamage) {
                ++frame.playerEnemyCollisions;
                frame.playerDamage += 20;
                uniqueEnemyRecycle.insert(enemy);
            } else {
                ++frame.playerEnemyBlockedByIFrame;
            }
        }
    }

    for (const auto& bulletPtr : bullets) {
        BulletItem* bullet = bulletPtr.get();
        if (bullet == nullptr || !bullet->isAlive()) {
            continue;
        }

        for (const auto& enemyPtr : enemies) {
            EnemyEntity* enemy = enemyPtr.get();
            if (enemy == nullptr || !enemy->isAlive()) {
                continue;
            }

            if (uniqueEnemyRecycle.contains(enemy)) {
                continue;
            }

            if (bullet->collidesWithItem(enemy)) {
                ++frame.bulletEnemyCollisions;
                enemy->applyDamage(bullet->damage());
                uniqueBulletRecycle.insert(bullet);
                if (!enemy->isAlive()) {
                    uniqueEnemyRecycle.insert(enemy);
                }
                frame.shotsHit += qMax(1, bullet->damage() / 16);
                break;
            }
        }
    }

    for (const auto& bulletPtr : enemyBullets) {
        EnemyBulletItem* bullet = bulletPtr.get();
        if (bullet == nullptr || !bullet->isAlive()) {
            continue;
        }

        if (player->collidesWithItem(bullet)) {
            ++frame.enemyBulletPlayerCollisions;
            frame.playerDamage += bullet->damage();
            uniqueEnemyBulletRecycle.insert(bullet);
        }
    }

    QSet<PickupItem*> uniquePickupRecycle;

    for (const auto& pickupPtr : pickups) {
        PickupItem* pickup = pickupPtr.get();
        if (pickup == nullptr || !pickup->isAlive()) {
            continue;
        }

        if (player->collidesWithItem(pickup)) {
            ++frame.pickupPlayerCollisions;
            uniquePickupRecycle.insert(pickup);
        }
    }

    for (const auto& trapPtr : traps) {
        TrapItem* trap = trapPtr.get();
        if (trap == nullptr || trap->isExpired()) {
            continue;
        }

        if (player->collidesWithItem(trap) && !trap->isTriggered()) {
            trap->trigger();
            frame.trapPlayerCollisions += 10;
        }
    }

    frame.pickupsToRecycle = QVector<PickupItem*>(uniquePickupRecycle.begin(), uniquePickupRecycle.end());

    frame.enemiesToRecycle = QVector<EnemyEntity*>(uniqueEnemyRecycle.begin(), uniqueEnemyRecycle.end());
    frame.bulletsToRecycle = QVector<BulletItem*>(uniqueBulletRecycle.begin(), uniqueBulletRecycle.end());
    frame.enemyBulletsToRecycle = QVector<EnemyBulletItem*>(uniqueEnemyBulletRecycle.begin(), uniqueEnemyBulletRecycle.end());

    flushPerSecondStatsIfNeeded(deltaTimeSeconds,
                                frame.playerEnemyCollisions,
                                frame.bulletEnemyCollisions,
                                frame.pickupPlayerCollisions);

    return frame;
}

void CollisionManager::flushPerSecondStatsIfNeeded(double deltaTimeSeconds,
                                                   int playerEnemyCollisions,
                                                   int bulletEnemyCollisions,
                                                   int pickupPlayerCollisions) {
    m_statWindowSeconds += deltaTimeSeconds;
    m_statPlayerEnemy += playerEnemyCollisions;
    m_statBulletEnemy += bulletEnemyCollisions;
    m_statPickupPlayer += pickupPlayerCollisions;

    if (m_statWindowSeconds < 1.0) {
        return;
    }

    qInfo() << "[CollisionManager][1s]"
            << "playerVsEnemy=" << m_statPlayerEnemy
            << "bulletVsEnemy=" << m_statBulletEnemy
            << "pickupVsPlayer=" << m_statPickupPlayer;

    m_statWindowSeconds = 0.0;
    m_statPlayerEnemy = 0;
    m_statBulletEnemy = 0;
    m_statPickupPlayer = 0;
}
