#include "Items/PuzzleCanvas/puzzlecanvas.h"
#include "Config/puzzleconfig.h"

#include <QGuiApplication>
#include <QQuickWindow>

int main(int argc, char* argv[])
{
    QGuiApplication application(argc, argv);

    const PuzzleConfig config = PuzzleConfigLoader::load(
        QStringLiteral(":/config/puzzle-config.json"));

    QQuickWindow window;
    window.setTitle(config.window.title);
    window.setColor(config.appearance.background);
    window.resize(config.window.width, config.window.height);

    auto* canvas = new PuzzleCanvas(config, window.contentItem());
    canvas->setParentItem(window.contentItem());
    canvas->setSize(window.size());

    QObject::connect(&window, &QQuickWindow::widthChanged,
                     canvas, [canvas, &window] { canvas->setWidth(window.width()); });
    QObject::connect(&window, &QQuickWindow::heightChanged,
                     canvas, [canvas, &window] { canvas->setHeight(window.height()); });

    window.show();
    return application.exec();
}
