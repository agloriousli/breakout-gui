/**
 * @file brick.cpp
 * @brief Brick entity implementations for the Breakout game.
 * 
 * This file implements the brick hierarchy:
 * - Brick (base class): Abstract base with position, hit detection, destroyed state
 * - NormalBrick: Standard brick destroyed in 1 hit (green)
 * - DurableBrick: Tougher brick requiring multiple hits (blue -> cyan when damaged)
 * - IndestructibleBrick: Cannot be destroyed, acts as permanent obstacle (gray)
 * 
 * The BrickFactory provides a convenient way to create bricks from ASCII symbols
 * used in level layouts.
 */

#include "brick.h"

using namespace std;

namespace breakout {

/**
 * @brief Apply a hit to this brick and check if it's destroyed.
 * 
 * Each hit reduces hitsRemaining by 1. When hitsRemaining reaches 0,
 * the brick is marked as destroyed.
 * 
 * @return true if the brick was just destroyed, false otherwise
 */
bool Brick::applyHit() {
    // Indestructible bricks cannot be damaged
    if (!isBreakable()) {
        return false;
    }
    // Reduce remaining hits
    if (hitsRemaining_ > 0) {
        --hitsRemaining_;
    }
    // Check if brick is now destroyed
    if (hitsRemaining_ <= 0) {
        destroyed_ = true;
        return true;  // Brick destroyed
    }
    return false;  // Still alive after hit
}

/**
 * @brief Create a brick object from an ASCII character symbol.
 * 
 * This factory method converts level layout characters to brick objects:
 * - '@' -> NormalBrick (1 hit)
 * - '#' -> DurableBrick (2 hits)
 * - '*' -> IndestructibleBrick (unbreakable)
 * 
 * @param symbol The ASCII character from the level layout
 * @param bounds The position and size of the brick
 * @return Unique pointer to the created brick, or nullptr for unknown symbols
 */
std::unique_ptr<Brick> BrickFactory::createFromChar(char symbol, const Rect& bounds) {
    switch (symbol) {
        case '@':
            return std::make_unique<NormalBrick>(bounds);
        case '#':
            return std::make_unique<DurableBrick>(bounds, 2);
        case '*':
            return std::make_unique<IndestructibleBrick>(bounds);
        default:
            return nullptr;
    }
}

} // namespace breakout
