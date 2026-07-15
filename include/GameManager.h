#pragma once

#include <QObject>
#include <QString>

class SceneManager;
class CombatLoop;
class RunContext;
class ResourceManager;
class QGraphicsScene;

enum class GameMode {
    Normal,
    Endless
};

class GameManager final : public QObject {
    Q_OBJECT

public:
    enum class GameState {
        Menu,
        Combat,
        Result
    };

    enum class InitState {
        Idle,
        BootstrappingScene,
        SceneReady,
        Failed
    };

    explicit GameManager(QObject* parent = nullptr);
    ~GameManager() override;

    bool initialize();
    bool requestTransition(GameState nextState, const QString& reason);
    bool requestNextState(const QString& reason);
    GameState currentState() const;
    InitState initState() const;
    static QString stateName(GameState state);
#ifdef QT_DEBUG
    bool triggerDebugFailThenRetry();
#endif
    RunContext* runContext() const;
    void stopCombatMusic();
    void setGameMode(GameMode mode);
    GameMode gameMode() const;

signals:
    void requestSceneBootstrap();
    void requestSceneSwitch(const QString& sceneKey, const QString& reason);
    void requestCombatStart();
    void sceneReady(QGraphicsScene* scene);
    void stateChanged(GameManager::GameState oldState,
                      GameManager::GameState newState,
                      const QString& reason);
    void initializationFailed(int errorCode, const QString& errorMessage);

private slots:
    void onSceneBootstrapped(QGraphicsScene* scene);
    void onSceneSwitched(QGraphicsScene* scene, const QString& sceneKey);
    void onCombatLoopStarted();
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
    bool isTransitionAllowed(GameState from, GameState to, QString* denyReason) const;
    void onExit(GameState oldState);
    void onEnter(GameState newState);
    static const char* toString(GameState state);
    static const char* toString(InitState state);
    static QString sceneKeyForState(GameState state);
    bool canInitializeNow() const;
    void resetForRetry();

    SceneManager* m_sceneManager {nullptr};
    CombatLoop* m_combatLoop {nullptr};
    RunContext* m_runContext {nullptr};
    ResourceManager* m_resourceManager {nullptr};

    GameMode m_gameMode {GameMode::Normal};
    InitState m_initState {InitState::Idle};
    GameState m_currentState {GameState::Menu};

    bool m_initializeCalled {false};
    bool m_isTransitioning {false};
};
