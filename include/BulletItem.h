#pragma once

#include <QGraphicsRectItem>
#include <QPointF>

class BulletItem final : public QGraphicsRectItem {
public:
    BulletItem(const QPointF& spawnPos,
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
    qreal m_speed {760.0};
    int m_damage {20};
    bool m_alive {true};
};
