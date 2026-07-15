#include "FeedbackSystem.h"

#include <QRandomGenerator>
#include <QtMath>

#include <algorithm>

FeedbackSystem::FeedbackSystem() = default;

void FeedbackSystem::setConfig(const Config& config) {
    m_config = config;
}

const FeedbackSystem::Config& FeedbackSystem::config() const {
    return m_config;
}

void FeedbackSystem::setScreenShakeEnabled(bool enabled) {
    m_config.screenShakeEnabled = enabled;
}

bool FeedbackSystem::isScreenShakeEnabled() const {
    return m_config.screenShakeEnabled;
}

void FeedbackSystem::onHit(int damage) {
    if (m_eventCooldownRemaining > 0.0) {
        return;
    }

    const bool heavy = damage >= m_config.heavyHitDamageThreshold;
    const double stop = heavy ? m_config.hitStopHeavySeconds : m_config.hitStopNormalSeconds;
    m_hitStopRemaining = qMax(m_hitStopRemaining, stop);
    m_eventCooldownRemaining = m_config.eventCooldownSeconds;
}

void FeedbackSystem::onHurt(int damage, const QPointF& direction) {
    if (m_eventCooldownRemaining > 0.0) {
        return;
    }

    m_hurtFlashRemaining = qMax(m_hurtFlashRemaining, m_config.hurtFlashSeconds);
    m_hurtBlinkRemaining = qMax(m_hurtBlinkRemaining, m_config.hurtBlinkSeconds);
    m_lastHurtDirection = normalizedOrDefault(direction);

    const bool heavy = damage >= m_config.heavyHitDamageThreshold;
    m_lastShakeAmplitude = heavy ? m_config.shakeAmplitudeHeavy : m_config.shakeAmplitudeLight;
    if (m_config.screenShakeEnabled) {
        m_killShakeRemaining = qMax(m_killShakeRemaining, m_config.killShakeSeconds * 0.65);
    }

    m_eventCooldownRemaining = m_config.eventCooldownSeconds;
}

void FeedbackSystem::onKill(int damage) {
    if (m_eventCooldownRemaining > 0.0) {
        return;
    }

    m_killFlashRemaining = qMax(m_killFlashRemaining, m_config.killFlashSeconds);

    const bool heavy = damage >= m_config.heavyHitDamageThreshold;
    m_lastShakeAmplitude = heavy ? m_config.shakeAmplitudeHeavy : m_config.shakeAmplitudeLight;

    if (m_config.screenShakeEnabled) {
        m_killShakeRemaining = qMax(m_killShakeRemaining, m_config.killShakeSeconds);
    }

    m_eventCooldownRemaining = m_config.eventCooldownSeconds;
}

FeedbackSystem::FrameEffects FeedbackSystem::update(double deltaTimeSeconds) {
    const double dt = qBound(0.0, deltaTimeSeconds, 0.05);

    m_eventCooldownRemaining = qMax(0.0, m_eventCooldownRemaining - dt);
    m_hitStopRemaining = qMax(0.0, m_hitStopRemaining - dt);
    m_hurtFlashRemaining = qMax(0.0, m_hurtFlashRemaining - dt);
    m_hurtBlinkRemaining = qMax(0.0, m_hurtBlinkRemaining - dt);
    m_killFlashRemaining = qMax(0.0, m_killFlashRemaining - dt);
    m_killShakeRemaining = qMax(0.0, m_killShakeRemaining - dt);

    FrameEffects fx;
    fx.timeScale = (m_hitStopRemaining > 0.0) ? 0.05 : 1.0;

    const double flashMax = qMax(m_config.hurtFlashSeconds, m_config.killFlashSeconds);
    const double hurtAlpha = (m_config.hurtFlashSeconds > 0.0)
                                 ? (m_hurtFlashRemaining / m_config.hurtFlashSeconds)
                                 : 0.0;
    const double killAlpha = (m_config.killFlashSeconds > 0.0)
                                 ? (m_killFlashRemaining / m_config.killFlashSeconds)
                                 : 0.0;
    fx.flashAlpha = qBound(0.0, qMax(hurtAlpha, killAlpha), 1.0);

    if (m_hurtBlinkRemaining > 0.0 && m_config.hurtBlinkSeconds > 0.0) {
        const double progress = 1.0 - (m_hurtBlinkRemaining / m_config.hurtBlinkSeconds);
        const double pulse = std::sin(progress * 18.0);
        fx.playerBlinkAlpha = (pulse > 0.0) ? 1.0 : 0.45;
    } else {
        fx.playerBlinkAlpha = 1.0;
    }

    if (m_config.screenShakeEnabled && m_killShakeRemaining > 0.0 && m_config.killShakeSeconds > 0.0) {
        const double t = m_killShakeRemaining / m_config.killShakeSeconds;
        const double amplitude = m_lastShakeAmplitude * t;

        const auto randomSigned = [](double limit) {
            const double u = QRandomGenerator::global()->generateDouble(); // [0, 1)
            return (u * 2.0 - 1.0) * limit;
        };

        const double jitterX = randomSigned(amplitude);
        const double jitterY = randomSigned(amplitude);

        fx.screenShakeOffset = QPointF(jitterX, jitterY) + m_lastHurtDirection * (amplitude * 0.25);
    } else {
        fx.screenShakeOffset = QPointF(0.0, 0.0);
    }

    Q_UNUSED(flashMax);
    return fx;
}

bool FeedbackSystem::isHitStopActive() const {
    return m_hitStopRemaining > 0.0;
}

QPointF FeedbackSystem::normalizedOrDefault(const QPointF& input) const {
    const double len = std::hypot(input.x(), input.y());
    if (len > 0.0001) {
        return QPointF(input.x() / len, input.y() / len);
    }

    return QPointF(1.0, 0.0);
}
