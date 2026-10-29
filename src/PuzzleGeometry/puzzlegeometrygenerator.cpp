#include "puzzlegeometrygenerator.h"

#include <QRandomGenerator>
#include <QLineF>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace
{
    using Edge = QPolygonF;

    qreal cross(const QPointF& a, const QPointF& b, const QPointF& c)
    {
        return (b.x() - a.x()) * (c.y() - a.y())
               - (b.y() - a.y()) * (c.x() - a.x());
    }

    QPointF cubicPoint(const QPointF& start, const PuzzleCubicBezier& curve, qreal t)
    {
        const qreal inverse = 1.0 - t;
        return inverse * inverse * inverse * start
               + 3.0 * inverse * inverse * t * curve.control1
               + 3.0 * inverse * t * t * curve.control2
               + t * t * t * curve.end;
    }

    const PuzzleEdgeProfile& chooseProfile(const PuzzleShapeSet& shapeSet,
                                           QRandomGenerator& random)
    {
        int totalWeight = 0;
        for (const PuzzleEdgeProfile& profile : shapeSet.profiles)
            totalWeight += profile.weight;

        int choice = random.bounded(totalWeight);
        for (const PuzzleEdgeProfile& profile : shapeSet.profiles)
        {
            choice -= profile.weight;
            if (choice < 0)
                return profile;
        }
        return shapeSet.profiles.constLast();
    }

    Edge straightEdge(const QPointF& start, const QPointF& end)
    {
        return {start, end};
    }

    Edge shapedEdge(const QPointF& start,
                    const QPointF& end,
                    const PuzzleShapeSet& shapeSet,
                    const PuzzleEdgeProfile& profile,
                    qreal depth,
                    qreal direction)
    {
        const QPointF delta = end - start;
        const qreal length = std::hypot(delta.x(), delta.y());
        const QPointF tangent = delta / length;
        const QPointF normal(-tangent.y(), tangent.x());
        Edge result;
        result.push_back(start);
        QPointF curveStart(0.0, 0.0);
        for (const PuzzleCubicBezier& curve : profile.beziers)
        {
            for (int sample = 1; sample <= shapeSet.samplesPerBezier; ++sample)
            {
                const QPointF normalized = cubicPoint(
                    curveStart, curve, qreal(sample) / shapeSet.samplesPerBezier);
                result.push_back(start + tangent * (normalized.x() * length)
                                 + normal * (normalized.y() * depth * direction));
            }
            curveStart = curve.end;
        }
        result.last() = end;
        return result;
    }

    void appendEdge(QPolygonF& polygon, const Edge& edge, bool reverse)
    {
        if (!reverse)
        {
            for (int i = polygon.isEmpty() ? 0 : 1; i < edge.size(); ++i)
                polygon.push_back(edge.at(i));
        }
        else
        {
            for (int i = edge.size() - 1 - (polygon.isEmpty() ? 0 : 1); i >= 0; --i)
                polygon.push_back(edge.at(i));
        }
    }

    QPolygonF cleanPolygon(QPolygonF polygon)
    {
        constexpr qreal epsilon = 0.0001;
        if (polygon.size() > 1
            && QLineF(polygon.first(), polygon.last()).length() < epsilon)
            polygon.removeLast();

        bool removed = true;
        while (removed && polygon.size() > 3)
        {
            removed = false;
            for (int i = 0; i < polygon.size(); ++i)
            {
                const QPointF& previous = polygon.at((i - 1 + polygon.size()) % polygon.size());
                const QPointF& current = polygon.at(i);
                const QPointF& next = polygon.at((i + 1) % polygon.size());
                if (QLineF(previous, current).length() < epsilon
                    || std::abs(cross(previous, current, next)) < epsilon)
                {
                    polygon.removeAt(i);
                    removed = true;
                    break;
                }
            }
        }
        return polygon;
    }

    qreal signedArea(const QPolygonF& polygon)
    {
        qreal area = 0.0;
        for (int i = 0; i < polygon.size(); ++i)
        {
            const QPointF& a = polygon.at(i);
            const QPointF& b = polygon.at((i + 1) % polygon.size());
            area += a.x() * b.y() - b.x() * a.y();
        }
        return area * 0.5;
    }

    bool pointInTriangle(const QPointF& point,
                         const QPointF& a,
                         const QPointF& b,
                         const QPointF& c)
    {
        constexpr qreal epsilon = 0.000001;
        return cross(a, b, point) >= -epsilon
               && cross(b, c, point) >= -epsilon
               && cross(c, a, point) >= -epsilon;
    }

    QVector<quint16> triangulate(const QPolygonF& polygon)
    {
        QVector<int> remaining(polygon.size());
        std::iota(remaining.begin(), remaining.end(), 0);
        if (signedArea(polygon) < 0.0)
            std::reverse(remaining.begin(), remaining.end());

        QVector<quint16> indices;
        indices.reserve((polygon.size() - 2) * 3);
        while (remaining.size() > 3)
        {
            bool foundEar = false;
            for (int i = 0; i < remaining.size(); ++i)
            {
                const int previous = remaining.at((i - 1 + remaining.size()) % remaining.size());
                const int current = remaining.at(i);
                const int next = remaining.at((i + 1) % remaining.size());
                if (cross(polygon.at(previous), polygon.at(current), polygon.at(next)) <= 0.000001)
                    continue;

                bool containsVertex = false;
                for (const int candidate : std::as_const(remaining))
                {
                    if (candidate != previous && candidate != current && candidate != next
                        && pointInTriangle(polygon.at(candidate), polygon.at(previous),
                                           polygon.at(current), polygon.at(next)))
                    {
                        containsVertex = true;
                        break;
                    }
                }
                if (containsVertex)
                    continue;

                indices += {quint16(previous), quint16(current), quint16(next)};
                remaining.removeAt(i);
                foundEar = true;
                break;
            }

            if (!foundEar)
                throw std::runtime_error("Unable to triangulate a puzzle piece outline");
        }
        indices += {quint16(remaining.at(0)), quint16(remaining.at(1)),
                    quint16(remaining.at(2))};
        return indices;
    }
}

QVector<PuzzlePieceGeometry> PuzzleGeometryGenerator::generate(
    const PuzzleShapeSet& shapeSet,
    int columns,
    int rows,
    const QSizeF& pieceSize,
    quint32 seed)
{
    if (columns < 1 || rows < 1 || pieceSize.isEmpty())
        return {};

    QRandomGenerator random(seed);
    const qreal tabDepth = shapeSet.tabDepth
                           * std::min(pieceSize.width(), pieceSize.height());
    QVector<QVector<Edge>> horizontal(rows + 1, QVector<Edge>(columns));
    QVector<QVector<Edge>> vertical(rows, QVector<Edge>(columns + 1));

    for (int row = 0; row <= rows; ++row)
    {
        for (int column = 0; column < columns; ++column)
        {
            const QPointF start(column * pieceSize.width(), row * pieceSize.height());
            const QPointF end((column + 1) * pieceSize.width(), row * pieceSize.height());
            if (row == 0 || row == rows)
                horizontal[row][column] = straightEdge(start, end);
            else
            {
                horizontal[row][column] = shapedEdge(
                    start, end, shapeSet, chooseProfile(shapeSet, random), tabDepth,
                    random.bounded(2) == 0 ? -1.0 : 1.0);
            }
        }
    }

    for (int row = 0; row < rows; ++row)
    {
        for (int column = 0; column <= columns; ++column)
        {
            const QPointF start(column * pieceSize.width(), row * pieceSize.height());
            const QPointF end(column * pieceSize.width(), (row + 1) * pieceSize.height());
            if (column == 0 || column == columns)
                vertical[row][column] = straightEdge(start, end);
            else
            {
                vertical[row][column] = shapedEdge(
                    start, end, shapeSet, chooseProfile(shapeSet, random), tabDepth,
                    random.bounded(2) == 0 ? -1.0 : 1.0);
            }
        }
    }

    QVector<PuzzlePieceGeometry> pieces;
    pieces.reserve(columns * rows);
    const QSizeF completeSize(columns * pieceSize.width(), rows * pieceSize.height());
    for (int row = 0; row < rows; ++row)
    {
        for (int column = 0; column < columns; ++column)
        {
            QPolygonF globalOutline;
            appendEdge(globalOutline, horizontal.at(row).at(column), false);
            appendEdge(globalOutline, vertical.at(row).at(column + 1), false);
            appendEdge(globalOutline, horizontal.at(row + 1).at(column), true);
            appendEdge(globalOutline, vertical.at(row).at(column), true);
            globalOutline = cleanPolygon(std::move(globalOutline));

            PuzzlePieceGeometry piece;
            const QPointF cellOrigin(column * pieceSize.width(), row * pieceSize.height());
            piece.outline.reserve(globalOutline.size());
            piece.vertices.reserve(globalOutline.size());
            for (const QPointF& globalPoint : std::as_const(globalOutline))
            {
                const QPointF localPoint = globalPoint - cellOrigin;
                piece.outline.push_back(localPoint);
                piece.vertices.push_back({localPoint,
                                          {globalPoint.x() / completeSize.width(),
                                           globalPoint.y() / completeSize.height()}});
            }
            piece.localBounds = piece.outline.boundingRect();
            piece.indices = triangulate(piece.outline);
            pieces.push_back(std::move(piece));
        }
    }
    return pieces;
}
