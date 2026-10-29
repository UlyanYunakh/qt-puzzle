#include "puzzlecanvas.h"

#include "PuzzleGeometry/puzzlegeometrygenerator.h"
#include "PuzzleGeometry/puzzleshapeloader.h"

#include <QKeyEvent>
#include <QGuiApplication>
#include <QMatrix4x4>
#include <QMouseEvent>
#include <QQuickWindow>
#include <QRandomGenerator>
#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>
#include <QSGTextureMaterial>
#include <QSGTransformNode>
#include <QStyleHints>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace
{
    class PuzzleRenderNode final : public QSGNode
    {
    public:
        // QSGTextureMaterial references but does not own its texture.
        ~PuzzleRenderNode() override
        {
            delete texture;
        }

        QSGTexture* texture = nullptr;
        QSGGeometryNode* backgroundNode = nullptr;
        QSGTransformNode* contentNode = nullptr;
        QSGTransformNode* shadowTransformNode = nullptr;
        QSGGeometryNode* shadowNode = nullptr;
        QSGGeometryNode* pieceNode = nullptr;
        QSGGeometryNode* selectionNode = nullptr;
        QSGGeometryNode* marqueeFillNode = nullptr;
        QSGGeometryNode* marqueeBorderNode = nullptr;
        quint64 geometryRevision = 0;
        quint64 overlayRevision = 0;
    };

    // Creates a reusable scene-graph node for solid-color overlays.
    QSGGeometryNode* createFlatNode(const QColor& color)
    {
        auto* node = new QSGGeometryNode;
        auto* material = new QSGFlatColorMaterial;
        material->setColor(color);
        // These nodes are interaction overlays and must remain after the opaque piece batch.
        // Prevent the renderer from merging/reordering them with other flat-color geometry.
        material->setFlag(QSGMaterial::NoBatching, true);
        node->setMaterial(material);
        node->setFlag(QSGNode::OwnsMaterial);
        return node;
    }

    // Replaces a solid-color node's geometry with indexed quads.
    void setColoredQuads(QSGGeometryNode* node, const QVector<QRectF>& rectangles)
    {
        auto* geometry = node->geometry();
        if (!geometry)
        {
            geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 0, 0,
                                       QSGGeometry::UnsignedShortType);
            node->setGeometry(geometry);
            node->setFlag(QSGNode::OwnsGeometry);
        }

        const int vertexCount = rectangles.size() * 4;
        const int indexCount = rectangles.size() * 6;
        geometry->allocate(vertexCount, indexCount);

        auto* vertices = geometry->vertexDataAsPoint2D();
        auto* indices = geometry->indexDataAsUShort();
        for (int i = 0; i < rectangles.size(); ++i)
        {
            const QRectF rect = rectangles.at(i).normalized();
            const int vertex = i * 4;
            vertices[vertex + 0].set(float(rect.left()), float(rect.top()));
            vertices[vertex + 1].set(float(rect.right()), float(rect.top()));
            vertices[vertex + 2].set(float(rect.right()), float(rect.bottom()));
            vertices[vertex + 3].set(float(rect.left()), float(rect.bottom()));

            const int index = i * 6;
            indices[index + 0] = quint16(vertex + 0);
            indices[index + 1] = quint16(vertex + 1);
            indices[index + 2] = quint16(vertex + 2);
            indices[index + 3] = quint16(vertex + 0);
            indices[index + 4] = quint16(vertex + 2);
            indices[index + 5] = quint16(vertex + 3);
        }

        geometry->setDrawingMode(QSGGeometry::DrawTriangles);
        node->markDirty(QSGNode::DirtyGeometry);
    }

    // Replaces a solid-color node's geometry with arbitrary indexed triangles.
    void setColoredTriangles(QSGGeometryNode* node,
                             const QVector<QPointF>& points,
                             const QVector<quint32>& triangleIndices)
    {
        auto* geometry = node->geometry();
        if (!geometry)
        {
            geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 0, 0,
                                       QSGGeometry::UnsignedIntType);
            node->setGeometry(geometry);
            node->setFlag(QSGNode::OwnsGeometry);
        }

        geometry->allocate(points.size(), triangleIndices.size());
        auto* vertices = geometry->vertexDataAsPoint2D();
        auto* indices = static_cast<quint32*>(geometry->indexData());
        for (int i = 0; i < points.size(); ++i)
            vertices[i].set(float(points.at(i).x()), float(points.at(i).y()));
        std::copy(triangleIndices.cbegin(), triangleIndices.cend(), indices);

        geometry->setDrawingMode(QSGGeometry::DrawTriangles);
        node->markDirty(QSGNode::DirtyGeometry);
    }

    // Adds a screen-consistent strip around a piece's sampled outline.
    void appendOutlineBorder(QVector<QPointF>& points,
                             QVector<quint32>& indices,
                             const QPolygonF& outline,
                             const QPointF& position,
                             qreal thickness)
    {
        const qreal halfThickness = thickness * 0.5;
        for (int i = 0; i < outline.size(); ++i)
        {
            const QPointF start = position + outline.at(i);
            const QPointF end = position + outline.at((i + 1) % outline.size());
            const QPointF delta = end - start;
            const qreal length = std::hypot(delta.x(), delta.y());
            if (length <= 0.0001)
                continue;

            const QPointF direction = delta / length;
            const QPointF normal(-direction.y() * halfThickness,
                                 direction.x() * halfThickness);
            const QPointF extension = direction * halfThickness;
            const quint32 vertex = quint32(points.size());
            points += {
                start - extension + normal, end + extension + normal,
                end + extension - normal, start - extension - normal
            };
            indices += {
                vertex + 0, vertex + 1, vertex + 2,
                vertex + 0, vertex + 2, vertex + 3
            };
        }
    }

    // Uses quads instead of line primitives for consistent borders on every graphics backend.
    QVector<QRectF> borderQuads(const QRectF& rect, qreal thickness)
    {
        const QRectF outer = rect.adjusted(-thickness, -thickness, thickness, thickness);
        return {
            QRectF(outer.left(), outer.top(), outer.width(), thickness),
            QRectF(outer.left(), rect.bottom(), outer.width(), thickness),
            QRectF(outer.left(), rect.top(), thickness, rect.height()),
            QRectF(rect.right(), rect.top(), thickness, rect.height())
        };
    }
} // namespace

// Loads the embedded image and enables custom rendering and pointer input.
PuzzleCanvas::PuzzleCanvas(QQuickItem* parent)
    : PuzzleCanvas(PuzzleConfigLoader::load(QStringLiteral(":/config/puzzle-config.json")), parent)
{
}

PuzzleCanvas::PuzzleCanvas(const PuzzleConfig& config, QQuickItem* parent)
    : QQuickItem(parent)
      , m_config(config)
      , m_image(m_config.imageResource)
      , m_backgroundColor(m_config.appearance.background)
      , m_shapeSet(PuzzleShapeLoader::load(m_config.shapeResource))
{
    m_maximumZoom = m_config.interaction.maximumZoom;
    m_sourceImageUrl = QUrl(QStringLiteral("qrc") + m_config.imageResource);
    setFlag(ItemHasContents, true);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setKeepMouseGrab(true);
    setFocus(true);
}

qreal PuzzleCanvas::maximumZoom() const
{
    return m_maximumZoom;
}

QUrl PuzzleCanvas::sourceImage() const
{
    return m_sourceImageUrl;
}

QColor PuzzleCanvas::backgroundColor() const
{
    return m_backgroundColor;
}

void PuzzleCanvas::setBackgroundColor(const QColor& color)
{
    if (m_backgroundColor == color)
        return;

    m_backgroundColor = color;
    emit backgroundColorChanged();
    update();
}

const PuzzleInitialLayoutConfig& PuzzleCanvas::initialLayoutConfig() const
{
    return m_config.initialLayout;
}

void PuzzleCanvas::setInitialLayoutConfig(const PuzzleInitialLayoutConfig& config)
{
    m_config.initialLayout = config;
    resetPieces();
    initializePieces();
}

bool PuzzleCanvas::startPuzzle(const QUrl& imageUrl, int columns, int rows)
{
    if (columns < 1 || rows < 1 || qint64(columns) * rows > 10000)
        return false;

    QString imagePath = m_config.imageResource;
    if (!imageUrl.isEmpty())
    {
        if (imageUrl.isLocalFile())
            imagePath = imageUrl.toLocalFile();
        else if (imageUrl.scheme() == QStringLiteral("qrc"))
            imagePath = QStringLiteral(":") + imageUrl.path();
        else
            imagePath = imageUrl.toString();
    }

    const QImage image(imagePath);
    if (image.isNull())
        return false;

    m_config.columns = columns;
    m_config.rows = rows;
    m_image = image;
    const QUrl nextSource = imageUrl.isEmpty()
                                ? QUrl(QStringLiteral("qrc")
                                    + m_config.imageResource)
                                : imageUrl;
    if (m_sourceImageUrl != nextSource)
    {
        m_sourceImageUrl = nextSource;
        emit sourceImageChanged();
    }
    resetPieces();
    initializePieces();
    // A newly constructed QML item may not have received its final size yet;
    // geometryChange() will initialize it as soon as usable dimensions arrive.
    return true;
}

bool PuzzleCanvas::startPuzzleWithPieceCount(const QUrl& imageUrl, int pieceCount)
{
    if (pieceCount < 1 || pieceCount > 10000)
        return false;

    QString imagePath = m_config.imageResource;
    if (!imageUrl.isEmpty())
        imagePath = imageUrl.isLocalFile() ? imageUrl.toLocalFile() : imageUrl.toString();
    const QImage image(imagePath);
    if (image.isNull())
        return false;

    const qreal targetAspect = qreal(image.width()) / image.height();
    const int idealColumns = std::max(1, int(std::round(
                                          std::sqrt(pieceCount * targetAspect))));
    int bestColumns = idealColumns;
    int bestRows = std::max(1, int(std::round(qreal(pieceCount) / idealColumns)));
    qreal bestError = std::numeric_limits<qreal>::max();
    for (int columns = std::max(1, idealColumns - 8);
         columns <= idealColumns + 8; ++columns)
    {
        const int approximateRows = std::max(
            1, int(std::round(qreal(pieceCount) / columns)));
        for (int rows = std::max(1, approximateRows - 1);
             rows <= approximateRows + 1; ++rows)
        {
            // A cell is square when imageAspect * rows / columns == 1.
            // Shape receives more weight than a small difference from the
            // requested count, avoiding visibly stretched puzzle pieces.
            const qreal cellAspect = targetAspect * rows / columns;
            const qreal shapeError = std::abs(std::log(cellAspect));
            const qreal countError = std::abs(columns * rows - pieceCount)
                / qreal(pieceCount);
            const qreal error = shapeError * 2.0 + countError;
            if (error < bestError)
            {
                bestError = error;
                bestColumns = columns;
                bestRows = rows;
            }
        }
    }
    return startPuzzle(imageUrl, bestColumns, bestRows);
}

void PuzzleCanvas::resetPieces()
{
    m_pieces.clear();
    m_drawOrder.clear();
    m_dragStartPositions.clear();
    m_selectionBeforeMarquee.clear();
    m_logic = PuzzleLogic();
    m_interaction = Interaction::None;
    m_activePiece = -1;
    m_movingSelection = false;
    m_secondaryPressed = false;
    m_secondaryPanning = false;
    m_initialized = false;
    m_puzzleCompleted = false;
    m_zoom = 1.0;
    m_pan = QPointF();
    m_maximumZoom = m_config.interaction.maximumZoom;
    markGeometryDirty();
}

// Splits the image into UV regions and distributes pieces around a clear work area.
void PuzzleCanvas::initializePieces()
{
    if (m_initialized || m_image.isNull() || width() <= 1.0 || height() <= 1.0)
        return;

    const int pieceColumns = m_config.columns;
    const int pieceRows = m_config.rows;
    const int pieceCount = pieceColumns * pieceRows;
    const PuzzleInitialLayoutConfig& layoutConfig = m_config.initialLayout;
    const qreal gap = std::max<qreal>(0.0, layoutConfig.pieceGap);
    const qreal widthRatio = std::clamp<qreal>(layoutConfig.widthRatio,
                                               0.1, 1.0);
    const qreal heightRatio = std::clamp<qreal>(layoutConfig.heightRatio,
                                                0.1, 1.0);

    // Size the completed image for the protected central workspace. Initial
    // scatter spacing is independent of the solved row and column count.
    const qreal availableWidth = std::max<qreal>(
        100.0, width() * layoutConfig.centerClearWidthRatio);
    const qreal availableHeight = std::max<qreal>(
        100.0, height() * layoutConfig.centerClearHeightRatio);
    const qreal maximumImageScale = std::min(availableWidth / m_image.width(),
                                             availableHeight / m_image.height());
    const qreal imageScale = maximumImageScale
        * std::clamp<qreal>(layoutConfig.pieceScale, 0.1, 1.0);
    const qreal pieceWidth = imageScale * m_image.width() / pieceColumns;
    const qreal pieceHeight = imageScale * m_image.height() / pieceRows;
    PuzzleShapeSet renderedShapeSet = m_shapeSet;
    // Dense puzzles display very small pieces, where eight samples per Bezier
    // are visually redundant and make startup triangulation disproportionately
    // expensive. Preserve full detail for larger pieces and scale it down safely.
    renderedShapeSet.samplesPerBezier = std::clamp(
        int(std::min(pieceWidth, pieceHeight) / 4.0), 2,
        m_shapeSet.samplesPerBezier);
    QVector<PuzzlePieceGeometry> pieceGeometry = PuzzleGeometryGenerator::generate(
        renderedShapeSet, pieceColumns, pieceRows, QSizeF(pieceWidth, pieceHeight),
        m_config.geometrySeed);

    qreal maximumHalfExtent = 0.0;
    const QPointF rotationCenter(pieceWidth * 0.5, pieceHeight * 0.5);
    for (const PuzzlePieceGeometry& geometry : std::as_const(pieceGeometry))
    {
        const QRectF bounds = geometry.localBounds;
        maximumHalfExtent = std::max({
            maximumHalfExtent,
            std::abs(bounds.left() - rotationCenter.x()),
            std::abs(bounds.right() - rotationCenter.x()),
            std::abs(bounds.top() - rotationCenter.y()),
            std::abs(bounds.bottom() - rotationCenter.y())
        });
    }
    const qreal pieceFootprint = maximumHalfExtent * 2.0;
    QVector<QPointF> slotCenters;
    const QPointF layoutCenter(
        width() * std::clamp<qreal>(layoutConfig.centerX, 0.0, 1.0),
        height() * std::clamp<qreal>(layoutConfig.centerY, 0.0, 1.0));
    const QRectF layoutArea(layoutCenter.x() - width() * widthRatio * 0.5,
                            layoutCenter.y() - height() * heightRatio * 0.5,
                            width() * widthRatio, height() * heightRatio);

    // Reduce only scatter spacing when density requires it. Piece geometry
    // remains sized for the central solved image even for 2,000-piece puzzles.
    qreal packedGap = gap;
    constexpr int maximumPackingAttempts = 64;
    for (int attempt = 0; attempt < maximumPackingAttempts; ++attempt)
    {
        packedGap = gap * std::pow(0.88, attempt);
        const qreal slotStep = pieceFootprint + packedGap;
        const qreal maximumJitter = packedGap * 0.20
            * std::clamp<qreal>(layoutConfig.naturalJitter,
                                0.0, 1.0);
        const qreal left = layoutArea.left() + pieceFootprint * 0.5 + maximumJitter;
        const qreal right = layoutArea.right() - pieceFootprint * 0.5 - maximumJitter;
        const qreal top = layoutArea.top() + pieceFootprint * 0.5 + maximumJitter;
        const qreal bottom = layoutArea.bottom() - pieceFootprint * 0.5 - maximumJitter;
        const qreal clearHalfWidth = width()
            * std::clamp<qreal>(
                layoutConfig.centerClearWidthRatio, 0.0, 0.95)
            * 0.5
            + pieceFootprint * 0.5 + maximumJitter;
        const qreal clearHalfHeight = height()
            * std::clamp<qreal>(
                layoutConfig.centerClearHeightRatio, 0.0, 0.95)
            * 0.5
            + pieceFootprint * 0.5 + maximumJitter;

        slotCenters.clear();
        int row = 0;
        for (qreal y = top; y <= bottom + 0.001; y += slotStep, ++row)
        {
            const qreal stagger = row % 2 == 0 ? 0.0 : slotStep * 0.5;
            for (qreal x = left + stagger; x <= right + 0.001; x += slotStep)
            {
                const qreal normalizedX = (x - layoutCenter.x()) / clearHalfWidth;
                const qreal normalizedY = (y - layoutCenter.y()) / clearHalfHeight;
                if (normalizedX * normalizedX + normalizedY * normalizedY >= 1.0)
                    slotCenters.push_back(QPointF(x, y));
            }
        }

        if (slotCenters.size() >= pieceCount)
            break;
    }

    if (slotCenters.size() < pieceCount)
        throw std::runtime_error("Unable to fit non-overlapping pieces around the workspace");

    m_magnetRadius = std::min(pieceWidth, pieceHeight)
        * m_config.interaction.magnetRadiusRatio;
    const qreal pieceSize = std::max<qreal>(0.001,
                                            std::min(pieceWidth, pieceHeight));
    m_maximumZoom = std::clamp(
        m_config.interaction.zoomedPieceSize / pieceSize,
        m_config.interaction.maximumZoom,
        m_config.interaction.adaptiveZoomLimit);
    m_logic.reset(pieceColumns, pieceRows, pieceWidth, pieceHeight);

    // Shuffle the available perimeter slots and add bounded jitter. The slot
    // spacing retains enough clearance that the jitter cannot create overlap.
    QRandomGenerator random(layoutConfig.seed);
    for (int i = slotCenters.size() - 1; i > 0; --i)
        std::swap(slotCenters[i], slotCenters[random.bounded(i + 1)]);
    const qreal maximumJitter = packedGap * 0.20
        * std::clamp<qreal>(layoutConfig.naturalJitter, 0.0, 1.0);

    m_pieces.reserve(pieceCount);
    m_drawOrder.reserve(pieceCount);
    for (int id = 0; id < pieceCount; ++id)
    {
        QPointF slotCenter = slotCenters.at(id);
        if (maximumJitter > 0.0)
        {
            slotCenter.rx() += random.generateDouble() * maximumJitter * 2.0
                - maximumJitter;
            slotCenter.ry() += random.generateDouble() * maximumJitter * 2.0
                - maximumJitter;
        }

        Piece piece;
        piece.rotationCenter = QPointF(pieceWidth * 0.5, pieceHeight * 0.5);
        piece.position = slotCenter - piece.rotationCenter;
        piece.quarterTurns = random.bounded(4);
        PuzzlePieceGeometry& geometry = pieceGeometry[id];
        piece.localBounds = geometry.localBounds;
        piece.outline = std::move(geometry.outline);
        piece.vertices = std::move(geometry.vertices);
        piece.indices = std::move(geometry.indices);
        m_pieces.push_back(piece);
        m_drawOrder.push_back(id);
        m_logic.setPiecePose(id, {
                                 {piece.position.x(), piece.position.y()},
                                 piece.quarterTurns
                             });
    }

    m_initialized = true;
    markGeometryDirty();
}

// Reverses the pan/zoom transform applied by the root scene-graph node.
QPointF PuzzleCanvas::toWorld(const QPointF& screenPosition) const
{
    return (screenPosition - m_pan) / m_zoom;
}

// Positive quarter turns are clockwise in Qt's downward-pointing screen Y axis.
QPointF PuzzleCanvas::rotatedLocalPoint(const Piece& piece, const QPointF& localPoint) const
{
    const QPointF offset = localPoint - piece.rotationCenter;
    QPointF rotated;
    switch (piece.quarterTurns % 4)
    {
    case 1:
        rotated = QPointF(-offset.y(), offset.x());
        break;
    case 2:
        rotated = QPointF(-offset.x(), -offset.y());
        break;
    case 3:
        rotated = QPointF(offset.y(), -offset.x());
        break;
    default:
        rotated = offset;
        break;
    }
    return piece.rotationCenter + rotated;
}

QPolygonF PuzzleCanvas::rotatedOutline(const Piece& piece) const
{
    QPolygonF outline;
    outline.reserve(piece.outline.size());
    for (const QPointF& point : piece.outline)
        outline.push_back(rotatedLocalPoint(piece, point));
    return outline;
}

QRectF PuzzleCanvas::pieceWorldBounds(const Piece& piece) const
{
    return rotatedOutline(piece).boundingRect().translated(piece.position);
}

// Tests in reverse draw order so overlapping pieces select the visible topmost piece.
int PuzzleCanvas::hitTest(const QPointF& worldPosition) const
{
    for (auto it = m_drawOrder.crbegin(); it != m_drawOrder.crend(); ++it)
    {
        const Piece& piece = m_pieces.at(*it);
        const QPolygonF outline = rotatedOutline(piece).translated(piece.position);
        if (outline.boundingRect().contains(worldPosition)
            && outline.containsPoint(worldPosition, Qt::OddEvenFill))
            return *it;
    }
    return -1;
}

// Normalization supports marquee dragging in every direction.
QRectF PuzzleCanvas::marqueeRect() const
{
    return QRectF(m_pressWorld, m_currentWorld).normalized();
}

// Clears selection and schedules only the lightweight overlay for rebuilding.
void PuzzleCanvas::clearSelection()
{
    for (Piece& piece : m_pieces)
        piece.selected = false;
    markOverlayDirty();
}

int PuzzleCanvas::selectedPieceCount() const
{
    return int(std::count_if(m_pieces.cbegin(), m_pieces.cend(),
                             [](const Piece& piece) { return piece.selected; }));
}

// Preserves relative ordering while placing the pieces being moved on top.
void PuzzleCanvas::raiseMovingPieces()
{
    QVector<int> stationary;
    QVector<int> moving;
    stationary.reserve(m_drawOrder.size());
    moving.reserve(m_drawOrder.size());
    for (const int id : std::as_const(m_drawOrder))
    {
        const bool isMoving = m_movingSelection
                                  ? m_pieces.at(id).selected
                                  : id == m_activePiece;
        (isMoving ? moving : stationary).push_back(id);
    }
    stationary += moving;
    m_drawOrder = std::move(stationary);
    markGeometryDirty();
}

// Applies replace, union, or toggle behavior from the selection captured at drag start.
void PuzzleCanvas::updateMarqueeSelection()
{
    const QRectF area = marqueeRect();
    for (int id = 0; id < m_pieces.size(); ++id)
    {
        const Piece& piece = m_pieces.at(id);
        const bool inside = area.intersects(pieceWorldBounds(piece));
        if (m_marqueeToggle)
            m_pieces[id].selected = m_selectionBeforeMarquee.at(id) != inside;
        else if (m_marqueeAdditive)
            m_pieces[id].selected = m_selectionBeforeMarquee.at(id) || inside;
        else
            m_pieces[id].selected = inside;
    }
    markOverlayDirty();
}

// Keeps held dragging on one stable origin to avoid accumulating movement error.
void PuzzleCanvas::moveActivePieces(const QPointF& screenPosition)
{
    if (!m_dragMoved)
    {
        const QPointF screenDelta = screenPosition - m_pressScreen;
        if (std::hypot(screenDelta.x(), screenDelta.y())
            < QGuiApplication::styleHints()->startDragDistance())
            return;

        // A single connected piece, or a selected subset of a component, is
        // detached only when the pointer actually starts moving. This preserves
        // the component during the first click of a double click.
        m_logic.detachMovingPieces(movingPieceIds());
        m_dragMoved = true;
    }

    const QPointF delta = toWorld(screenPosition) - m_pressWorld;
    for (int id = 0; id < m_pieces.size(); ++id)
    {
        const bool isMoving = m_movingSelection
                                  ? m_pieces.at(id).selected
                                  : id == m_activePiece;
        if (isMoving)
            m_pieces[id].position = m_dragStartPositions.at(id) + delta;
    }
    synchronizeMovingPiecePoses();
    applyMagneticAttraction();
    markGeometryDirty();
}

std::vector<int> PuzzleCanvas::movingPieceIds() const
{
    std::vector<int> ids;
    ids.reserve(m_pieces.size());
    for (int id = 0; id < m_pieces.size(); ++id)
    {
        if (m_movingSelection ? m_pieces.at(id).selected : id == m_activePiece)
            ids.push_back(id);
    }
    return ids;
}

void PuzzleCanvas::synchronizeMovingPiecePoses()
{
    for (const int id : movingPieceIds())
    {
        const Piece& piece = m_pieces.at(id);
        m_logic.setPiecePose(id, {
                                 {piece.position.x(), piece.position.y()},
                                 piece.quarterTurns
                             });
    }
}

void PuzzleCanvas::applyMagneticAttraction()
{
    const std::vector<int> moving = movingPieceIds();
    const PuzzleSnapResult attraction = m_logic.magneticAttraction(moving, m_magnetRadius);
    if (!attraction.snapped || m_magnetRadius <= 0.0)
        return;

    const qreal distance = std::hypot(attraction.translation.x,
                                      attraction.translation.y);
    const qreal proximity = std::clamp(1.0 - distance / m_magnetRadius, 0.0, 1.0);
    const qreal smoothProximity = proximity * proximity * (3.0 - 2.0 * proximity);

    // Never apply the full correction while the pointer is held. The piece is
    // attracted toward its neighbour, but the cursor can always pull it away.
    const qreal pull = m_config.interaction.maximumMagneticPull * smoothProximity;
    const QPointF translation(attraction.translation.x * pull,
                              attraction.translation.y * pull);
    for (const int id : moving)
        m_pieces[id].position += translation;
    synchronizeMovingPiecePoses();
}

PuzzleSnapResult PuzzleCanvas::snapMovingPieces()
{
    const std::vector<int> moving = movingPieceIds();
    // Exact alignment is committed only in the inner part of the magnetic
    // field, keeping release near its outer edge from causing a large jump.
    const PuzzleSnapResult snap = m_logic.snapMovedPieces(
        moving, m_magnetRadius * m_config.interaction.connectionRadiusRatio);
    if (!snap.snapped)
        return snap;

    for (const int id : moving)
    {
        const PuzzlePiecePose& pose = m_logic.piecePose(id);
        m_pieces[id].position = QPointF(pose.position.x, pose.position.y);
    }

    for (const auto& [first, second] : snap.newConnections)
        emit piecesConnected(first, second);

    if (!m_puzzleCompleted && m_logic.isComplete())
    {
        m_puzzleCompleted = true;
        emit puzzleCompleted();
    }
    return snap;
}

void PuzzleCanvas::rotateMovingPieces(int direction)
{
    const std::vector<int> moving = movingPieceIds();
    if (moving.empty())
        return;
    direction = direction < 0 ? -1 : 1;

    // Connections inside the selection remain valid under a rigid rotation;
    // only seams leading to pieces outside the selection must be detached.
    m_logic.detachMovingPieces(moving);

    qreal left = std::numeric_limits<qreal>::max();
    qreal right = std::numeric_limits<qreal>::lowest();
    qreal top = std::numeric_limits<qreal>::max();
    qreal bottom = std::numeric_limits<qreal>::lowest();
    for (const int id : moving)
    {
        const QPointF center = m_pieces.at(id).position + m_pieces.at(id).rotationCenter;
        left = std::min(left, center.x());
        right = std::max(right, center.x());
        top = std::min(top, center.y());
        bottom = std::max(bottom, center.y());
    }
    const QPointF groupCenter((left + right) * 0.5, (top + bottom) * 0.5);

    for (const int id : moving)
    {
        Piece& piece = m_pieces[id];
        const QPointF center = piece.position + piece.rotationCenter;
        const QPointF offset = center - groupCenter;
        const QPointF rotatedOffset = direction > 0
                                          ? QPointF(-offset.y(), offset.x())
                                          : QPointF(offset.y(), -offset.x());
        piece.position = groupCenter + rotatedOffset - piece.rotationCenter;
        piece.quarterTurns = (piece.quarterTurns + direction + 4) % 4;
    }
    m_dragMoved = true;
    synchronizeMovingPiecePoses();
    markGeometryDirty();
}

// Revisions transfer invalidation safely from the GUI thread to the render thread.
void PuzzleCanvas::markGeometryDirty()
{
    ++m_geometryRevision;
    ++m_overlayRevision;
    update();
}

// Marks selection and marquee visuals for synchronization on the next frame.
void PuzzleCanvas::markOverlayDirty()
{
    ++m_overlayRevision;
    update();
}

// Defers initial layout until the window has assigned a usable canvas size.
void PuzzleCanvas::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    initializePieces();
}

// Chooses an interaction mode and captures the state required to update it.
void PuzzleCanvas::mousePressEvent(QMouseEvent* event)
{
    forceActiveFocus();

    if (m_interaction == Interaction::DragPieces)
    {
        // Defer the RMB decision until movement crosses the pan threshold or
        // the button is released as a click.
        if (event->button() == Qt::RightButton)
        {
            m_secondaryPressed = true;
            m_secondaryPanning = false;
            m_secondaryPressScreen = event->position();
            m_secondaryPanAtPress = m_pan;
            m_rotateCounterClockwise = event->modifiers().testFlag(Qt::ShiftModifier);
            event->accept();
            return;
        }
    }

    m_pressScreen = event->position();
    m_pressWorld = toWorld(m_pressScreen);
    m_currentWorld = m_pressWorld;

    // Right-button panning takes precedence over piece hit testing.
    if (event->button() == Qt::RightButton)
    {
        m_interaction = Interaction::Pan;
        m_panAtPress = m_pan;
        m_panMoved = false;
        m_rotateCounterClockwise = event->modifiers().testFlag(Qt::ShiftModifier);
        event->accept();
        return;
    }

    if (event->button() != Qt::LeftButton)
    {
        event->ignore();
        return;
    }

    const int hit = hitTest(m_pressWorld);
    const bool control = event->modifiers().testFlag(Qt::ControlModifier);
    const bool shift = event->modifiers().testFlag(Qt::ShiftModifier);
    if (hit >= 0)
    {
        bool beginMove = true;
        if (control)
        {
            m_pieces[hit].selected = !m_pieces.at(hit).selected;
            markOverlayDirty();
            beginMove = m_pieces.at(hit).selected;
        }
        else if (shift)
        {
            m_pieces[hit].selected = true;
            markOverlayDirty();
        }
        else if (!(m_pieces.at(hit).selected && selectedPieceCount() > 1))
        {
            // A normal LMB move has no persistent single-piece selection state.
            clearSelection();
        }

        if (beginMove)
        {
            m_activePiece = hit;
            m_movingSelection = m_pieces.at(hit).selected;
            // Every move is calculated from one stable drag origin to avoid accumulated error.
            raiseMovingPieces();
            m_dragStartPositions.resize(m_pieces.size());
            for (int id = 0; id < m_pieces.size(); ++id)
                m_dragStartPositions[id] = m_pieces.at(id).position;
            m_dragMoved = false;
            m_interaction = Interaction::DragPieces;
        }
    }
    else
    {
        // Marquee selection is recomputed from this snapshot instead of toggled cumulatively.
        m_selectionBeforeMarquee.resize(m_pieces.size());
        for (int id = 0; id < m_pieces.size(); ++id)
            m_selectionBeforeMarquee[id] = m_pieces.at(id).selected;
        m_marqueeToggle = control;
        m_marqueeAdditive = shift;
        if (!control && !shift)
            clearSelection();
        m_interaction = Interaction::Marquee;
        markOverlayDirty();
    }
    event->accept();
}

// Updates only the operation chosen when the pointer was pressed.
void PuzzleCanvas::mouseMoveEvent(QMouseEvent* event)
{
    if (m_interaction == Interaction::DragPieces && m_secondaryPressed)
    {
        const QPointF secondaryDelta = event->position() - m_secondaryPressScreen;
        if (!m_secondaryPanning
            && std::hypot(secondaryDelta.x(), secondaryDelta.y())
            >= QGuiApplication::styleHints()->startDragDistance())
            m_secondaryPanning = true;

        if (m_secondaryPanning)
            m_pan = m_secondaryPanAtPress + secondaryDelta;

        // Updating the dragged pieces after the pan keeps them beneath the
        // cursor while their world coordinates follow the moving viewport.
        moveActivePieces(event->position());
    }
    else if (m_interaction == Interaction::Pan)
    {
        if ((event->position() - m_pressScreen).manhattanLength()
            >= QGuiApplication::styleHints()->startDragDistance())
            m_panMoved = true;
        m_pan = m_panAtPress + event->position() - m_pressScreen;
        update();
    }
    else if (m_interaction == Interaction::DragPieces)
    {
        moveActivePieces(event->position());
    }
    else if (m_interaction == Interaction::Marquee)
    {
        m_currentWorld = toWorld(event->position());
        updateMarqueeSelection();
    }
    event->accept();
}

// Commits the final marquee position and removes transient interaction visuals.
void PuzzleCanvas::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_interaction == Interaction::DragPieces)
    {
        // RMB click rotates; RMB drag has already panned. In either case the
        // original LMB piece drag remains active.
        if (event->button() == Qt::RightButton)
        {
            if (m_secondaryPressed && !m_secondaryPanning)
            {
                rotateMovingPieces(m_rotateCounterClockwise ? -1 : 1);
                // Include the rotation in the stable drag origin, otherwise the
                // next pointer update would restore pre-rotation positions.
                const QPointF currentDelta = toWorld(event->position()) - m_pressWorld;
                for (const int id : movingPieceIds())
                    m_dragStartPositions[id] = m_pieces.at(id).position - currentDelta;
            }
            m_secondaryPressed = false;
            m_secondaryPanning = false;
            event->accept();
            return;
        }

        moveActivePieces(event->position());
        if (m_dragMoved)
        {
            const std::vector<int> placedIds = movingPieceIds();
            const PuzzleSnapResult placement = snapMovingPieces();
            QList<int> qtPlacedIds;
            qtPlacedIds.reserve(placedIds.size());
            for (const int id : placedIds)
                qtPlacedIds.push_back(id);
            emit piecesPlaced(qtPlacedIds, placement.snapped);
        }
        m_secondaryPressed = false;
        m_secondaryPanning = false;
        m_interaction = Interaction::None;
        m_activePiece = -1;
        m_movingSelection = false;
        markGeometryDirty();
        event->accept();
        return;
    }

    if (m_interaction == Interaction::Pan)
    {
        if (m_panMoved)
        {
            m_pan = m_panAtPress + event->position() - m_pressScreen;
        }
        else
        {
            // Sub-threshold pointer jitter belongs to the click, not to panning.
            m_pan = m_panAtPress;

            // RMB click rotates the clicked piece, or its persistent selection.
            // RMB drag remains reserved for canvas panning.
            const int hit = hitTest(m_pressWorld);
            if (hit >= 0)
            {
                m_activePiece = hit;
                m_movingSelection = m_pieces.at(hit).selected
                    && selectedPieceCount() > 1;
                rotateMovingPieces(m_rotateCounterClockwise ? -1 : 1);
                m_activePiece = -1;
                m_movingSelection = false;
                m_dragMoved = false;
            }
        }
        m_interaction = Interaction::None;
        markOverlayDirty();
        event->accept();
        return;
    }

    if (m_interaction == Interaction::Marquee)
    {
        m_currentWorld = toWorld(event->position());
        updateMarqueeSelection();
    }
    m_interaction = Interaction::None;
    markOverlayDirty();
    event->accept();
}

// The first click/release is intentionally non-destructive until movement crosses
// the drag threshold, so the original component is still available here.
void PuzzleCanvas::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
    {
        event->ignore();
        return;
    }

    const int hit = hitTest(toWorld(event->position()));
    if (hit < 0)
    {
        event->accept();
        return;
    }

    const std::vector<int> component = m_logic.connectedComponent(hit);
    clearSelection();
    for (const int id : component)
        m_pieces[id].selected = true;

    m_interaction = Interaction::None;
    m_activePiece = -1;
    m_movingSelection = false;
    markOverlayDirty();
    event->accept();
}

// Solves a new pan value that keeps the cursor's world point stationary while zooming.
void PuzzleCanvas::wheelEvent(QWheelEvent* event)
{
    const QPointF cursor = event->position();
    const QPointF anchor = toWorld(cursor);
    const qreal steps = event->angleDelta().y() / 120.0;
    const qreal nextZoom = std::clamp(m_zoom * std::pow(m_config.interaction.zoomStep, steps),
                                      m_config.interaction.minimumZoom,
                                      m_maximumZoom);
    m_zoom = nextZoom;
    m_pan = cursor - anchor * m_zoom;
    markOverlayDirty();
    event->accept();
}

// Escape cancels active movement and clears the selection.
void PuzzleCanvas::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape)
    {
        m_interaction = Interaction::None;
        m_activePiece = -1;
        m_movingSelection = false;
        clearSelection();
        event->accept();
    }
    else
    {
        QQuickItem::keyPressEvent(event);
    }
}

// Synchronizes GUI-thread model state into persistent render-thread scene-graph nodes.
QSGNode* PuzzleCanvas::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*)
{
    initializePieces();

    auto* root = static_cast<PuzzleRenderNode*>(oldNode);
    if (!root)
    {
        root = new PuzzleRenderNode;
        root->backgroundNode = createFlatNode(m_backgroundColor);
        root->appendChildNode(root->backgroundNode);

        // Pan and zoom affect puzzle content but never the canvas background.
        root->contentNode = new QSGTransformNode;
        root->appendChildNode(root->contentNode);

        // Every piece samples one mipmapped image texture through one shared material.
        root->texture = window()->createTextureFromImage(m_image, QQuickWindow::TextureHasMipmaps);
        root->texture->setFiltering(QSGTexture::Linear);
        root->texture->setMipmapFiltering(QSGTexture::Linear);

        // One opaque batched silhouette keeps shadows inexpensive even for very large puzzles.
        // It is composited before all pieces so shadows never darken the image itself.
        root->shadowTransformNode = new QSGTransformNode;
        root->contentNode->appendChildNode(root->shadowTransformNode);
        root->shadowNode = createFlatNode(m_config.appearance.pieceShadow);
        root->shadowTransformNode->appendChildNode(root->shadowNode);

        root->pieceNode = new QSGGeometryNode;
        auto* material = new QSGTextureMaterial;
        material->setTexture(root->texture);
        material->setFiltering(QSGTexture::Linear);
        root->pieceNode->setMaterial(material);
        root->pieceNode->setFlag(QSGNode::OwnsMaterial);
        root->contentNode->appendChildNode(root->pieceNode);

        root->selectionNode = createFlatNode(m_config.appearance.selection);
        root->contentNode->appendChildNode(root->selectionNode);

        // Keep this translucent node alive between marquee drags so it remains in the
        // renderer's alpha pass after a marquee that selected no pieces.
        root->marqueeFillNode = createFlatNode(m_config.appearance.marqueeFill);
        root->contentNode->appendChildNode(root->marqueeFillNode);
        root->marqueeBorderNode = createFlatNode(m_config.appearance.marqueeBorder);
        root->contentNode->appendChildNode(root->marqueeBorderNode);
    }

    setColoredQuads(root->backgroundNode, {boundingRect()});
    auto* backgroundMaterial = static_cast<QSGFlatColorMaterial*>(
        root->backgroundNode->material());
    backgroundMaterial->setColor(m_backgroundColor);
    root->backgroundNode->markDirty(QSGNode::DirtyMaterial);

    // Pan and zoom touch only this matrix; all vertices stay in puzzle world coordinates.
    QMatrix4x4 transform;
    transform.translate(float(m_pan.x()), float(m_pan.y()));
    transform.scale(float(m_zoom));
    root->contentNode->setMatrix(transform);

    // Express the configured screen-space drop in world coordinates so zoom does not
    // make the shadow appear farther away from its piece.
    QMatrix4x4 shadowTransform;
    shadowTransform.translate(float(m_config.appearance.pieceShadowOffsetX / m_zoom),
                              float(m_config.appearance.pieceShadowOffsetY / m_zoom));
    root->shadowTransformNode->setMatrix(shadowTransform);

    if (root->geometryRevision != m_geometryRevision)
    {
        // All pieces remain in one batch. Triangles in this opaque geometry have equal depth,
        // so the first overlapping triangle wins the depth test. Reverse logical draw order
        // to submit the topmost (including actively dragged) pieces first.
        int vertexCount = 0;
        int indexCount = 0;
        for (const Piece& piece : std::as_const(m_pieces))
        {
            vertexCount += piece.vertices.size();
            indexCount += piece.indices.size();
        }
        auto* geometry = root->pieceNode->geometry();
        if (!geometry)
        {
            geometry = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(),
                                       vertexCount, indexCount,
                                       QSGGeometry::UnsignedIntType);
            root->pieceNode->setGeometry(geometry);
            root->pieceNode->setFlag(QSGNode::OwnsGeometry);
        }
        else
        {
            geometry->allocate(vertexCount, indexCount);
        }

        auto* vertices = geometry->vertexDataAsTexturedPoint2D();
        auto* indices = static_cast<quint32*>(geometry->indexData());
        QVector<QPointF> shadowPoints;
        QVector<quint32> shadowIndices;
        shadowPoints.reserve(vertexCount);
        shadowIndices.resize(indexCount);
        int vertexOffset = 0;
        int indexOffset = 0;
        for (int drawIndex = 0; drawIndex < m_drawOrder.size(); ++drawIndex)
        {
            const int pieceId = m_drawOrder.at(m_drawOrder.size() - 1 - drawIndex);
            const Piece& piece = m_pieces.at(pieceId);
            for (int i = 0; i < piece.vertices.size(); ++i)
            {
                const PuzzleMeshVertex& source = piece.vertices.at(i);
                const QPointF position = piece.position
                    + rotatedLocalPoint(piece, source.localPosition);
                shadowPoints.push_back(position);
                vertices[vertexOffset + i].set(float(position.x()), float(position.y()),
                                               float(source.textureUv.x()),
                                               float(source.textureUv.y()));
            }
            for (int i = 0; i < piece.indices.size(); ++i)
            {
                indices[indexOffset + i] = quint32(vertexOffset) + piece.indices.at(i);
                shadowIndices[indexOffset + i] = indices[indexOffset + i];
            }
            vertexOffset += piece.vertices.size();
            indexOffset += piece.indices.size();
        }
        geometry->setDrawingMode(QSGGeometry::DrawTriangles);
        root->pieceNode->markDirty(QSGNode::DirtyGeometry);
        setColoredTriangles(root->shadowNode, shadowPoints, shadowIndices);
        root->geometryRevision = m_geometryRevision;
    }

    if (root->overlayRevision != m_overlayRevision)
    {
        // Only an actual multi-selection is highlighted; a lone moved piece is not.
        QVector<QPointF> selectionPoints;
        QVector<quint32> selectionIndices;
        if (selectedPieceCount() > 1)
        {
            for (const Piece& piece : std::as_const(m_pieces))
            {
                if (piece.selected)
                    appendOutlineBorder(selectionPoints, selectionIndices,
                                        rotatedOutline(piece), piece.position,
                                        m_config.appearance.selectionBorderWidth / m_zoom);
            }
        }
        setColoredTriangles(root->selectionNode, selectionPoints, selectionIndices);

        // Inverse zoom keeps marquee borders approximately constant on screen.
        QVector<QRectF> marqueeFill;
        QVector<QRectF> marqueeBorder;
        const bool marqueeActive = m_interaction == Interaction::Marquee;
        if (marqueeActive)
        {
            const QRectF area = marqueeRect();
            marqueeFill.push_back(area);
            marqueeBorder = borderQuads(
                area, m_config.appearance.marqueeBorderWidth / m_zoom);
        }
        else
        {
            // A non-empty off-canvas quad avoids the zero-geometry transition that can
            // make translucent nodes disappear from a later alpha-render pass.
            marqueeFill.push_back(QRectF(-1000000.0, -1000000.0, 1.0, 1.0));
        }
        setColoredQuads(root->marqueeFillNode, marqueeFill);
        setColoredQuads(root->marqueeBorderNode, marqueeBorder);
        root->overlayRevision = m_overlayRevision;
    }

    return root;
}
