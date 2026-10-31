#include "Items/PuzzleCanvas/puzzlecanvas.h"

#include <QCoreApplication>

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    PuzzleCanvas canvas;
    if (!canvas.startPuzzleWithPieceCount({}, 2000))
        return 1;
    canvas.setSize(QSizeF(1280.0, 800.0));
    if (canvas.maximumZoom() <= 10.0)
        return 2;
    if (canvas.sourceImage().isEmpty())
        return 3;
    const QVariantMap savedState = canvas.saveState();
    PuzzleCanvas restoredCanvas;
    restoredCanvas.setSize(QSizeF(1280.0, 800.0));
    if (!restoredCanvas.restoreState(savedState))
        return 4;
    if (restoredCanvas.saveState().value(QStringLiteral("pieces")).toList().size()
        != savedState.value(QStringLiteral("pieces")).toList().size())
        return 5;
    restoredCanvas.setMagnetism(0.0);
    if (!qFuzzyIsNull(restoredCanvas.magnetism()))
        return 6;
    return 0;
}
