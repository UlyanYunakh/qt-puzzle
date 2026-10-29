#include "PuzzleLogic/puzzlelogic.h"

#include <cassert>
#include <cmath>
#include <vector>

namespace
{
    bool close(double first, double second)
    {
        return std::abs(first - second) < 0.0001;
    }
}

int main()
{
    PuzzleLogic logic(3, 2, 100.0, 80.0);
    logic.setPiecePose(0, {{12.0, 10.0}, 0});
    logic.setPiecePose(1, {{117.0, 12.0}, 0});

    const PuzzleSnapResult attraction = logic.magneticAttraction({0}, 10.0);
    assert(attraction.snapped);
    assert(close(attraction.translation.x, 5.0));
    assert(close(attraction.translation.y, 2.0));
    assert(close(logic.piecePose(0).position.x, 12.0));
    assert(logic.connectedComponent(0).size() == 1);

    const PuzzleSnapResult firstSnap = logic.snapMovedPieces({0}, 10.0);
    assert(firstSnap.snapped);
    assert(firstSnap.newConnections.size() == 1);
    assert(close(logic.piecePose(0).position.x, 17.0));
    assert(close(logic.piecePose(0).position.y, 12.0));
    assert(logic.connectedComponent(0).size() == 2);

    // Moving only one member breaks the seam while retaining the pose.
    logic.detachMovingPieces({0});
    assert(logic.connectedComponent(0).size() == 1);
    assert(logic.connectedComponent(1).size() == 1);

    // Clockwise-rotated grid offsets connect vertically on screen.
    logic.setPiecePose(3, {{300.0, 300.0}, 1});
    logic.setPiecePose(4, {{302.0, 401.0}, 1});
    const PuzzleSnapResult rotatedSnap = logic.snapMovedPieces({4}, 5.0);
    assert(rotatedSnap.snapped);
    assert(close(logic.piecePose(4).position.x, 300.0));
    assert(close(logic.piecePose(4).position.y, 400.0));

    // Moving an entire connected component keeps its internal connection.
    logic.detachMovingPieces({3, 4});
    assert(logic.connectedComponent(3).size() == 2);

    PuzzleLogic completeLogic(2, 1, 100.0, 80.0);
    completeLogic.setPiecePose(0, {{0.0, 0.0}, 0});
    completeLogic.setPiecePose(1, {{101.0, 0.0}, 0});
    assert(!completeLogic.isComplete());
    const PuzzleSnapResult completingSnap = completeLogic.snapMovedPieces({1}, 2.0);
    assert(completingSnap.newConnections.size() == 1);
    assert(completeLogic.isComplete());
    return 0;
}
