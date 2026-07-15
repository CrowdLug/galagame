#pragma once

#include <QQueue>
#include <QString>

class LightDirector final {
public:
    struct Metrics {
        double elapsedSeconds {0.0};
        int playerHp {100};
        int maxPlayerHp {100};
        int hitsTakenTotal {0};
        int killsTotal {0};
    };

    struct Decision {
        double spawnIntervalScale {1.0};
        double densityScale {1.0};
        bool allowElite {true};
        QString label;
    };

    LightDirector();

    void reset();
    Decision update(const Metrics& metrics);
    QString debugText() const;

private:
    struct HitSample {
        double t {0.0};
        int totalHits {0};
    };

    struct KillSample {
        double t {0.0};
        int totalKills {0};
    };

    static double clamp01(double v);
    static double lerp(double a, double b, double t);

    QQueue<HitSample> m_hitWindow;
    QQueue<KillSample> m_killWindow;

    Decision m_smoothed;
    QString m_debugText;
};
