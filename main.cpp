#include "Items/PuzzleCanvas/puzzlecanvas.h"
#include "Audio/puzzlesoundsystem.h"
#include "Config/puzzleconfig.h"
#include "Settings/puzzleapplicationsettings.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtQml/qqml.h>

int main(int argc, char* argv[])
{
    QGuiApplication application(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Puzzle"));
    QCoreApplication::setApplicationName(QStringLiteral("Puzzle"));

    const PuzzleConfig config = PuzzleConfigLoader::load(
        QStringLiteral(":/config/puzzle-config.json"));

    qmlRegisterType<PuzzleCanvas>("Puzzle.App", 1, 0, "PuzzleCanvas");

    QQmlApplicationEngine engine;
    PuzzleSoundSystem soundSystem(config.audio);
    PuzzleApplicationSettings applicationSettings(config.appearance.background);
    engine.rootContext()->setContextProperty(QStringLiteral("puzzleSoundSystem"),
                                             &soundSystem);
    engine.rootContext()->setContextProperty(QStringLiteral("applicationSettings"),
                                             &applicationSettings);
    engine.rootContext()->setContextProperty(QStringLiteral("initialWindowWidth"),
                                             config.window.width);
    engine.rootContext()->setContextProperty(QStringLiteral("initialWindowHeight"),
                                             config.window.height);
    engine.rootContext()->setContextProperty(QStringLiteral("initialWindowTitle"),
                                             config.window.title);
    engine.loadFromModule(QStringLiteral("Puzzle.UI"), QStringLiteral("Main"));
    if (engine.rootObjects().isEmpty())
        return -1;
    return application.exec();
}
