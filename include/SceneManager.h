#pragma once

#include <QObject>
#include <QPointer>
#include <QString>

#include "GameManager.h"

class QGraphicsScene;
class TransitionController;
class RunContext;
class ResourceManager;

class SceneManager final : public QObject {
    Q_OBJECT

public:
    explicit SceneManager(QObject* parent = nullptr);
    ~SceneManager() override;

    void setFailNextBootstrap(bool shouldFail);
    void setRunContext(RunContext* runContext);
    void setResourceManager(ResourceManager* resourceManager);
    void setGameMode(GameMode mode);
    GameMode gameMode() const;

signals:
    void sceneBootstrapped(QGraphicsScene* scene);
    void sceneSwitched(QGraphicsScene* scene, const QString& sceneKey);
    void combatResolvedSignal(bool victory,
                              double clearTimeSeconds,
                              int hitsTaken,
                              double accuracy,
                              int pickupsCollected,
                              int iFrameBlocks,
                              int enemiesKilled,
                              int totalEnemiesSpawned,
                              int shotsFired);

public slots:
    void bootstrapScene();
    void switchScene(const QString& sceneKey, const QString& reason);
    void onCombatResolved(bool victory,
                          double clearTimeSeconds,
                          int hitsTaken,
                          double accuracy,
                          int pickupsCollected,
                          int iFrameBlocks,
                          int enemiesKilled,
                          int totalEnemiesSpawned,
                          int shotsFired);

private:
    QGraphicsScene* createSceneForKey(const QString& sceneKey);
    void scheduleCurrentSceneDestroy();

    QPointer<QGraphicsScene> m_scene;
    TransitionController* m_transitionController {nullptr};
    RunContext* m_runContext {nullptr};
    ResourceManager* m_resourceManager {nullptr};

    GameMode m_gameMode {GameMode::Normal};
    bool m_failNextBootstrap {false};
    int m_createdCount {0};
    int m_destroyedCount {0};

    bool m_combatVictory {false};
    int m_combatEnemiesKilled {0};
    int m_combatTotalEnemies {0};
    double m_combatClearTime {0.0};
    int m_combatShotsFired {0};
};
