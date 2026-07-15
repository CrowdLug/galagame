#pragma once

#include <QGraphicsPixmapItem>
#include <QPointF>

class PlayerEntity final : public QGraphicsPixmapItem {
public:
    explicit PlayerEntity(const QRectF& bounds);

    void setMovementInput(bool up, bool down, bool left, bool right);
    void update(double deltaTimeSeconds);

    int hp() const;
    void applyDamage(int damage);

    void applyTrapDamage(int damage);
    bool isTrapped() const;
    double trapRemainingSeconds() const;

private:
    QRectF m_worldBounds;
    QPointF m_velocity;
    int m_hp {100};
    double m_moveSpeed {465.0};

    bool m_up {false};
    bool m_down {false};
    bool m_left {false};
    bool m_right {false};

    bool m_trapped {false};
    double m_trapRemaining {0.0};
};
