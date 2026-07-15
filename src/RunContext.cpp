#include "RunContext.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRandomGenerator>
#include <algorithm>
#include <QtGlobal>

namespace {
constexpr double kDefaultEnemyWeightBias = 1.0;
constexpr double kDefaultDropBias = 1.0;
constexpr double kDefaultSpecialEventChance = 0.10;
} // namespace

QVector<RunContext::TacticalUpgrade> RunContext::defaultUpgradePool() {
    return {
        TacticalUpgrade {QStringLiteral("rapid_fire"),
                         QStringLiteral("速射校准"),
                         QStringLiteral("射速 +10%"),
                         1.10,
                         0,
                         0.0},
        TacticalUpgrade {QStringLiteral("piercing_round"),
                         QStringLiteral("穿透弹头"),
                         QStringLiteral("穿透 +1"),
                         1.0,
                         1,
                         0.0},
        TacticalUpgrade {QStringLiteral("impact_guard"),
                         QStringLiteral("冲击护层"),
                         QStringLiteral("受击减伤 +8%"),
                         1.0,
                         0,
                         0.08},
    };
}

RunContext::RunContext(QObject* parent)
    : QObject(parent) {
    qInfo() << "[RunContext] Constructed";
}

RunContext::~RunContext() {
    qInfo() << "[RunContext] Destroyed"
            << "runPrepared=" << m_isPrepared
            << "runId=" << m_runId;
}

void RunContext::markRunPrepared(const QString& runId) {
    resetForNewRun(runId);
}

void RunContext::reset() {
    resetForNewRun(QString());
}

void RunContext::resetForNewRun(const QString& runId) {
    m_runId = runId;
    m_isPrepared = !m_runId.isEmpty();

    m_combatModifiers.enemyWeightBias = kDefaultEnemyWeightBias;
    m_combatModifiers.dropBias = kDefaultDropBias;
    m_combatModifiers.specialEventChance = kDefaultSpecialEventChance;

    m_combatPerformance.accuracy = 0.0;
    m_combatPerformance.hitsTaken = 0;
    m_combatPerformance.clearTimeSeconds = 0.0;
    m_combatPerformance.pickupsCollected = 0;
    m_combatPerformance.iFrameBlocks = 0;

    m_activeUpgrade = TacticalUpgrade{};
    m_lastRolledChoices.clear();

    qInfo() << "[RunContext] resetForNewRun"
            << "runPrepared=" << m_isPrepared
            << "runId=" << m_runId;
}



void RunContext::applyCombatResult(double accuracy,
                                   int hitsTaken,
                                   double clearTimeSeconds,
                                   int pickupsCollected,
                                   int iFrameBlocks) {
    m_combatPerformance.accuracy = qBound(0.0, accuracy, 1.0);
    m_combatPerformance.hitsTaken = qMax(0, hitsTaken);
    m_combatPerformance.clearTimeSeconds = qMax(0.0, clearTimeSeconds);
    m_combatPerformance.pickupsCollected = qMax(0, pickupsCollected);
    m_combatPerformance.iFrameBlocks = qMax(0, iFrameBlocks);

    qInfo() << "[RunContext] applyCombatResult"
            << "accuracy=" << m_combatPerformance.accuracy
            << "hitsTaken=" << m_combatPerformance.hitsTaken
            << "clearTimeSeconds=" << m_combatPerformance.clearTimeSeconds
            << "pickupsCollected=" << m_combatPerformance.pickupsCollected
            << "iFrameBlocks=" << m_combatPerformance.iFrameBlocks;
}

QVector<RunContext::TacticalUpgrade> RunContext::rollUpgradeChoices(int count) {
    QVector<TacticalUpgrade> pool = defaultUpgradePool();
    if (pool.isEmpty() || count <= 0) {
        return {};
    }

    std::shuffle(pool.begin(),
                 pool.end(),
                 *QRandomGenerator::global());

    const int take = qBound(1, count, pool.size());
    QVector<TacticalUpgrade> out;
    out.reserve(take);
    for (int i = 0; i < take; ++i) {
        out.push_back(pool.at(i));
    }

    m_lastRolledChoices = out;
    return out;
}

bool RunContext::applyUpgradeById(const QString& upgradeId) {
    const QString key = upgradeId.trimmed().toLower();
    const QVector<TacticalUpgrade> pool = defaultUpgradePool();

    for (const TacticalUpgrade& item : pool) {
        if (item.id == key) {
            m_activeUpgrade = item;
            qInfo() << "[RunContext] applyUpgradeById"
                    << "id=" << item.id
                    << "title=" << item.title;
            return true;
        }
    }

    qWarning() << "[RunContext] applyUpgradeById failed, unknown id=" << upgradeId;
    return false;
}

void RunContext::clearRunScopedUpgrades() {
    m_activeUpgrade = TacticalUpgrade{};
    m_lastRolledChoices.clear();
}

QJsonObject RunContext::toJsonObject() const {
    QJsonObject root;
    root.insert(QStringLiteral("runId"), m_runId);
    root.insert(QStringLiteral("isPrepared"), m_isPrepared);

    QJsonObject modifiers;
    modifiers.insert(QStringLiteral("enemyWeightBias"), m_combatModifiers.enemyWeightBias);
    modifiers.insert(QStringLiteral("dropBias"), m_combatModifiers.dropBias);
    modifiers.insert(QStringLiteral("specialEventChance"), m_combatModifiers.specialEventChance);
    root.insert(QStringLiteral("combatModifiers"), modifiers);

    QJsonObject performance;
    performance.insert(QStringLiteral("accuracy"), m_combatPerformance.accuracy);
    performance.insert(QStringLiteral("hitsTaken"), m_combatPerformance.hitsTaken);
    performance.insert(QStringLiteral("clearTimeSeconds"), m_combatPerformance.clearTimeSeconds);
    performance.insert(QStringLiteral("pickupsCollected"), m_combatPerformance.pickupsCollected);
    performance.insert(QStringLiteral("iFrameBlocks"), m_combatPerformance.iFrameBlocks);
    root.insert(QStringLiteral("combatPerformance"), performance);

    QJsonObject upgrade;
    upgrade.insert(QStringLiteral("id"), m_activeUpgrade.id);
    upgrade.insert(QStringLiteral("title"), m_activeUpgrade.title);
    upgrade.insert(QStringLiteral("description"), m_activeUpgrade.description);
    upgrade.insert(QStringLiteral("fireRateMultiplier"), m_activeUpgrade.fireRateMultiplier);
    upgrade.insert(QStringLiteral("penetrationBonus"), m_activeUpgrade.penetrationBonus);
    upgrade.insert(QStringLiteral("damageReductionRatio"), m_activeUpgrade.damageReductionRatio);
    root.insert(QStringLiteral("activeUpgrade"), upgrade);

    return root;
}

QString RunContext::toJsonString() const {
    const QJsonDocument doc(toJsonObject());
    return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

bool RunContext::fromJsonObject(const QJsonObject& object, QString* errorMessage) {
    if (object.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("RunContext JSON is empty");
        }
        return false;
    }

    m_runId = object.value(QStringLiteral("runId")).toString();
    m_isPrepared = object.value(QStringLiteral("isPrepared")).toBool(!m_runId.isEmpty());

    const QJsonObject modifiers = object.value(QStringLiteral("combatModifiers")).toObject();
    m_combatModifiers.enemyWeightBias = modifiers.value(QStringLiteral("enemyWeightBias")).toDouble(kDefaultEnemyWeightBias);
    m_combatModifiers.dropBias = modifiers.value(QStringLiteral("dropBias")).toDouble(kDefaultDropBias);
    m_combatModifiers.specialEventChance = qBound(0.0,
                                                  modifiers.value(QStringLiteral("specialEventChance")).toDouble(kDefaultSpecialEventChance),
                                                  1.0);

    const QJsonObject performance = object.value(QStringLiteral("combatPerformance")).toObject();
    m_combatPerformance.accuracy = qBound(0.0,
                                          performance.value(QStringLiteral("accuracy")).toDouble(0.0),
                                          1.0);
    m_combatPerformance.hitsTaken = qMax(0, performance.value(QStringLiteral("hitsTaken")).toInt(0));
    m_combatPerformance.clearTimeSeconds = qMax(0.0,
                                                performance.value(QStringLiteral("clearTimeSeconds")).toDouble(0.0));
    m_combatPerformance.pickupsCollected = qMax(0,
                                                performance.value(QStringLiteral("pickupsCollected")).toInt(0));
    m_combatPerformance.iFrameBlocks = qMax(0,
                                            performance.value(QStringLiteral("iFrameBlocks")).toInt(0));

    const QJsonObject upgrade = object.value(QStringLiteral("activeUpgrade")).toObject();
    m_activeUpgrade.id = upgrade.value(QStringLiteral("id")).toString();
    m_activeUpgrade.title = upgrade.value(QStringLiteral("title")).toString();
    m_activeUpgrade.description = upgrade.value(QStringLiteral("description")).toString();
    m_activeUpgrade.fireRateMultiplier = qBound(0.5,
                                                upgrade.value(QStringLiteral("fireRateMultiplier")).toDouble(1.0),
                                                2.0);
    m_activeUpgrade.penetrationBonus = qMax(0,
                                            upgrade.value(QStringLiteral("penetrationBonus")).toInt(0));
    m_activeUpgrade.damageReductionRatio = qBound(0.0,
                                                  upgrade.value(QStringLiteral("damageReductionRatio")).toDouble(0.0),
                                                  0.5);

    qInfo() << "[RunContext] fromJsonObject loaded"
            << "runId=" << m_runId;

    if (errorMessage != nullptr) {
        errorMessage->clear();
    }

    return true;
}

QString RunContext::runId() const {
    return m_runId;
}

bool RunContext::isRunPrepared() const {
    return m_isPrepared;
}

const RunContext::CombatModifiers& RunContext::combatModifiers() const {
    return m_combatModifiers;
}

const RunContext::CombatPerformance& RunContext::combatPerformance() const {
    return m_combatPerformance;
}

const RunContext::TacticalUpgrade& RunContext::activeUpgrade() const {
    return m_activeUpgrade;
}
