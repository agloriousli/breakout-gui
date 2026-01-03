/**
 * @file random.cpp
 * @brief Random number generation utilities for the Breakout game.
 * 
 * This file implements the Random class which provides:
 * - Integer random numbers in a specified range
 * - Floating-point random numbers in a specified range
 * - Reproducible sequences via optional seed parameter
 * 
 * The class uses the Mersenne Twister (mt19937) algorithm which provides
 * high-quality pseudo-random numbers suitable for game mechanics like
 * powerup drops and random brick patterns.
 * 
 * Seeding:
 * - Default constructor: Uses current time for unpredictable sequences
 * - Seeded constructor: Uses fixed seed for reproducible behavior (useful for testing)
 */

#include "random.h"

#include <chrono>

using namespace std;

namespace breakout {

/**
 * @brief Construct a Random generator seeded with current time.
 * 
 * This creates a random sequence that will be different each time
 * the game runs, making powerup drops and other random events
 * unpredictable.
 */
Random::Random() {
    // Use high-resolution clock for better randomness
    auto now = chrono::high_resolution_clock::now().time_since_epoch().count();
    generator_ = mt19937(static_cast<unsigned int>(now));
}

/**
 * @brief Construct a Random generator with a specific seed.
 * 
 * Using the same seed will produce the same sequence of random numbers,
 * which is useful for:
 * - Testing: Reproducible game behavior
 * - Debugging: Recreating specific scenarios
 * - Replays: Ensuring identical powerup drops
 * 
 * @param seed The seed value to initialize the generator
 */
Random::Random(int seed) : generator_(static_cast<unsigned int>(seed)) {}

/**
 * @brief Generate a random integer in the specified range.
 * 
 * Uses uniform distribution so each value in the range has equal probability.
 * 
 * @param minInclusive Minimum value (inclusive)
 * @param maxInclusive Maximum value (inclusive)
 * @return Random integer between minInclusive and maxInclusive
 */
int Random::nextInt(int minInclusive, int maxInclusive) {
    uniform_int_distribution<int> dist(minInclusive, maxInclusive);
    return dist(generator_);
}

/**
 * @brief Generate a random floating-point number in the specified range.
 * 
 * Used for probability checks (e.g., powerup spawn chance).
 * 
 * @param minInclusive Minimum value (inclusive)
 * @param maxInclusive Maximum value (inclusive)
 * @return Random double between minInclusive and maxInclusive
 */
double Random::nextDouble(double minInclusive, double maxInclusive) {
    uniform_real_distribution<double> dist(minInclusive, maxInclusive);
    return dist(generator_);
}

} // namespace breakout
