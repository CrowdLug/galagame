#pragma once

#include <QMainWindow>

#include "GameManager.h"

class QGraphicsScene;
class QGraphicsView;
class QResizeEvent;
class QMouseEvent;
class QTimer;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(GameManager* gameManager, QWidget* parent = nullptr);
    ~MainWindow() override;

public slots:
    void onSceneReady(QGraphicsScene* scene);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private slots:
    void onStateChanged(GameManager::GameState oldState,
                        GameManager::GameState newState,
                        const QString& reason);
    void applyPendingViewFit();
    void onStartGameClicked();
    void onEndlessModeClicked();
    void onQuitGameClicked();

private:
    void updateOverlayForState(GameManager::GameState state);
    void layoutOverlayWidgets();

    GameManager* m_gameManager {nullptr};
    QGraphicsView* m_view {nullptr};
    QTimer* m_resizeFitTimer {nullptr};

    QWidget* m_stage {nullptr};
    QWidget* m_overlay {nullptr};
};
