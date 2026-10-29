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
#include <numeric>

namespace
{
    // These fixed values define the first milestone's puzzle. They can later become user settings.
    constexpr int PieceColumns = 12;
    constexpr int PieceRows = 8;
    constexpr int PieceCount = PieceColumns * PieceRows;
    constexpr qreal MinimumZoom = 0.15;
    constexpr qreal MaximumZoom = 6.0;

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
    : QQuickItem(parent)
      , m_image(QStringLiteral(":/assets/test-puzzle.jpg"))
      , m_shapeSet(PuzzleShapeLoader::load(QStringLiteral(":/puzzle-shapes/classic.json")))
{
    setFlag(ItemHasContents, true);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setKeepMouseGrab(true);
    setFocus(true);
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

// Splits the image into UV regions and assigns them to deterministic shuffled board slots.
void PuzzleCanvas::initializePieces()
{
    if (m_initialized || m_image.isNull() || width() <= 1.0 || height() <= 1.0)
        return;

    constexpr qreal gap = 12.0;
    constexpr qreal horizontalMargin = 64.0;
    constexpr qreal verticalMargin = 56.0;

    // A uniform image scale preserves the source aspect ratio across the complete grid.
    const qreal availableWidth = std::max<qreal>(100.0, width() - 2.0 * horizontalMargin
                                                 - gap * (PieceColumns - 1));
    const qreal availableHeight = std::max<qreal>(100.0, height() - 2.0 * verticalMargin
                                                  - gap * (PieceRows - 1));
    const qreal imageScale = std::min(availableWidth / m_image.width(),
                                      availableHeight / m_image.height());
    const qreal pieceWidth = imageScale * m_image.width() / PieceColumns;
    const qreal pieceHeight = imageScale * m_image.height() / PieceRows;
    const qreal layoutWidth = PieceColumns * pieceWidth + (PieceColumns - 1) * gap;
    const qreal layoutHeight = PieceRows * pieceHeight + (PieceRows - 1) * gap;
    const QPointF origin((width() - layoutWidth) * 0.5, (height() - layoutHeight) * 0.5);

    QVector<PuzzlePieceGeometry> pieceGeometry = PuzzleGeometryGenerator::generate(
        m_shapeSet, PieceColumns, PieceRows, QSizeF(pieceWidth, pieceHeight), 0x50555A5Au);

    // A fixed seed keeps startup placement repeatable while interactions are under development.
    QVector<int> shuffledSlots(PieceCount);
    std::iota(shuffledSlots.begin(), shuffledSlots.end(), 0);
    QRandomGenerator random(0x50555A5Au);
    for (int i = shuffledSlots.size() - 1; i > 0; --i)
        std::swap(shuffledSlots[i], shuffledSlots[random.bounded(i + 1)]);

    m_pieces.reserve(PieceCount);
    m_drawOrder.reserve(PieceCount);
    for (int id = 0; id < PieceCount; ++id)
    {
        const int slot = shuffledSlots.at(id);
        const int boardColumn = slot % PieceColumns;
        const int boardRow = slot / PieceColumns;

        Piece piece;
        piece.position = QPointF(origin.x() + boardColumn * (pieceWidth + gap),
                                 origin.y() + boardRow * (pieceHeight + gap));
        piece.rotationCenter = QPointF(pieceWidth * 0.5, pieceHeight * 0.5);
        piece.quarterTurns = random.bounded(4);
        PuzzlePieceGeometry& geometry = pieceGeometry[id];
        piece.localBounds = geometry.localBounds;
        piece.outline = std::move(geometry.outline);
        piece.vertices = std::move(geometry.vertices);
        piece.indices = std::move(geometry.indices);
        m_pieces.push_back(piece);
        m_drawOrder.push_back(id);
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
    const QPointF delta = toWorld(screenPosition) - m_pressWorld;
    for (int id = 0; id < m_pieces.size(); ++id)
    {
        const bool isMoving = m_movingSelection
                                  ? m_pieces.at(id).selected
                                  : id == m_activePiece;
        if (isMoving)
            m_pieces[id].position = m_dragStartPositions.at(id) + delta;
    }
    markGeometryDirty();
}

void PuzzleCanvas::rotateMovingPieces()
{
    for (int id = 0; id < m_pieces.size(); ++id)
    {
        const bool isMoving = m_movingSelection
                                  ? m_pieces.at(id).selected
                                  : id == m_activePiece;
        if (isMoving)
            m_pieces[id].quarterTurns = (m_pieces.at(id).quarterTurns + 1) % 4;
    }
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
        // RMB rotates only pieces that are currently being moved.
        if (event->button() == Qt::RightButton)
        {
            rotateMovingPieces();
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
    if (m_interaction == Interaction::Pan)
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
        // Releasing the rotation button must not finish the LMB-held move.
        if (event->button() == Qt::RightButton)
        {
            event->accept();
            return;
        }

        moveActivePieces(event->position());
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

// Solves a new pan value that keeps the cursor's world point stationary while zooming.
void PuzzleCanvas::wheelEvent(QWheelEvent* event)
{
    const QPointF cursor = event->position();
    const QPointF anchor = toWorld(cursor);
    const qreal steps = event->angleDelta().y() / 120.0;
    const qreal nextZoom = std::clamp(m_zoom * std::pow(1.15, steps),
                                      MinimumZoom, MaximumZoom);
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

        root->pieceNode = new QSGGeometryNode;
        auto* material = new QSGTextureMaterial;
        material->setTexture(root->texture);
        material->setFiltering(QSGTexture::Linear);
        root->pieceNode->setMaterial(material);
        root->pieceNode->setFlag(QSGNode::OwnsMaterial);
        root->contentNode->appendChildNode(root->pieceNode);

        root->selectionNode = createFlatNode(QColor(255, 193, 7, 255));
        root->contentNode->appendChildNode(root->selectionNode);

        // Keep this translucent node alive between marquee drags so it remains in the
        // renderer's alpha pass after a marquee that selected no pieces.
        root->marqueeFillNode = createFlatNode(QColor(70, 150, 255, 120));
        root->contentNode->appendChildNode(root->marqueeFillNode);
        root->marqueeBorderNode = createFlatNode(QColor(100, 180, 255, 255));
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
                                       QSGGeometry::UnsignedShortType);
            root->pieceNode->setGeometry(geometry);
            root->pieceNode->setFlag(QSGNode::OwnsGeometry);
        }
        else
        {
            geometry->allocate(vertexCount, indexCount);
        }

        auto* vertices = geometry->vertexDataAsTexturedPoint2D();
        auto* indices = geometry->indexDataAsUShort();
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
                vertices[vertexOffset + i].set(float(position.x()), float(position.y()),
                                               float(source.textureUv.x()),
                                               float(source.textureUv.y()));
            }
            for (int i = 0; i < piece.indices.size(); ++i)
                indices[indexOffset + i] = quint16(vertexOffset + piece.indices.at(i));
            vertexOffset += piece.vertices.size();
            indexOffset += piece.indices.size();
        }
        geometry->setDrawingMode(QSGGeometry::DrawTriangles);
        root->pieceNode->markDirty(QSGNode::DirtyGeometry);
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
                                        3.0 / m_zoom);
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
            marqueeBorder = borderQuads(area, 1.5 / m_zoom);
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
