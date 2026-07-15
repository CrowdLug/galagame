#pragma once

#include <QGraphicsEllipseItem>
#include <QPointF>

class EnemyBulletItem final : public QGraphicsEllipseItem {
public:
    EnemyBulletItem(const QPointF& spawnPos,
                    const QPointF& direction,
                    const QRectF& worldBounds,
                    qreal speed,
                    int damage);

    void update(double deltaTimeSeconds);
    bool isAlive() const;
    void kill();
    int damage() const;

private:
    QPointF m_direction;
    QRectF m_worldBounds;
    qreal m_speed;
    int m_damage;
    bool m_alive {true};
};
