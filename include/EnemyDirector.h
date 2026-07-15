#pragma once

#include <QPointF>
#include <QRectF>
#include <QString>

#include <vector>

#include "EnemyEntity.h"

class EnemyDirector final {
public:
    struct SpawnEvent {
        double timeSeconds {0.0};
        bool useTriggerZone {false};
        QRectF triggerZone;
        int meleeCount {0};
        int rangedCount {0};
        bool elite {false};
        bool triggered {false};
    };

    struct SpawnRequest {
        int meleeCount {0};
        int rangedCount {0};
    };

    struct RuntimeTuning {
        double spawnIntervalScale {1.0};
        double densityScale {1.0};
        bool allowElite {true};
    };

    struct Config {
        int maxOnScreenEnemies {22};
        std::vector<SpawnEvent> events;
    };

    explicit EnemyDirector(Config config);

    static Config loadConfigFromJson(const QString& path, const Config& fallback);

    void reset();
    void setPaused(bool paused);
    bool isPaused() const;
    void setRuntimeTuning(const RuntimeTuning& tuning);
    RuntimeTuning getRuntimeTuning() const;

    SpawnRequest update(double elapsedSeconds,
                        const QPointF& playerPos,
                        int currentOnScreenEnemies);

    int maxOnScreenEnemies() const;
    bool hasPendingEvents() const;
    const Config& getConfig() const;

private:
    static bool containsPoint(const QRectF& zone, const QPointF& point);

    Config m_config;
    bool m_paused {false};
    RuntimeTuning m_runtimeTuning;
    double m_lastSpawnedEventTime {0.0};
};
