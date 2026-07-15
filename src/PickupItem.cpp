#include "PickupItem.h"

#include <QBrush>
#include <QColor>
#include <QPen>

PickupItem::PickupItem(const QPointF& pos, int healAmount)
    : m_healAmount(healAmount) {
    setRect(0.0, 0.0, 20.0, 20.0);
    setBrush(QBrush(QColor(100, 255, 150)));
    setPen(QPen(Qt::NoPen));
    setPos(pos);
}

bool PickupItem::isAlive() const {
    return m_alive;
}

void PickupItem::collect() {
    m_alive = false;
}

int PickupItem::healAmount() const {
    return m_healAmount;
}
