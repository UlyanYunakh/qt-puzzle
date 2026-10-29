#ifndef PUZZLECANVAS_H
#define PUZZLECANVAS_H

#include <QImage>
#include <QPointF>
#include <QQuickItem>
#include <QRectF>
#include <QVector>

class QKeyEvent;
class QMouseEvent;
class QWheelEvent;

class PuzzleCanvas final : public QQuickItem
{
    Q_OBJECT

public:
    /// Creates a canvas and loads the built-in test puzzle image.
    explicit PuzzleCanvas(QQuickItem *parent = nullptr);

protected:
    /// Synchronizes puzzle geometry and visual overlays with the Qt Quick render thread.
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;

    /// Creates the initial piece layout when the canvas receives a usable size.
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

    /// Starts piece dragging, marquee selection, or panning based on the pressed button.
    void mousePressEvent(QMouseEvent *event) override;

    /// Updates the active drag, marquee, or pan interaction.
    void mouseMoveEvent(QMouseEvent *event) override;

    /// Completes the active pointer interaction.
    void mouseReleaseEvent(QMouseEvent *event) override;

    /// Zooms around the cursor while keeping its world position fixed.
    void wheelEvent(QWheelEvent *event) override;

    /// Handles keyboard-assisted panning and selection cancellation.
    void keyPressEvent(QKeyEvent *event) override;

    /// Finishes temporary keyboard modes such as Space-based panning.
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    struct Piece {
        QRectF rect;
        QRectF sourceUv;
        bool selected = false;
    };

    enum class Interaction {
        None,
        DragPieces,
        Marquee,
        Pan
    };

    /// Divides the image into pieces and places them in shuffled board slots.
    void initializePieces();

    /// Converts a canvas-space point into puzzle world coordinates.
    [[nodiscard]] QPointF toWorld(const QPointF &screenPosition) const;

    /// Returns the topmost piece under a world-space point, or -1 when none is hit.
    [[nodiscard]] int hitTest(const QPointF &worldPosition) const;

    /// Returns the normalized world-space marquee rectangle.
    [[nodiscard]] QRectF marqueeRect() const;

    /// Clears the selection state of every piece.
    void clearSelection();

    /// Moves selected pieces to the end of draw order so they render on top.
    void raiseSelectedPieces();

    /// Recalculates selection from the current marquee and modifier mode.
    void updateMarqueeSelection();

    /// Invalidates piece geometry and every overlay that depends on it.
    void markGeometryDirty();

    /// Invalidates selection and marquee overlay geometry.
    void markOverlayDirty();

    QImage m_image;
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
    qreal m_zoom = 1.0;
    bool m_spaceHeld = false;
    bool m_marqueeAdditive = false;
    bool m_marqueeToggle = false;
    bool m_initialized = false;
    quint64 m_geometryRevision = 1;
    quint64 m_overlayRevision = 1;
};

#endif // PUZZLECANVAS_H
