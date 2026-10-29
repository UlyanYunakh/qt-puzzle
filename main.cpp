#include "Items/PuzzleCanvas/puzzlecanvas.h"
#include "Config/puzzleconfig.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtQml/qqml.h>

int main(int argc, char* argv[])
{
    QGuiApplication application(argc, argv);

    const PuzzleConfig config = PuzzleConfigLoader::load(
        QStringLiteral(":/config/puzzle-config.json"));

    qmlRegisterType<PuzzleCanvas>("Puzzle.App", 1, 0, "PuzzleCanvas");

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("initialWindowWidth"),
                                             config.window.width);
    engine.rootContext()->setContextProperty(QStringLiteral("initialWindowHeight"),
                                             config.window.height);
    engine.rootContext()->setContextProperty(QStringLiteral("initialWindowTitle"),
                                             config.window.title);
    engine.rootContext()->setContextProperty(QStringLiteral("appBackgroundColor"),
                                             config.appearance.background);
    engine.loadFromModule(QStringLiteral("Puzzle.UI"), QStringLiteral("Main"));
    if (engine.rootObjects().isEmpty())
        return -1;
    return application.exec();
}
