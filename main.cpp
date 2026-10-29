#include "Items/PuzzleCanvas/puzzlecanvas.h"

#include <QGuiApplication>
#include <QQuickWindow>

int main(int argc, char* argv[])
{
    QGuiApplication application(argc, argv);

    QQuickWindow window;
    window.setTitle(QStringLiteral("Puzzle Canvas"));
    window.setColor(QColor(QStringLiteral("#20242b")));
    window.resize(1280, 800);

    auto* canvas = new PuzzleCanvas(window.contentItem());
    canvas->setParentItem(window.contentItem());
    canvas->setSize(window.size());

    QObject::connect(&window, &QQuickWindow::widthChanged,
                     canvas, [canvas, &window] { canvas->setWidth(window.width()); });
    QObject::connect(&window, &QQuickWindow::heightChanged,
                     canvas, [canvas, &window] { canvas->setHeight(window.height()); });

    window.show();
    return application.exec();
}
