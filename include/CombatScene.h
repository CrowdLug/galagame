#pragma once

#include <QElapsedTimer>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QTimer>

#include "GameManager.h"
#include "CombatLoop.h"

class QGraphicsSimpleTextItem;

class CombatScene final : public QGraphicsScene {
    Q_OBJECT

public:
    explicit CombatScene(GameMode gameMode, QObject* parent = nullptr);
    ~CombatScene() override;
    void setCombatModifiers(double enemyDensityBias,
                            double dropBias,
                            double specialEventChance);
    void setTacticalUpgrade(double fireRateMultiplier,
                            int penetrationBonus,
                            double damageReductionRatio,
                            const QString& title,
                            const QString& description);

signals:
    void combatFinished(bool victory,
                        double clearTimeSeconds,
                        int hitsTaken,
                        double accuracy,
                        int pickupsCollected,
                        int iFrameBlocks,
                        int enemiesKilled,
                        int totalEnemiesSpawned,
                        int shotsFired);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;

private slots:
    void onTick();
    void onCombatFinished(bool victory,
                          double clearTimeSeconds,
                          int hitsTaken,
                          double accuracy,
                          int pickupsCollected,
                          int iFrameBlocks,
                          int enemiesKilled,
                          int totalEnemiesSpawned,
                          int shotsFired);

private:
    void syncInputToLoop();
    void updateHud();

    CombatLoop m_loop;
    QTimer m_tickTimer;
    QElapsedTimer m_frameTimer;
    QGraphicsSimpleTextItem* m_hud {nullptr};
    QGraphicsRectItem* m_flashOverlay {nullptr};
    QString m_feedbackText;
    bool m_screenShakeEnabled {true};
    double m_flashAlpha {0.0};
    QPointF m_baseSceneTopLeft {0.0, 0.0};
    QPointF m_screenShakeOffset {0.0, 0.0};
    GameMode m_gameMode;

    bool m_up {false};
    bool m_down {false};
    bool m_left {false};
    bool m_right {false};
    bool m_isFiring {false};
    QPointF m_aimTarget;
};
