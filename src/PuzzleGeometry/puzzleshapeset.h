#ifndef PUZZLESHAPESET_H
#define PUZZLESHAPESET_H

#include <QPointF>
#include <QString>
#include <QVector>

struct PuzzleCubicBezier
{
    QPointF control1;
    QPointF control2;
    QPointF end;
};

struct PuzzleEdgeProfile
{
    QString id;
    int weight = 1;
    QVector<PuzzleCubicBezier> beziers;
};

struct PuzzleShapeSet
{
    int schemaVersion = 1;
    QString id;
    QString displayName;
    qreal tabDepth = 0.2;
    int samplesPerBezier = 5;
    QVector<PuzzleEdgeProfile> profiles;
};

#endif // PUZZLESHAPESET_H
