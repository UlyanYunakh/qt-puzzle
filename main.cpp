#include "Items/PuzzleCanvas/puzzlecanvas.h"
#include "Audio/puzzlesoundsystem.h"
#include "Config/puzzleconfig.h"
#include "Settings/puzzleapplicationsettings.h"
#include "SaveGame/puzzlesavegamesystem.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtQml/qqml.h>

int main(int argc, char* argv[])
{
    QGuiApplication application(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Puzzle"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("puzzle.app"));
    QCoreApplication::setApplicationName(QStringLiteral("Puzzle"));
    QCoreApplication::setApplicationVersion(QStringLiteral(PUZZLE_VERSION));

    const PuzzleConfig config = PuzzleConfigLoader::load(
        QStringLiteral(":/config/puzzle-config.json"));

    qmlRegisterType<PuzzleCanvas>("Puzzle.App", 1, 0, "PuzzleCanvas");

    QQmlApplicationEngine engine;
    PuzzleSoundSystem soundSystem(config.audio);
    PuzzleApplicationSettings applicationSettings(
        config.appearance.background,
        config.interaction.magnetRadiusRatio,
        config.interaction.minimumMagnetRadiusRatio,
        config.interaction.maximumMagnetRadiusRatio);
    PuzzleSaveGameSystem saveGameSystem;
    engine.rootContext()->setContextProperty(QStringLiteral("puzzleSoundSystem"),
                                             &soundSystem);
    engine.rootContext()->setContextProperty(QStringLiteral("applicationSettings"),
                                             &applicationSettings);
    engine.rootContext()->setContextProperty(QStringLiteral("saveGameSystem"),
                                             &saveGameSystem);
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
