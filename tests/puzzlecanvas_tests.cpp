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
    return 0;
}
