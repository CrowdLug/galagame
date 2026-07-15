#include "EnemyDirector.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtMath>

EnemyDirector::EnemyDirector(Config config)
    : m_config(std::move(config)) {}

EnemyDirector::Config EnemyDirector::loadConfigFromJson(const QString& path, const Config& fallback) {
    Config cfg = fallback;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[EnemyDirector] Failed to open stage config:" << path;
        return cfg;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        qWarning() << "[EnemyDirector] Invalid stage json root:" << path;
        return cfg;
    }

    const QJsonObject root = doc.object();
    if (root.contains("maxOnScreenEnemies") && root.value("maxOnScreenEnemies").isDouble()) {
        cfg.maxOnScreenEnemies = qMax(1, root.value("maxOnScreenEnemies").toInt(cfg.maxOnScreenEnemies));
    }

    cfg.events.clear();
    const QJsonValue eventsValue = root.value("events");
    if (!eventsValue.isArray()) {
        qWarning() << "[EnemyDirector] Missing/invalid events array, using fallback events";
        return cfg;
    }

    const QJsonArray events = eventsValue.toArray();
    int eventIndex = 0;
    for (const QJsonValue& v : events) {
        if (!v.isObject()) {
            qWarning() << "[EnemyDirector] Skip non-object event at index" << eventIndex;
            ++eventIndex;
            continue;
        }

        const QJsonObject obj = v.toObject();
        SpawnEvent e;

        const bool hasTime = obj.contains("timeSeconds") && obj.value("timeSeconds").isDouble();
        e.timeSeconds = hasTime ? qMax(0.0, obj.value("timeSeconds").toDouble(0.0)) : 0.0;
        e.meleeCount = qMax(0, obj.value("meleeCount").toInt(0));
        e.rangedCount = qMax(0, obj.value("rangedCount").toInt(0));
        e.elite = obj.value("elite").toBool(false);

        if (obj.contains("triggerZone")) {
            if (!obj.value("triggerZone").isObject()) {
                qWarning() << "[EnemyDirector] Invalid triggerZone at index" << eventIndex;
            } else {
                const QJsonObject z = obj.value("triggerZone").toObject();
                const double x = z.value("x").toDouble(0.0);
                const double y = z.value("y").toDouble(0.0);
                const double w = z.value("w").toDouble(0.0);
                const double h = z.value("h").toDouble(0.0);
                if (w > 0.0 && h > 0.0) {
                    e.useTriggerZone = true;
                    e.triggerZone = QRectF(x, y, w, h);
                } else {
                    qWarning() << "[EnemyDirector] Invalid triggerZone size at index" << eventIndex;
                }
            }
        }

        if (!e.useTriggerZone && !hasTime) {
            qWarning() << "[EnemyDirector] Skip event without timeSeconds or valid triggerZone at index" << eventIndex;
            ++eventIndex;
            continue;
        }

        if (e.meleeCount + e.rangedCount <= 0) {
            qWarning() << "[EnemyDirector] Skip empty event with zero enemies at index" << eventIndex;
            ++eventIndex;
            continue;
        }

        cfg.events.push_back(e);
        ++eventIndex;
    }

    if (cfg.events.empty()) {
        qWarning() << "[EnemyDirector] No valid events loaded, using fallback events";
        return fallback;
    }

    return cfg;
}

void EnemyDirector::reset() {
    for (SpawnEvent& e : m_config.events) {
        e.triggered = false;
    }
    m_lastSpawnedEventTime = 0.0;
}

void EnemyDirector::setPaused(bool paused) {
    m_paused = paused;
}

bool EnemyDirector::isPaused() const {
    return m_paused;
}

void EnemyDirector::setRuntimeTuning(const RuntimeTuning& tuning) {
    m_runtimeTuning.spawnIntervalScale = qBound(0.7, tuning.spawnIntervalScale, 1.35);
    m_runtimeTuning.densityScale = qBound(0.70, tuning.densityScale, 1.30);
    m_runtimeTuning.allowElite = tuning.allowElite;
}

EnemyDirector::RuntimeTuning EnemyDirector::getRuntimeTuning() const {
    return m_runtimeTuning;
}

EnemyDirector::SpawnRequest EnemyDirector::update(double elapsedSeconds,
                                                  const QPointF& playerPos,
                                                  int currentOnScreenEnemies) {
    SpawnRequest req;
    if (m_paused || currentOnScreenEnemies >= m_config.maxOnScreenEnemies) {
        return req;
    }

    int capacity = m_config.maxOnScreenEnemies - currentOnScreenEnemies;

    for (SpawnEvent& e : m_config.events) {
        if (e.triggered) {
            continue;
        }

        const double gatedTime = e.timeSeconds * m_runtimeTuning.spawnIntervalScale;
        const bool timeReady = elapsedSeconds >= gatedTime;
        const bool zoneReady = !e.useTriggerZone || containsPoint(e.triggerZone, playerPos);

        if (!timeReady || !zoneReady) {
            continue;
        }

        if (e.elite && !m_runtimeTuning.allowElite) {
            continue;
        }

        const double minGap = 0.35 * m_runtimeTuning.spawnIntervalScale;
        if ((elapsedSeconds - m_lastSpawnedEventTime) < minGap) {
            break;
        }

        int wantMelee = qMax(0, static_cast<int>(qRound(static_cast<double>(e.meleeCount) * m_runtimeTuning.densityScale)));
        int wantRanged = qMax(0, static_cast<int>(qRound(static_cast<double>(e.rangedCount) * m_runtimeTuning.densityScale)));
        if (e.meleeCount > 0 && wantMelee <= 0) {
            wantMelee = 1;
        }
        if (e.rangedCount > 0 && wantRanged <= 0) {
            wantRanged = 1;
        }

        const int totalWant = wantMelee + wantRanged;
        if (totalWant <= 0) {
            e.triggered = true;
            continue;
        }

        if (capacity <= 0) {
            break;
        }

        if (totalWant > capacity) {
            const double ratio = static_cast<double>(capacity) / static_cast<double>(totalWant);
            wantMelee = qMax(0, static_cast<int>(wantMelee * ratio));
            wantRanged = qMax(0, static_cast<int>(wantRanged * ratio));
            while (wantMelee + wantRanged < capacity) {
                if (wantMelee < totalWant && wantMelee < e.meleeCount + 2) {
                    ++wantMelee;
                } else if (wantRanged < totalWant && wantRanged < e.rangedCount + 2) {
                    ++wantRanged;
                } else {
                    break;
                }
            }
        }

        req.meleeCount += wantMelee;
        req.rangedCount += wantRanged;
        capacity -= (wantMelee + wantRanged);
        e.triggered = true;
        m_lastSpawnedEventTime = elapsedSeconds;

        qInfo() << "[EnemyDirector] Spawn event triggered"
                << "t=" << elapsedSeconds
                << "melee=" << wantMelee
                << "ranged=" << wantRanged
                << "elite=" << e.elite;

        if (capacity <= 0) {
            break;
        }
    }

    return req;
}

int EnemyDirector::maxOnScreenEnemies() const {
    return m_config.maxOnScreenEnemies;
}

bool EnemyDirector::hasPendingEvents() const {
    for (const SpawnEvent& e : m_config.events) {
        if (!e.triggered) {
            return true;
        }
    }

    return false;
}

const EnemyDirector::Config& EnemyDirector::getConfig() const {
    return m_config;
}

bool EnemyDirector::containsPoint(const QRectF& zone, const QPointF& point) {
    return zone.contains(point);
}
