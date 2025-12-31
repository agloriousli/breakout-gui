#include "level_manager.h"

namespace breakout {

void LevelManager::setLayouts(const std::vector<std::vector<std::string>>& layouts) {
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
        maxCols = std::max(maxCols, row.size());
    }
    return maxCols;
}

std::vector<std::unique_ptr<Brick>> LevelManager::buildLevel(int index, double brickWidth, double brickHeight, double offsetX, double offsetY) const {
    std::vector<std::unique_ptr<Brick>> bricks;
    int zeroBased = index - 1;
    if (!hasLevel(index)) {
        return bricks;
    }

    const auto& rows = layouts_[zeroBased];
    for (size_t r = 0; r < rows.size(); ++r) {
        const std::string& row = rows[r];
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
