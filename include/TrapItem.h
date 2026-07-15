#pragma once

#include <QGraphicsEllipseItem>
#include <QPointF>

class TrapItem final : public QGraphicsEllipseItem {
public:
    TrapItem(const QPointF& center, double radius);

    void update(double deltaTimeSeconds);
    bool isExpired() const;
    void trigger();
    bool isTriggered() const;

private:
    bool m_triggered {false};
    double m_triggeredTime {0.0};
    double m_lifetime {0.0};
    static constexpr double kMaxLifetime {30.0};
};
