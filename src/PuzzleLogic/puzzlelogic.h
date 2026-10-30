#ifndef PUZZLELOGIC_H
#define PUZZLELOGIC_H

#include <utility>
#include <vector>

// This module deliberately uses only the C++ standard library.  The canvas is
// an adapter around it, and another front end (for example Unreal) can provide
// its own vector and input conversions without bringing Qt with it.
struct PuzzlePoint
{
    double x = 0.0;
    double y = 0.0;
};

struct PuzzlePiecePose
{
    PuzzlePoint position;
    int quarterTurns = 0;
};

struct PuzzleSnapResult
{
    bool snapped = false;
    PuzzlePoint translation;
    std::vector<std::pair<int, int>> newConnections;
};

class PuzzleLogic
{
public:
    PuzzleLogic() = default;
    PuzzleLogic(int columns, int rows, double pieceWidth, double pieceHeight);

    void reset(int columns, int rows, double pieceWidth, double pieceHeight);
    void setPiecePose(int pieceId, PuzzlePiecePose pose);
    [[nodiscard]] const PuzzlePiecePose& piecePose(int pieceId) const;

    [[nodiscard]] std::vector<int> connectedComponent(int pieceId) const;
    [[nodiscard]] bool isComplete() const;
    [[nodiscard]] std::vector<std::pair<int, int>> connections() const;
    void restoreConnections(const std::vector<std::pair<int, int>>& connections);

    // Removes only connections that cross from the moving set to stationary
    // pieces. Internal connections remain intact, allowing a selected connected
    // component to move as one rigid group.
    void detachMovingPieces(const std::vector<int>& movingPieceIds);

    // Removes every connection involving one of the pieces. This is used when
    // rotation changes the relative geometry of the moving pieces.
    void detachAll(const std::vector<int>& pieceIds);

    // Returns the full correction toward the nearest compatible neighbour
    // without changing poses or connections. A front end can apply any fraction
    // of this vector to create a soft, controllable magnetic pull.
    [[nodiscard]] PuzzleSnapResult magneticAttraction(
        const std::vector<int>& movingPieceIds, double magnetRadius) const;

    // Finds a correctly oriented logical neighbour within the magnet radius,
    // translates the full moving set into exact alignment, and records every
    // neighbour connection produced by that alignment.
    [[nodiscard]] PuzzleSnapResult snapMovedPieces(
        const std::vector<int>& movingPieceIds, double magnetRadius);

private:
    [[nodiscard]] bool isValidPiece(int pieceId) const;
    [[nodiscard]] bool areGridNeighbours(int first, int second) const;
    [[nodiscard]] PuzzlePoint expectedPosition(int pieceId, int anchorId) const;
    [[nodiscard]] bool isAligned(int first, int second, double tolerance) const;
    [[nodiscard]] bool connect(int first, int second);
    void disconnect(int first, int second);

    int m_columns = 0;
    int m_rows = 0;
    double m_pieceWidth = 0.0;
    double m_pieceHeight = 0.0;
    std::vector<PuzzlePiecePose> m_poses;
    std::vector<std::vector<int>> m_connections;
};

#endif // PUZZLELOGIC_H
