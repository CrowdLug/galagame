#include "TrapItem.h"

#include <QBrush>
#include <QColor>
#include <QPen>
#include <Qt>

TrapItem::TrapItem(const QPointF& center, double radius)
    : QGraphicsEllipseItem(-radius, -radius, radius * 2.0, radius * 2.0) {
    setPos(center);
    setBrush(QBrush(QColor(255, 50, 50, 180)));
    setPen(QPen(Qt::NoPen));
}

void TrapItem::update(double deltaTimeSeconds) {
    if (m_triggered) {
        m_triggeredTime += deltaTimeSeconds;
    } else {
        m_lifetime += deltaTimeSeconds;
    }
}

bool TrapItem::isExpired() const {
    if (m_triggered) {
        return m_triggeredTime >= 3.0;
    }
    return m_lifetime >= kMaxLifetime;
}

void TrapItem::trigger() {
    m_triggered = true;
}

bool TrapItem::isTriggered() const {
    return m_triggered;
}
