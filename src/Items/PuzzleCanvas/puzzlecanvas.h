#ifndef PUZZLECANVAS_H
#define PUZZLECANVAS_H

#include "PuzzleGeometry/puzzlegeometrygenerator.h"

#include <QColor>
#include <QImage>
#include <QPointF>
#include <QPolygonF>
#include <QQuickItem>
#include <QRectF>
#include <QVector>

class QKeyEvent;
class QMouseEvent;
class QWheelEvent;

class PuzzleCanvas final : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor
        NOTIFY backgroundColorChanged)

public:
    /// Creates a canvas and loads the built-in test puzzle image.
    explicit PuzzleCanvas(QQuickItem* parent = nullptr);

    /// Returns the color painted behind the complete puzzle canvas.
    [[nodiscard]] QColor backgroundColor() const;

    /// Changes the canvas background color.
    void setBackgroundColor(const QColor& color);

signals:
    void backgroundColorChanged();

protected:
    /// Synchronizes puzzle geometry and visual overlays with the Qt Quick render thread.
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data) override;

    /// Creates the initial piece layout when the canvas receives a usable size.
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

    /// Starts piece dragging, marquee selection, or panning based on the pressed button.
    void mousePressEvent(QMouseEvent* event) override;

    /// Updates the active drag, marquee, or pan interaction.
    void mouseMoveEvent(QMouseEvent* event) override;

    /// Completes the active pointer interaction.
    void mouseReleaseEvent(QMouseEvent* event) override;

    /// Zooms around the cursor while keeping its world position fixed.
    void wheelEvent(QWheelEvent* event) override;

    /// Handles keyboard-based interaction cancellation.
    void keyPressEvent(QKeyEvent* event) override;

private:
    struct Piece
    {
        QPointF position;
        QPointF rotationCenter;
        QRectF localBounds;
        QPolygonF outline;
        QVector<PuzzleMeshVertex> vertices;
        QVector<quint16> indices;
        int quarterTurns = 0;
        bool selected = false;
    };

    enum class Interaction
    {
        None,
        DragPieces,
        Marquee,
        Pan
    };

    /// Divides the image into pieces and places them in shuffled board slots.
    void initializePieces();

    /// Converts a canvas-space point into puzzle world coordinates.
    [[nodiscard]] QPointF toWorld(const QPointF& screenPosition) const;

    /// Returns the topmost piece under a world-space point, or -1 when none is hit.
    [[nodiscard]] int hitTest(const QPointF& worldPosition) const;

    /// Rotates a piece-local point clockwise in 90-degree increments.
    [[nodiscard]] QPointF rotatedLocalPoint(const Piece& piece,
                                            const QPointF& localPoint) const;

    /// Returns the piece outline after applying its current rotation.
    [[nodiscard]] QPolygonF rotatedOutline(const Piece& piece) const;

    /// Returns the axis-aligned world bounds of a rotated piece.
    [[nodiscard]] QRectF pieceWorldBounds(const Piece& piece) const;

    /// Returns the normalized world-space marquee rectangle.
    [[nodiscard]] QRectF marqueeRect() const;

    /// Clears the selection state of every piece.
    void clearSelection();

    /// Moves the active piece or selected group to the top of the draw order.
    void raiseMovingPieces();

    /// Returns how many pieces belong to the persistent group selection.
    [[nodiscard]] int selectedPieceCount() const;

    /// Recalculates selection from the current marquee and modifier mode.
    void updateMarqueeSelection();

    /// Positions the active piece or selected group relative to the drag origin.
    void moveActivePieces(const QPointF& screenPosition);

    /// Rotates every piece in the active move by 90 degrees clockwise.
    void rotateMovingPieces();

    /// Invalidates piece geometry and every overlay that depends on it.
    void markGeometryDirty();

    /// Invalidates selection and marquee overlay geometry.
    void markOverlayDirty();

    QImage m_image;
    QColor m_backgroundColor = QColor(0x20, 0x24, 0x2b);
    PuzzleShapeSet m_shapeSet;
    QVector<Piece> m_pieces;
    QVector<int> m_drawOrder;
    QVector<QPointF> m_dragStartPositions;
    QVector<bool> m_selectionBeforeMarquee;

    Interaction m_interaction = Interaction::None;
    QPointF m_pressScreen;
    QPointF m_pressWorld;
    QPointF m_currentWorld;
    QPointF m_panAtPress;
    QPointF m_pan;
    int m_activePiece = -1;
    qreal m_zoom = 1.0;
    bool m_panMoved = false;
    bool m_movingSelection = false;
    bool m_marqueeAdditive = false;
    bool m_marqueeToggle = false;
    bool m_initialized = false;
    quint64 m_geometryRevision = 1;
    quint64 m_overlayRevision = 1;
};

#endif // PUZZLECANVAS_H
