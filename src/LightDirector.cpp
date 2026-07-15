#include "LightDirector.h"

#include <QtMath>

LightDirector::LightDirector() {
    reset();
}

void LightDirector::reset() {
    m_hitWindow.clear();
    m_killWindow.clear();
    m_smoothed = Decision {};
    m_smoothed.spawnIntervalScale = 1.0;
    m_smoothed.densityScale = 1.0;
    m_smoothed.allowElite = true;
    m_smoothed.label = QStringLiteral("Stable");
    m_debugText = QStringLiteral("Director: Stable");
}

LightDirector::Decision LightDirector::update(const Metrics& metrics) {
    const double now = qMax(0.0, metrics.elapsedSeconds);

    m_hitWindow.enqueue(HitSample {now, qMax(0, metrics.hitsTakenTotal)});
    m_killWindow.enqueue(KillSample {now, qMax(0, metrics.killsTotal)});

    while (!m_hitWindow.isEmpty() && (now - m_hitWindow.head().t) > 30.0) {
        m_hitWindow.dequeue();
    }
    while (!m_killWindow.isEmpty() && (now - m_killWindow.head().t) > 30.0) {
        m_killWindow.dequeue();
    }

    const int maxHp = qMax(1, metrics.maxPlayerHp);
    const double hpRatio = clamp01(static_cast<double>(metrics.playerHp) / static_cast<double>(maxHp));

    double hitRate30s = 0.0;
    if (!m_hitWindow.isEmpty()) {
        const int deltaHits = qMax(0, metrics.hitsTakenTotal - m_hitWindow.head().totalHits);
        hitRate30s = static_cast<double>(deltaHits) / 30.0;
    }

    double dpm = 0.0;
    if (!m_killWindow.isEmpty()) {
        const int deltaKills = qMax(0, metrics.killsTotal - m_killWindow.head().totalKills);
        dpm = static_cast<double>(deltaKills) * 2.0;
    }

    const double hpStress = clamp01((0.65 - hpRatio) / 0.65);
    const double hitStress = clamp01(hitRate30s / 0.18);
    const double perfPressure = clamp01(dpm / 16.0);
    const double timePressure = clamp01(metrics.elapsedSeconds / 160.0);

    double stress = 0.55 * hpStress + 0.45 * hitStress;
    double pressure = 0.60 * perfPressure + 0.40 * timePressure;

    double targetInterval = 1.0 + 0.22 * stress - 0.20 * pressure;
    double targetDensity = 1.0 - 0.25 * stress + 0.22 * pressure;

    targetInterval = qBound(0.80, targetInterval, 1.22);
    targetDensity = qBound(0.80, targetDensity, 1.22);

    const bool allowElite = !(hpRatio < 0.30 && hitRate30s > 0.08);

    const double smooth = 0.08;
    m_smoothed.spawnIntervalScale = lerp(m_smoothed.spawnIntervalScale, targetInterval, smooth);
    m_smoothed.densityScale = lerp(m_smoothed.densityScale, targetDensity, smooth);
    m_smoothed.allowElite = allowElite;

    if (stress > 0.56) {
        m_smoothed.label = QStringLiteral("Buffer Window");
    } else if (pressure > 0.56) {
        m_smoothed.label = QStringLiteral("Pressure Up");
    } else {
        m_smoothed.label = QStringLiteral("Stable");
    }

    m_debugText = QStringLiteral("Director[%1] HP:%2% Hit30:%3/s DPM:%4 I:%5 D:%6 Elite:%7")
                      .arg(m_smoothed.label)
                      .arg(static_cast<int>(hpRatio * 100.0))
                      .arg(hitRate30s, 0, 'f', 2)
                      .arg(dpm, 0, 'f', 1)
                      .arg(m_smoothed.spawnIntervalScale, 0, 'f', 2)
                      .arg(m_smoothed.densityScale, 0, 'f', 2)
                      .arg(m_smoothed.allowElite ? QStringLiteral("Y") : QStringLiteral("N"));

    return m_smoothed;
}

QString LightDirector::debugText() const {
    return m_debugText;
}

double LightDirector::clamp01(double v) {
    return qBound(0.0, v, 1.0);
}

double LightDirector::lerp(double a, double b, double t) {
    return a + (b - a) * clamp01(t);
}
