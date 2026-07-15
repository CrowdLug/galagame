#include <QApplication>
#include <QDebug>
#include <QTimer>

#include "GameManager.h"
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    qInfo() << "[main] QApplication created";

    GameManager gameManager;
    MainWindow mainWindow(&gameManager);

    QObject::connect(&gameManager,
                     &GameManager::initializationFailed,
                     &app,
                     [](int errorCode, const QString& errorMessage) {
                         qCritical() << "[main] initializationFailed"
                                     << "errorCode=" << errorCode
                                     << "errorMessage=" << errorMessage;
                     });

    mainWindow.resize(1280, 720);
    mainWindow.show();

    QTimer::singleShot(0, &gameManager, [&gameManager]() {
        const bool ok = gameManager.initialize();
        qInfo() << "[main] initialize() invoked after show" << "result=" << ok;
    });

    qInfo() << "[main] Main window shown, entering event loop";

    return app.exec();
}
