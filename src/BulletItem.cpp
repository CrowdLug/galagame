#include "BulletItem.h"

#include <QBrush>
#include <QColor>
#include <QPen>
#include <QtMath>

BulletItem::BulletItem(const QPointF& spawnPos,
                       const QPointF& direction,
                       const QRectF& worldBounds,
                       qreal speed,
                       int damage)
    : m_direction(direction)
    , m_worldBounds(worldBounds)
    , m_speed(speed)
    , m_damage(damage) {
    const qreal len = std::hypot(m_direction.x(), m_direction.y());
    if (len > 0.001) {
        m_direction = QPointF(m_direction.x() / len, m_direction.y() / len);
    } else {
        m_direction = QPointF(1.0, 0.0);
    }

    const qreal radius = 5.0;
    setRect(-radius, -radius, radius * 2.0, radius * 2.0);
    setBrush(QBrush(QColor(255, 0, 0)));
    setPen(QPen(Qt::NoPen));
    setPos(spawnPos);
}

void BulletItem::update(double deltaTimeSeconds) {
    if (!m_alive) {
        return;
    }

    const QPointF nextPos = pos() + m_direction * (m_speed * deltaTimeSeconds);
    setPos(nextPos);

    if (!m_worldBounds.intersects(sceneBoundingRect())) {
        m_alive = false;
    }
}

bool BulletItem::isAlive() const {
    return m_alive;
}

void BulletItem::kill() {
    m_alive = false;
}

int BulletItem::damage() const {
    return m_damage;
}
