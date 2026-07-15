#pragma once

#include <QGraphicsPixmapItem>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <memory>
#include <vector>

class EnemyBulletItem;

class EnemyEntity final : public QGraphicsPixmapItem {
public:
    enum class Archetype {
        Melee,
        Ranged
    };

    enum class State {
        Idle,
        Chase,
        Attack,
        Retreat
    };

    struct AIConfig {
        double detectRadius {5000.0};
        double speed {170.0};
        double attackRange {54.0};
        double preferredRange {280.0};
        double attackInterval {0.9};
        int attackDamage {12};
        int retreatHpThreshold {30};
        double retreatDuration {0.7};
    };

    EnemyEntity(const QPointF& spawnPos, Archetype archetype, const AIConfig& config);

    void update(double deltaTimeSeconds, const QPointF& targetPos);
    bool isAlive() const;
    void kill();

    int consumePendingAttackDamage();
    QPointF attackDirectionToPlayer() const;
    Archetype archetype() const;
    State state() const;

    void applyDamage(int damage);
    int hp() const;

    std::vector<std::unique_ptr<EnemyBulletItem>> shoot(const QRectF& worldBounds);

    static AIConfig loadConfigFromJson(const QString& path, const AIConfig& fallback);

private:
    void transitionTo(State next, const QString& reason);
    static QString stateName(State state);

    Archetype m_archetype {Archetype::Melee};
    State m_state {State::Idle};
    AIConfig m_config;

    QPointF m_attackDirToPlayer {1.0, 0.0};

    bool m_alive {true};
    double m_attackCooldownRemaining {0.0};
    double m_retreatRemaining {0.0};
    int m_pendingAttackDamage {0};
    int m_hp {1};
    bool m_needsToShoot {false};
};
