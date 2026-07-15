#include "GameManager.h"

#include <QDateTime>
#include <QDebug>
#include <QGraphicsScene>
#include <QRandomGenerator>
#include <QTimer>

#include "CombatLoop.h"
#include "RunContext.h"
#include "SceneManager.h"
#include "ResourceManager.h"

GameManager::GameManager(QObject* parent)
    : QObject(parent)
    , m_sceneManager(new SceneManager(this))
    , m_combatLoop(new CombatLoop(this))
    , m_runContext(new RunContext(this))
    , m_resourceManager(new ResourceManager()) {
    m_sceneManager->setRunContext(m_runContext);
    m_sceneManager->setResourceManager(m_resourceManager);

    const auto signalConnectionType = Qt::UniqueConnection;

    connect(this,
            &GameManager::requestSceneBootstrap,
            m_sceneManager,
            &SceneManager::bootstrapScene,
            signalConnectionType);
    connect(this,
            &GameManager::requestSceneSwitch,
            m_sceneManager,
            &SceneManager::switchScene,
            signalConnectionType);
    connect(m_sceneManager,
            &SceneManager::sceneBootstrapped,
            this,
            &GameManager::onSceneBootstrapped,
            signalConnectionType);
    connect(m_sceneManager,
            &SceneManager::sceneSwitched,
            this,
            &GameManager::onSceneSwitched,
            signalConnectionType);
    connect(m_sceneManager,
            &SceneManager::combatResolvedSignal,
            this,
            &GameManager::onCombatResolved,
            signalConnectionType);

    connect(this,
            &GameManager::requestCombatStart,
            m_combatLoop,
            &CombatLoop::start,
            signalConnectionType);
    connect(m_combatLoop,
            &CombatLoop::combatLoopStarted,
            this,
            &GameManager::onCombatLoopStarted,
            signalConnectionType);

    qInfo() << "[GameManager] Constructed and module wiring complete"
            << "gameState=" << toString(m_currentState)
            << "initState=" << toString(m_initState);
}

GameManager::~GameManager() {
    qInfo() << "[GameManager] Destroyed"
            << "initializeCalled=" << m_initializeCalled
            << "gameState=" << toString(m_currentState)
            << "initState=" << toString(m_initState);
}

bool GameManager::initialize() {
    if (!canInitializeNow()) {
        qWarning() << "[GameManager] initialize() denied"
                   << "initState=" << toString(m_initState)
                   << "gameState=" << toString(m_currentState);
        return false;
    }

    if (m_initState == InitState::Failed) {
        qInfo() << "[GameManager] initialize() retry path entered";
        resetForRetry();
    }

    m_initializeCalled = true;
    m_initState = InitState::BootstrappingScene;

    const QString runId = QStringLiteral("run_%1_%2")
                              .arg(QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd_hhmmss_zzz")))
                              .arg(QRandomGenerator::global()->generate(), 8, 16, QLatin1Char('0'));
    m_runContext->markRunPrepared(runId);

    qInfo() << "[GameManager] Initialization started"
            << "initState=" << toString(m_initState)
            << "runPrepared=" << m_runContext->isRunPrepared()
            << "runId=" << m_runContext->runId();

    emit requestSceneBootstrap();
    return true;
}

bool GameManager::requestTransition(GameState nextState, const QString& reason) {
    if (!m_initializeCalled || m_initState != InitState::SceneReady) {
        qWarning() << "[GameManager] Transition denied: manager is not ready"
                   << "from=" << toString(m_currentState)
                   << "to=" << toString(nextState)
                   << "reason=" << reason
                   << "initState=" << toString(m_initState);
        return false;
    }

    if (m_isTransitioning) {
        qWarning() << "[GameManager] Transition denied: reentrancy guard active"
                   << "from=" << toString(m_currentState)
                   << "to=" << toString(nextState)
                   << "reason=" << reason;
        return false;
    }

    if (nextState == m_currentState) {
        qWarning() << "[GameManager] Transition denied: no-op transition"
                   << "state=" << toString(m_currentState)
                   << "reason=" << reason;
        return false;
    }

    QString denyReason;
    if (!isTransitionAllowed(m_currentState, nextState, &denyReason)) {
        qWarning() << "[GameManager] Transition denied: illegal state edge"
                   << "from=" << toString(m_currentState)
                   << "to=" << toString(nextState)
                   << "requestReason=" << reason
                   << "denyReason=" << denyReason;
        return false;
    }

    m_isTransitioning = true;

    const GameState oldState = m_currentState;
    onExit(oldState);
    m_currentState = nextState;
    onEnter(nextState);

    emit stateChanged(oldState, nextState, reason);
    emit requestSceneSwitch(sceneKeyForState(nextState), reason);

    if (nextState == GameState::Combat) {
        emit requestCombatStart();
    }

    m_isTransitioning = false;

    qInfo() << "[GameManager] Transition success"
            << "from=" << toString(oldState)
            << "to=" << toString(nextState)
            << "reason=" << reason;

    return true;
}

bool GameManager::requestNextState(const QString& reason) {
    GameState nextState = m_currentState;

    switch (m_currentState) {
    case GameState::Menu:
        nextState = GameState::Combat;
        break;
    case GameState::Combat:
        nextState = GameState::Result;
        break;
    case GameState::Result:
        nextState = GameState::Menu;
        break;
    default:
        break;
    }

    return requestTransition(nextState, reason);
}

GameManager::GameState GameManager::currentState() const {
    return m_currentState;
}

GameManager::InitState GameManager::initState() const {
    return m_initState;
}

void GameManager::onSceneBootstrapped(QGraphicsScene* scene) {
    if (scene == nullptr) {
        m_initState = InitState::Failed;

        const int errorCode = 1001;
        const QString errorMessage = QStringLiteral("Scene bootstrap returned null scene");

        qWarning() << "[GameManager] Initialization failed"
                   << "errorCode=" << errorCode
                   << "errorMessage=" << errorMessage
                   << "initState=" << toString(m_initState);

        emit initializationFailed(errorCode, errorMessage);
        return;
    }

    m_initState = InitState::SceneReady;

    qInfo() << "[GameManager] Scene bootstrap complete"
            << "sceneRect=" << scene->sceneRect()
            << "initState=" << toString(m_initState);

    emit sceneReady(scene);
}

void GameManager::onSceneSwitched(QGraphicsScene* scene, const QString& sceneKey) {
    if (scene == nullptr) {
        qWarning() << "[GameManager] onSceneSwitched received null scene"
                   << "sceneKey=" << sceneKey;
        return;
    }

    qInfo() << "[GameManager] onSceneSwitched"
            << "sceneKey=" << sceneKey
            << "sceneRect=" << scene->sceneRect();

    emit sceneReady(scene);
}

void GameManager::onCombatLoopStarted() {
    qInfo() << "[GameManager] Combat loop reported started"
            << "combatStarted=" << m_combatLoop->isStarted();
}

void GameManager::onCombatResolved(bool victory,
                                   double clearTimeSeconds,
                                   int hitsTaken,
                                   double accuracy,
                                   int pickupsCollected,
                                   int iFrameBlocks,
                                   int enemiesKilled,
                                   int totalEnemiesSpawned,
                                   int shotsFired) {
    Q_UNUSED(enemiesKilled);
    Q_UNUSED(totalEnemiesSpawned);
    Q_UNUSED(shotsFired);

    if (m_runContext != nullptr) {
        m_runContext->applyCombatResult(
            accuracy,
            hitsTaken,
            clearTimeSeconds,
            pickupsCollected,
            iFrameBlocks);
    }

    const QString reason = victory
                               ? QStringLiteral("CombatVictory")
                               : QStringLiteral("CombatDefeat");

    const bool moved = requestTransition(GameState::Result, reason);

    qInfo() << "[GameManager] onCombatResolved"
            << "victory=" << victory
            << "clearTimeSeconds=" << clearTimeSeconds
            << "hitsTaken=" << hitsTaken
            << "accuracy=" << accuracy
            << "pickupsCollected=" << pickupsCollected
            << "iFrameBlocks=" << iFrameBlocks
            << "transitioned=" << moved;
}

bool GameManager::isTransitionAllowed(GameState from, GameState to, QString* denyReason) const {
    const bool allowed = (from == GameState::Menu && to == GameState::Combat)
                         || (from == GameState::Combat && to == GameState::Result)
                         || (from == GameState::Result && to == GameState::Menu);

    if (!allowed && denyReason != nullptr) {
        *denyReason = QStringLiteral("Only fixed backbone flow is allowed");
    }

    return allowed;
}

void GameManager::onExit(GameState oldState) {
    qInfo() << "[GameManager] onExit" << toString(oldState);

    if (oldState == GameState::Combat) {
        stopCombatMusic();
    }
}

void GameManager::stopCombatMusic() {
    qInfo() << "[GameManager] Stopping combat music";
    if (m_combatLoop != nullptr) {
        m_combatLoop->stopMusic();
    }
}

void GameManager::setGameMode(GameMode mode) {
    m_gameMode = mode;
    if (m_sceneManager != nullptr) {
        m_sceneManager->setGameMode(mode);
    }
}

GameMode GameManager::gameMode() const {
    return m_gameMode;
}

void GameManager::onEnter(GameState newState) {
    qInfo() << "[GameManager] onEnter" << toString(newState);

    if (m_runContext == nullptr) {
        return;
    }

    const auto& performance = m_runContext->combatPerformance();
    qInfo() << "[GameManager] RunContext"
                << "accuracy=" << performance.accuracy
                << "hitsTaken=" << performance.hitsTaken
                << "clearTimeSeconds=" << performance.clearTimeSeconds;

    if (newState == GameState::Result) {
        qInfo().noquote() << "[GameManager] RunContext result snapshot\n" << m_runContext->toJsonString();
    }

    if (newState == GameState::Menu) {
        m_runContext->clearRunScopedUpgrades();
    }

    
}

const char* GameManager::toString(GameState state) {
    switch (state) {
    case GameState::Menu:
        return "Menu";
    case GameState::Combat:
        return "Combat";
    case GameState::Result:
        return "Result";
    }

    return "Unknown";
}

const char* GameManager::toString(InitState state) {
    switch (state) {
    case InitState::Idle:
        return "Idle";
    case InitState::BootstrappingScene:
        return "BootstrappingScene";
    case InitState::SceneReady:
        return "SceneReady";
    case InitState::Failed:
        return "Failed";
    }

    return "Unknown";
}

QString GameManager::stateName(GameState state) {
    return QString::fromLatin1(toString(state));
}

QString GameManager::sceneKeyForState(GameState state) {
    switch (state) {
    case GameState::Menu:
        return QStringLiteral("Menu");
    case GameState::Combat:
        return QStringLiteral("Combat");
    case GameState::Result:
        return QStringLiteral("Result");
    }

    return QStringLiteral("Menu");
}

RunContext* GameManager::runContext() const {
    return m_runContext;
}



#ifdef QT_DEBUG
bool GameManager::triggerDebugFailThenRetry() {
    qInfo() << "[GameManager] triggerDebugFailThenRetry() started";

    if (m_sceneManager == nullptr) {
        qWarning() << "[GameManager] triggerDebugFailThenRetry() failed: SceneManager is null";
        return false;
    }

    if (m_initState != InitState::Idle && m_initState != InitState::Failed) {
        qWarning() << "[GameManager] triggerDebugFailThenRetry() denied"
                   << "initState=" << toString(m_initState);
        return false;
    }

    m_sceneManager->setFailNextBootstrap(true);

    const bool firstAttempt = initialize();
    const bool firstFailedAsExpected = firstAttempt && (m_initState == InitState::Failed);

    qInfo() << "[GameManager] debug first attempt"
            << "attemptOk=" << firstAttempt
            << "failedAsExpected=" << firstFailedAsExpected
            << "initState=" << toString(m_initState);

    if (!firstFailedAsExpected) {
        qWarning() << "[GameManager] triggerDebugFailThenRetry() failed: first attempt did not fail as expected";
        return false;
    }

    const bool retryAttempt = initialize();
    const bool retrySucceeded = retryAttempt && (m_initState == InitState::SceneReady);

    qInfo() << "[GameManager] debug retry attempt"
            << "attemptOk=" << retryAttempt
            << "retrySucceeded=" << retrySucceeded
            << "initState=" << toString(m_initState);

    return retrySucceeded;
}
#endif

bool GameManager::canInitializeNow() const {
    return m_initState == InitState::Idle || m_initState == InitState::Failed;
}

void GameManager::resetForRetry() {
    m_combatLoop->reset();
    m_runContext->reset();
    m_currentState = GameState::Menu;
    m_isTransitioning = false;
    m_initializeCalled = false;

    qInfo() << "[GameManager] resetForRetry() completed"
            << "gameState=" << toString(m_currentState)
            << "initState=" << toString(m_initState);
}
