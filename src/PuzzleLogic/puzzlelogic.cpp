#include "puzzlelogic.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

namespace
{
    PuzzlePoint rotateClockwise(PuzzlePoint point, int quarterTurns)
    {
        switch ((quarterTurns % 4 + 4) % 4)
        {
        case 1: return {-point.y, point.x};
        case 2: return {-point.x, -point.y};
        case 3: return {point.y, -point.x};
        default: return point;
        }
    }

    double lengthSquared(PuzzlePoint point)
    {
        return point.x * point.x + point.y * point.y;
    }
}

PuzzleLogic::PuzzleLogic(int columns, int rows, double pieceWidth, double pieceHeight)
{
    reset(columns, rows, pieceWidth, pieceHeight);
}

void PuzzleLogic::reset(int columns, int rows, double pieceWidth, double pieceHeight)
{
    m_columns = std::max(0, columns);
    m_rows = std::max(0, rows);
    m_pieceWidth = pieceWidth;
    m_pieceHeight = pieceHeight;
    m_poses.assign(m_columns * m_rows, {});
    m_connections.assign(m_poses.size(), {});
}

void PuzzleLogic::setPiecePose(int pieceId, PuzzlePiecePose pose)
{
    if (!isValidPiece(pieceId))
        return;
    pose.quarterTurns = (pose.quarterTurns % 4 + 4) % 4;
    m_poses[pieceId] = pose;
}

const PuzzlePiecePose& PuzzleLogic::piecePose(int pieceId) const
{
    return m_poses.at(pieceId);
}

std::vector<int> PuzzleLogic::connectedComponent(int pieceId) const
{
    if (!isValidPiece(pieceId))
        return {};

    std::vector<int> result;
    std::vector<bool> visited(m_poses.size(), false);
    std::queue<int> pending;
    pending.push(pieceId);
    visited[pieceId] = true;
    while (!pending.empty())
    {
        const int current = pending.front();
        pending.pop();
        result.push_back(current);
        for (const int neighbour : m_connections[current])
        {
            if (!visited[neighbour])
            {
                visited[neighbour] = true;
                pending.push(neighbour);
            }
        }
    }
    return result;
}

bool PuzzleLogic::isComplete() const
{
    return !m_poses.empty()
        && connectedComponent(0).size() == m_poses.size();
}

std::vector<std::pair<int, int>> PuzzleLogic::connections() const
{
    std::vector<std::pair<int, int>> result;
    for (int first = 0; first < static_cast<int>(m_connections.size()); ++first)
        for (const int second : m_connections[first])
            if (first < second)
                result.emplace_back(first, second);
    return result;
}

void PuzzleLogic::restoreConnections(
    const std::vector<std::pair<int, int>>& connections)
{
    for (auto& pieceConnections : m_connections)
        pieceConnections.clear();
    for (const auto& [first, second] : connections)
        if (isValidPiece(first) && isValidPiece(second)
            && areGridNeighbours(first, second))
            (void)connect(first, second);
}

void PuzzleLogic::detachMovingPieces(const std::vector<int>& movingPieceIds)
{
    std::vector<bool> moving(m_poses.size(), false);
    for (const int id : movingPieceIds)
        if (isValidPiece(id))
            moving[id] = true;

    for (int first = 0; first < static_cast<int>(m_poses.size()); ++first)
    {
        if (!moving[first])
            continue;
        const std::vector<int> neighbours = m_connections[first];
        for (const int second : neighbours)
            if (!moving[second])
                disconnect(first, second);
    }
}

void PuzzleLogic::detachAll(const std::vector<int>& pieceIds)
{
    for (const int id : pieceIds)
    {
        if (!isValidPiece(id))
            continue;
        const std::vector<int> neighbours = m_connections[id];
        for (const int neighbour : neighbours)
            disconnect(id, neighbour);
    }
}

PuzzleSnapResult PuzzleLogic::magneticAttraction(
    const std::vector<int>& movingPieceIds, double magnetRadius) const
{
    PuzzleSnapResult result;
    if (movingPieceIds.empty() || magnetRadius < 0.0)
        return result;

    std::vector<bool> moving(m_poses.size(), false);
    for (const int id : movingPieceIds)
        if (isValidPiece(id))
            moving[id] = true;

    double bestDistanceSquared = std::numeric_limits<double>::max();
    for (const int movingId : movingPieceIds)
    {
        if (!isValidPiece(movingId))
            continue;
        for (int stationaryId = 0; stationaryId < static_cast<int>(m_poses.size());
             ++stationaryId)
        {
            if (moving[stationaryId] || !areGridNeighbours(movingId, stationaryId)
                || m_poses[movingId].quarterTurns != m_poses[stationaryId].quarterTurns)
                continue;

            const PuzzlePoint expected = expectedPosition(movingId, stationaryId);
            const PuzzlePoint correction{
                expected.x - m_poses[movingId].position.x,
                expected.y - m_poses[movingId].position.y
            };
            const double distance = lengthSquared(correction);
            if (distance <= magnetRadius * magnetRadius && distance < bestDistanceSquared)
            {
                bestDistanceSquared = distance;
                result.translation = correction;
                result.snapped = true;
            }
        }
    }

    return result;
}

PuzzleSnapResult PuzzleLogic::snapMovedPieces(const std::vector<int>& movingPieceIds,
                                              double magnetRadius)
{
    PuzzleSnapResult result = magneticAttraction(movingPieceIds, magnetRadius);
    if (!result.snapped)
        return result;

    std::vector<bool> moving(m_poses.size(), false);
    for (const int id : movingPieceIds)
        if (isValidPiece(id))
            moving[id] = true;

    for (const int id : movingPieceIds)
    {
        if (isValidPiece(id))
        {
            m_poses[id].position.x += result.translation.x;
            m_poses[id].position.y += result.translation.y;
        }
    }
    // One snap can close several seams at once (for example, filling a hole).
    constexpr double alignmentTolerance = 0.001;
    for (const int movingId : movingPieceIds)
    {
        if (!isValidPiece(movingId))
            continue;
        for (int stationaryId = 0; stationaryId < static_cast<int>(m_poses.size());
             ++stationaryId)
        {
            if (!moving[stationaryId] && areGridNeighbours(movingId, stationaryId)
                && isAligned(movingId, stationaryId, alignmentTolerance))
            {
                if (connect(movingId, stationaryId))
                    result.newConnections.emplace_back(
                        std::min(movingId, stationaryId),
                        std::max(movingId, stationaryId));
            }
        }
    }
    return result;
}

bool PuzzleLogic::isValidPiece(int pieceId) const
{
    return pieceId >= 0 && pieceId < static_cast<int>(m_poses.size());
}

bool PuzzleLogic::areGridNeighbours(int first, int second) const
{
    const int firstColumn = first % m_columns;
    const int firstRow = first / m_columns;
    const int secondColumn = second % m_columns;
    const int secondRow = second / m_columns;
    return std::abs(firstColumn - secondColumn) + std::abs(firstRow - secondRow) == 1;
}

PuzzlePoint PuzzleLogic::expectedPosition(int pieceId, int anchorId) const
{
    const int columnDelta = pieceId % m_columns - anchorId % m_columns;
    const int rowDelta = pieceId / m_columns - anchorId / m_columns;
    const PuzzlePoint offset = rotateClockwise(
        {columnDelta * m_pieceWidth, rowDelta * m_pieceHeight},
        m_poses[anchorId].quarterTurns);
    return {
        m_poses[anchorId].position.x + offset.x,
        m_poses[anchorId].position.y + offset.y
    };
}

bool PuzzleLogic::isAligned(int first, int second, double tolerance) const
{
    if (m_poses[first].quarterTurns != m_poses[second].quarterTurns)
        return false;
    const PuzzlePoint expected = expectedPosition(first, second);
    return lengthSquared({
        expected.x - m_poses[first].position.x,
        expected.y - m_poses[first].position.y
    }) <= tolerance * tolerance;
}

bool PuzzleLogic::connect(int first, int second)
{
    if (std::find(m_connections[first].begin(), m_connections[first].end(), second)
        == m_connections[first].end())
    {
        m_connections[first].push_back(second);
        m_connections[second].push_back(first);
        return true;
    }
    return false;
}

void PuzzleLogic::disconnect(int first, int second)
{
    auto& firstConnections = m_connections[first];
    firstConnections.erase(std::remove(firstConnections.begin(), firstConnections.end(), second),
                           firstConnections.end());
    auto& secondConnections = m_connections[second];
    secondConnections.erase(std::remove(secondConnections.begin(), secondConnections.end(), first),
                            secondConnections.end());
}
