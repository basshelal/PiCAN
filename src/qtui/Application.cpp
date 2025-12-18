#include "Application.hpp"

#include <QDebug>
#include <QDirIterator>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <iostream>

#include "Controller.hpp"

int
PiCAN::QtUI::Application::run(int argc, char** argv) {
    const QGuiApplication app(argc, argv);

    StopwatchBridge stopwatchBackend{};

    QQmlApplicationEngine engine{};

    // --- THE KEY STEP ---
    // Take the C++ object and expose it to QML under the name "backend".
    // This allows Main.qml to write `text: backend.elapsedTimeString`
    engine.rootContext()->setContextProperty("backend", &stopwatchBackend);
    // --------------------

    const QUrl url("qrc:qml/Application.qml");
    std::cout << url.path().toStdString() << std::endl;

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [url](QObject* obj, const QUrl& objUrl) {
            if (!obj && url == objUrl) {
                QCoreApplication::exit(-1);
            }
        },
        Qt::ConnectionType::QueuedConnection
    );

    engine.load("qtui/qml/Application.qml");

    return app.exec();
}
