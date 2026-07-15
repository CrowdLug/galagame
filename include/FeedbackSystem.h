#pragma once

#include <QPointF>

class FeedbackSystem final {
public:
    struct Config {
        double hitStopNormalSeconds {0.025};
        double hitStopHeavySeconds {0.055};
        int heavyHitDamageThreshold {26};

        double hurtFlashSeconds {0.11};
        double hurtBlinkSeconds {0.45};

        double killFlashSeconds {0.09};
        double killShakeSeconds {0.10};

        double shakeAmplitudeLight {5.0};
        double shakeAmplitudeHeavy {10.0};
        double eventCooldownSeconds {0.015};

        bool screenShakeEnabled {true};
    };

    struct FrameEffects {
        double timeScale {1.0};
        double flashAlpha {0.0};
        double playerBlinkAlpha {1.0};
        QPointF screenShakeOffset {0.0, 0.0};
    };

    FeedbackSystem();

    void setConfig(const Config& config);
    const Config& config() const;

    void setScreenShakeEnabled(bool enabled);
    bool isScreenShakeEnabled() const;

    void onHit(int damage);
    void onHurt(int damage, const QPointF& direction);
    void onKill(int damage);

    FrameEffects update(double deltaTimeSeconds);
    bool isHitStopActive() const;

private:
    QPointF normalizedOrDefault(const QPointF& input) const;

    Config m_config;

    double m_eventCooldownRemaining {0.0};
    double m_hitStopRemaining {0.0};
    double m_hurtFlashRemaining {0.0};
    double m_hurtBlinkRemaining {0.0};
    double m_killFlashRemaining {0.0};
    double m_killShakeRemaining {0.0};

    double m_lastShakeAmplitude {0.0};
    QPointF m_lastHurtDirection {1.0, 0.0};
};
