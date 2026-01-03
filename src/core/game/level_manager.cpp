/**
 * @file level_manager.cpp
 * @brief Manages level layouts and brick generation for the Breakout game.
 * 
 * This file implements the LevelManager class which:
 * - Stores level layouts as ASCII art strings
 * - Converts ASCII layouts to brick objects with proper positioning
 * - Supports multiple levels with different patterns
 * 
 * Layout Format:
 * Each level is defined as a vector of strings where each character represents:
 * - '@' = Normal brick (1 hit to destroy)
 * - '#' = Durable brick (2 hits to destroy, changes color when damaged)
 * - '*' = Indestructible brick (cannot be destroyed)
 * - ' ' = Empty space (no brick)
 * 
 * Example layout for a simple level:
 *   {"@@@@@@@@@@@@",
 *    "@#@#@#@#@#@#",
 *    "@@@@@***@@@@"}
 */

#include "level_manager.h"

using namespace std;

namespace breakout {

void LevelManager::setLayouts(const vector<vector<string>>& layouts) {
    layouts_ = layouts;
}

bool LevelManager::hasLevel(int index) const {
    int zeroBased = index - 1;
    return zeroBased >= 0 && zeroBased < static_cast<int>(layouts_.size());
}

size_t LevelManager::maxColumns(int index) const {
    int zeroBased = index - 1;
    if (!hasLevel(index)) {
        return 0;
    }
    size_t maxCols = 0;
    for (const auto& row : layouts_[zeroBased]) {
        maxCols = max(maxCols, row.size());
    }
    return maxCols;
}

/**
 * @brief Build brick objects for a specific level layout.
 * 
 * This function reads the ASCII layout for the specified level and
 * creates positioned brick objects. Each brick is placed according to:
 * - Column index determines X position: offsetX + col * brickWidth
 * - Row index determines Y position: offsetY + row * brickHeight
 * 
 * @param index Level number (1-indexed)
 * @param brickWidth Width of each brick in pixels
 * @param brickHeight Height of each brick in pixels
 * @param offsetX Starting X position for the brick grid
 * @param offsetY Starting Y position for the brick grid
 * @return Vector of unique_ptr to Brick objects
 */
vector<unique_ptr<Brick>> LevelManager::buildLevel(int index, double brickWidth, double brickHeight, double offsetX, double offsetY) const {
    vector<unique_ptr<Brick>> bricks;
    // Convert 1-indexed level number to 0-indexed array position
    int zeroBased = index - 1;
    if (!hasLevel(index)) {
        return bricks;
    }

    const auto& rows = layouts_[zeroBased];
    for (size_t r = 0; r < rows.size(); ++r) {
        const string& row = rows[r];
        for (size_t c = 0; c < row.size(); ++c) {
            char symbol = row[c];
            if (symbol == ' ') {
                continue;
            }
            Rect rect { offsetX + static_cast<double>(c) * brickWidth,
                        offsetY + static_cast<double>(r) * brickHeight,
                        brickWidth,
                        brickHeight };
            auto brick = BrickFactory::createFromChar(symbol, rect);
            if (brick) {
                bricks.push_back(std::move(brick));
            }
        }
    }

    return bricks;
}

} // namespace breakout
