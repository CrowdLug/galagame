#include "MainWindow.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFrame>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QPainter>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QStackedLayout>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(GameManager* gameManager, QWidget* parent)
    : QMainWindow(parent)
    , m_gameManager(gameManager)
    , m_view(new QGraphicsView(this))
    , m_resizeFitTimer(new QTimer(this))
    , m_stage(new QWidget(this))
    , m_overlay(new QWidget(m_stage)) {
    setWindowTitle(QStringLiteral("拯救塔菲喵"));

    auto* central = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->addWidget(m_stage, 1);

    setCentralWidget(central);

    auto* stageLayout = new QStackedLayout(m_stage);
    stageLayout->setStackingMode(QStackedLayout::StackAll);
    stageLayout->setContentsMargins(0, 0, 0, 0);
    stageLayout->addWidget(m_view);
    stageLayout->addWidget(m_overlay);
    m_stage->setLayout(stageLayout);

    m_view->setRenderHint(QPainter::Antialiasing, true);
    m_view->setFrameShape(QFrame::NoFrame);
    m_view->setFocusPolicy(Qt::StrongFocus);
    m_view->setMouseTracking(true);

    m_overlay->setAttribute(Qt::WA_StyledBackground, true);
    m_overlay->setStyleSheet(QStringLiteral("background: transparent;"));

    m_resizeFitTimer->setSingleShot(true);
    m_resizeFitTimer->setInterval(16);

    connect(m_resizeFitTimer, &QTimer::timeout, this, &MainWindow::applyPendingViewFit);

    if (m_gameManager != nullptr) {
        connect(m_gameManager, &GameManager::sceneReady, this, &MainWindow::onSceneReady);
        connect(m_gameManager, &GameManager::stateChanged, this, &MainWindow::onStateChanged);
    }

    qInfo() << "[MainWindow] Constructed"
            << "initialSize=" << size();
}

MainWindow::~MainWindow() {
    qInfo() << "[MainWindow] Destroyed";
}

void MainWindow::onSceneReady(QGraphicsScene* scene) {
    if (scene == nullptr) {
        qWarning() << "[MainWindow] Received null scene";
        return;
    }

    m_view->setScene(scene);
    m_view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    m_view->setFocus();

    qInfo() << "[MainWindow] Scene attached to view"
            << "sceneRect=" << scene->sceneRect()
            << "viewportSize=" << m_view->viewport()->size();
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);

    layoutOverlayWidgets();

    if (m_view == nullptr || m_view->scene() == nullptr) {
        return;
    }

    m_resizeFitTimer->start();

    qInfo() << "[MainWindow] Resize queued fitInView"
            << "windowSize=" << event->size()
            << "viewportSize=" << m_view->viewport()->size();
}

void MainWindow::mousePressEvent(QMouseEvent* event) {
    QMainWindow::mousePressEvent(event);

    if (m_view == nullptr || m_view->scene() == nullptr) {
        return;
    }

    QPoint viewPos = m_view->mapFromGlobal(event->globalPos());
    QPointF scenePos = m_view->mapToScene(viewPos);

    QGraphicsItem* item = m_view->scene()->itemAt(scenePos, QTransform());
    if (item != nullptr) {
        QString data = item->data(0).toString();
        if (data == QStringLiteral("StartGame")) {
            if (m_gameManager != nullptr && m_gameManager->currentState() == GameManager::GameState::Menu) {
                onStartGameClicked();
            }
        } else if (data == QStringLiteral("EndlessMode")) {
            if (m_gameManager != nullptr && m_gameManager->currentState() == GameManager::GameState::Menu) {
                onEndlessModeClicked();
            }
        } else if (data == QStringLiteral("QuitGame")) {
            if (m_gameManager != nullptr && m_gameManager->currentState() == GameManager::GameState::Menu) {
                onQuitGameClicked();
            }
        } else if (data == QStringLiteral("ReturnToMenu")) {
            if (m_gameManager != nullptr && m_gameManager->currentState() == GameManager::GameState::Result) {
                m_gameManager->requestTransition(GameManager::GameState::Menu, QStringLiteral("ReturnToMenu"));
            }
        }
    }
}

void MainWindow::onStateChanged(GameManager::GameState oldState,
                                GameManager::GameState newState,
                                const QString& reason) {
    qInfo() << "[MainWindow] State changed"
            << "old=" << static_cast<int>(oldState)
            << "new=" << static_cast<int>(newState)
            << "reason=" << reason;

    updateOverlayForState(newState);
}

void MainWindow::applyPendingViewFit() {
    if (m_view == nullptr || m_view->scene() == nullptr) {
        return;
    }

    m_view->fitInView(m_view->scene()->sceneRect(), Qt::KeepAspectRatio);
    layoutOverlayWidgets();

    qInfo() << "[MainWindow] Resize fitInView applied"
            << "viewportSize=" << m_view->viewport()->size();
}



void MainWindow::onStartGameClicked() {
    if (m_gameManager == nullptr) {
        return;
    }
    m_gameManager->setGameMode(GameMode::Normal);
    const bool ok = m_gameManager->requestTransition(GameManager::GameState::Combat, QStringLiteral("StartGameButton"));
    qInfo() << "[MainWindow] Start game requested" << "result=" << ok;
}

void MainWindow::onEndlessModeClicked() {
    if (m_gameManager == nullptr) {
        return;
    }
    m_gameManager->setGameMode(GameMode::Endless);
    const bool ok = m_gameManager->requestTransition(GameManager::GameState::Combat, QStringLiteral("EndlessModeButton"));
    qInfo() << "[MainWindow] Endless mode requested" << "result=" << ok;
}

void MainWindow::onQuitGameClicked() {
    qInfo() << "[MainWindow] Quit game clicked";
    QCoreApplication::quit();
}

void MainWindow::updateOverlayForState(GameManager::GameState state) {
    if (state == GameManager::GameState::Menu && m_view != nullptr) {
        m_view->setFocus();
    }
    layoutOverlayWidgets();
}

void MainWindow::layoutOverlayWidgets() {
    if (m_stage == nullptr || m_overlay == nullptr) {
        return;
    }

    m_overlay->setGeometry(m_stage->rect());

    const int w = m_stage->width();
    const int h = m_stage->height();

    const int sideMargin = 40;
    const int bottomMargin = 36;
}
