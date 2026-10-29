#ifndef PUZZLEGEOMETRYGENERATOR_H
#define PUZZLEGEOMETRYGENERATOR_H

#include "puzzleshapeset.h"

#include <QPolygonF>
#include <QRectF>
#include <QSizeF>
#include <QVector>

struct PuzzleMeshVertex
{
    QPointF localPosition;
    QPointF textureUv;
};

struct PuzzlePieceGeometry
{
    QRectF localBounds;
    QPolygonF outline;
    QVector<PuzzleMeshVertex> vertices;
    QVector<quint16> indices;
};

class PuzzleGeometryGenerator
{
public:
    [[nodiscard]] static QVector<PuzzlePieceGeometry> generate(
        const PuzzleShapeSet& shapeSet,
        int columns,
        int rows,
        const QSizeF& pieceSize,
        quint32 seed);
};

#endif // PUZZLEGEOMETRYGENERATOR_H
