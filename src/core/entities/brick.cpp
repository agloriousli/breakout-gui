#include "brick.h"

namespace breakout {

bool Brick::applyHit() {
    if (!isBreakable()) {
        return false;
    }
    if (hitsRemaining_ > 0) {
        --hitsRemaining_;
    }
    if (hitsRemaining_ <= 0) {
        destroyed_ = true;
        return true;  // Brick destroyed
    }
    return false;  // Still alive after hit
}

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
