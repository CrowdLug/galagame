#pragma once

#include <QObject>
#include <QJsonObject>
#include <QString>
#include <QVector>

class RunContext final : public QObject {
    Q_OBJECT

public:
    struct CombatModifiers {
        double enemyWeightBias {1.0};
        double dropBias {1.0};
        double specialEventChance {0.10};
    };

    struct CombatPerformance {
        double accuracy {0.0};
        int hitsTaken {0};
        double clearTimeSeconds {0.0};
        int pickupsCollected {0};
        int iFrameBlocks {0};
    };

    struct TacticalUpgrade {
        QString id;
        QString title;
        QString description;
        double fireRateMultiplier {1.0};
        int penetrationBonus {0};
        double damageReductionRatio {0.0};
    };

    explicit RunContext(QObject* parent = nullptr);
    ~RunContext() override;

    // Compatibility APIs used by existing steps.
    void markRunPrepared(const QString& runId);
    void reset();

    // Step 4 required APIs.
    void resetForNewRun(const QString& runId);
    void applyCombatResult(double accuracy,
                           int hitsTaken,
                           double clearTimeSeconds,
                           int pickupsCollected = 0,
                           int iFrameBlocks = 0);

    QVector<TacticalUpgrade> rollUpgradeChoices(int count = 3);
    bool applyUpgradeById(const QString& upgradeId);
    void clearRunScopedUpgrades();

    // JSON serialization for debug/save snapshots.
    QJsonObject toJsonObject() const;
    QString toJsonString() const;
    bool fromJsonObject(const QJsonObject& object, QString* errorMessage = nullptr);

    QString runId() const;
    bool isRunPrepared() const;
    const CombatModifiers& combatModifiers() const;
    const CombatPerformance& combatPerformance() const;
    const TacticalUpgrade& activeUpgrade() const;

private:
    QString m_runId;
    bool m_isPrepared {false};

    CombatModifiers m_combatModifiers;
    CombatPerformance m_combatPerformance;
    TacticalUpgrade m_activeUpgrade;
    QVector<TacticalUpgrade> m_lastRolledChoices;

    static QVector<TacticalUpgrade> defaultUpgradePool();
};
