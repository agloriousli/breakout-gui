#pragma once

#include <limits>
#include <memory>
#include <optional>
#include <string>

#include "../utils/collision.h"

namespace breakout {

enum class BrickType {
    Normal,
    Durable,
    Indestructible
};

struct BrickState {
    BrickType type { BrickType::Normal };
    Rect bounds;
    int hitsRemaining {1};
    bool destroyed {false};
};

class Brick {
public:
    virtual ~Brick() = default;

    const Rect& bounds() const { return bounds_; }
    BrickType type() const { return type_; }
    int hitsRemaining() const { return hitsRemaining_; }

    bool isBreakable() const { return type_ != BrickType::Indestructible; }
    bool isDestroyed() const { return destroyed_; }

    // Returns true when destroyed
    virtual bool applyHit();

    BrickState state() const { return { type_, bounds_, hitsRemaining_, destroyed_ }; }
    
    // Restore state from saved snapshot
    virtual void restoreState(const BrickState& state) {
        hitsRemaining_ = state.hitsRemaining;
        destroyed_ = state.destroyed;
    }

protected:
    Brick(Rect bounds, BrickType type, int hits) : bounds_(bounds), type_(type), hitsRemaining_(hits), destroyed_(false) {}

    Rect bounds_;
    BrickType type_ { BrickType::Normal };
    int hitsRemaining_ {1};
    bool destroyed_ {false};
};

class NormalBrick : public Brick {
public:
    explicit NormalBrick(const Rect& bounds) : Brick(bounds, BrickType::Normal, 1) {}
};

class DurableBrick : public Brick {
public:
    DurableBrick(const Rect& bounds, int hits) : Brick(bounds, BrickType::Durable, hits < 1 ? 1 : hits) {}
};

class IndestructibleBrick : public Brick {
public:
    explicit IndestructibleBrick(const Rect& bounds) : Brick(bounds, BrickType::Indestructible, std::numeric_limits<int>::max()) {}

    bool applyHit() override { return false; }
};

class BrickFactory {
public:
    static std::unique_ptr<Brick> createFromChar(char symbol, const Rect& bounds);
};

} // namespace breakout
