#pragma once

#include <QGraphicsRectItem>

class PickupItem final : public QGraphicsRectItem {
public:
    explicit PickupItem(const QPointF& pos, int healAmount = 10);

    bool isAlive() const;
    void collect();
    int healAmount() const;

private:
    bool m_alive {true};
    int m_healAmount {10};
};
