#pragma once

#include <random>

namespace breakout {

class Random {
public:
    Random();
    explicit Random(int seed);

    int nextInt(int minInclusive, int maxInclusive);
    double nextDouble(double minInclusive, double maxInclusive);

private:
    std::mt19937 generator_;
};

} // namespace breakout
