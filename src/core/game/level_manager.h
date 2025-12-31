#pragma once

#include <string>
#include <vector>
#include <memory>

#include "../entities/brick.h"

namespace breakout {

class LevelManager {
public:
    void setLayouts(const std::vector<std::vector<std::string>>& layouts);

    bool hasLevel(int index) const;
    int levelCount() const { return static_cast<int>(layouts_.size()); }
    size_t maxColumns(int index) const;

    std::vector<std::unique_ptr<Brick>> buildLevel(int index, double brickWidth, double brickHeight, double offsetX = 0.0, double offsetY = 0.0) const;

private:
    std::vector<std::vector<std::string>> layouts_;
};

} // namespace breakout
