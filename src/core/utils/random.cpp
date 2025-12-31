#include "random.h"

#include <chrono>

namespace breakout {

Random::Random() {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    generator_ = std::mt19937(static_cast<unsigned int>(now));
}

Random::Random(int seed) : generator_(static_cast<unsigned int>(seed)) {}

int Random::nextInt(int minInclusive, int maxInclusive) {
    std::uniform_int_distribution<int> dist(minInclusive, maxInclusive);
    return dist(generator_);
}

double Random::nextDouble(double minInclusive, double maxInclusive) {
    std::uniform_real_distribution<double> dist(minInclusive, maxInclusive);
    return dist(generator_);
}

} // namespace breakout
