#ifndef PUZZLECANVAS_H
#define PUZZLECANVAS_H

#include "Config/puzzleconfig.h"
#include "PuzzleGeometry/puzzlegeometrygenerator.h"
#include "PuzzleLogic/puzzlelogic.h"

#include <QColor>
#include <QImage>
#include <QList>
#include <QPointF>
#include <QPolygonF>
#include <QQuickItem>
#include <QRectF>
#include <QUrl>
#include <QVector>
#include <QVariantMap>

#include <vector>

class QKeyEvent;
class QMouseEvent;
class QWheelEvent;

class PuzzleCanvas : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor
        NOTIFY backgroundColorChanged)
    Q_PROPERTY(QUrl sourceImage READ sourceImage NOTIFY sourceImageChanged)

public:
    /// Creates a canvas and loads the built-in test puzzle image.
    explicit PuzzleCanvas(QQuickItem* parent = nullptr);

    /// Creates a canvas from an already loaded application configuration.
    explicit PuzzleCanvas(const PuzzleConfig& config, QQuickItem* parent = nullptr);

    /// Returns the color painted behind the complete puzzle canvas.
    [[nodiscard]] QColor backgroundColor() const;

    /// Changes the canvas background color.
    void setBackgroundColor(const QColor& color);

    /// Returns the piece-size-aware upper zoom limit for the current puzzle.
    [[nodiscard]] qreal maximumZoom() const;

    /// Returns the image URL used by the current puzzle.
    [[nodiscard]] QUrl sourceImage() const;

    /// Returns the settings used to distribute pieces before play begins.
    [[nodiscard]] const PuzzleInitialLayoutConfig& initialLayoutConfig() const;

    /// Rebuilds the unplayed puzzle using a configurable perimeter distribution.
    void setInitialLayoutConfig(const PuzzleInitialLayoutConfig& config);

    /// Loads an image and starts a new puzzle with the requested grid dimensions.
    Q_INVOKABLE bool startPuzzle(const QUrl& imageUrl, int columns, int rows);

    /// Chooses a near-target grid whose individual pieces are close to square.
    Q_INVOKABLE bool startPuzzleWithPieceCount(const QUrl& imageUrl, int pieceCount);

    /// Serializes every gameplay transform and logical connection.
    Q_INVOKABLE QVariantMap saveState() const;

    /// Restores a state previously returned by saveState().
    Q_INVOKABLE bool restoreState(const QVariantMap& state);

signals:
    void backgroundColorChanged();
    void sourceImageChanged();

    /// Emitted once after a user finishes moving one piece or a selected group.
    void piecesPlaced(const QList<int>& pieceIds, bool snapped);

    /// Emitted for every new correctly aligned seam created by a placement.
    void piecesConnected(int firstPieceId, int secondPieceId);

    /// Emitted once when all pieces form one correctly connected component.
    void puzzleCompleted();

protected:
    /// Synchronizes puzzle geometry and visual overlays with the Qt Quick render thread.
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data) override;

    /// Creates the initial piece layout when the canvas receives a usable size.
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

    /// Starts piece dragging, lasso selection, or panning based on the pressed button.
    void mousePressEvent(QMouseEvent* event) override;

    /// Updates the active drag, lasso, or pan interaction.
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
        Lasso,
        Pan
    };

    /// Divides the image into non-overlapping slots around the central workspace.
    void initializePieces();

    /// Clears the current play state so initial layout settings can be reapplied.
    void resetPieces();

    /// Converts a canvas-space point into puzzle world coordinates.
    [[nodiscard]] QPointF toWorld(const QPointF& screenPosition) const;

    /// Returns the topmost piece under a world-space point, or -1 when none is hit.
    [[nodiscard]] int hitTest(const QPointF& worldPosition) const;

    /// Rotates a piece-local point clockwise in 90-degree increments.
    [[nodiscard]] QPointF rotatedLocalPoint(const Piece& piece,
                                            const QPointF& localPoint) const;

    /// Returns the piece outline after applying its current rotation.
    [[nodiscard]] QPolygonF rotatedOutline(const Piece& piece) const;

    /// Clears the selection state of every piece.
    void clearSelection();

    /// Moves the active piece or selected group to the top of the draw order.
    void raiseMovingPieces();

    /// Returns how many pieces belong to the persistent group selection.
    [[nodiscard]] int selectedPieceCount() const;

    /// Adds a sampled pointer position to the freehand lasso.
    void appendLassoPoint(const QPointF& worldPosition);

    /// Recalculates selection from the current lasso and modifier mode.
    void updateLassoSelection();

    /// Positions the active piece or selected group relative to the drag origin.
    void moveActivePieces(const QPointF& screenPosition);

    /// Returns the IDs of all pieces participating in the active move.
    [[nodiscard]] std::vector<int> movingPieceIds() const;

    /// Copies current canvas transforms into the platform-independent puzzle model.
    void synchronizeMovingPiecePoses();

    /// Applies a smooth fraction of the nearest compatible neighbour correction.
    void applyMagneticAttraction();

    /// Magnetizes the active move to the nearest compatible stationary neighbour.
    [[nodiscard]] PuzzleSnapResult snapMovingPieces();

    /// Rotates the active piece or selection as one rigid group by 90 degrees.
    void rotateMovingPieces(int direction);

    /// Invalidates piece geometry and every overlay that depends on it.
    void markGeometryDirty();

    /// Invalidates selection and lasso overlay geometry.
    void markOverlayDirty();

    PuzzleConfig m_config;
    QImage m_image;
    QUrl m_sourceImageUrl;
    QColor m_backgroundColor;
    PuzzleShapeSet m_shapeSet;
    PuzzleLogic m_logic;
    QVector<Piece> m_pieces;
    QVector<int> m_drawOrder;
    QVector<QPointF> m_dragStartPositions;
    QVector<bool> m_selectionBeforeLasso;
    QPolygonF m_lassoPoints;

    Interaction m_interaction = Interaction::None;
    QPointF m_pressScreen;
    QPointF m_pressWorld;
    QPointF m_currentWorld;
    QPointF m_panAtPress;
    QPointF m_pan;
    QPointF m_secondaryPressScreen;
    QPointF m_secondaryPanAtPress;
    int m_activePiece = -1;
    qreal m_zoom = 1.0;
    qreal m_maximumZoom = 6.0;
    qreal m_magnetRadius = 0.0;
    bool m_panMoved = false;
    bool m_secondaryPressed = false;
    bool m_secondaryPanning = false;
    bool m_rotateCounterClockwise = false;
    bool m_dragMoved = false;
    bool m_movingSelection = false;
    bool m_selectionOutlineEnabled = false;
    bool m_lassoAdditive = false;
    bool m_lassoToggle = false;
    bool m_initialized = false;
    bool m_puzzleCompleted = false;
    quint64 m_geometryRevision = 1;
    quint64 m_overlayRevision = 1;
};

#endif // PUZZLECANVAS_H
