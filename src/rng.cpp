#include "rng.hpp"

// float in [0,1)
float Rng::generate() {
	return (rand() >> 8) * (1.0f / 16777216.0f);
}

static inline uint32_t rotr32(uint32_t x, unsigned r) {
	return x >> r | x << (-r & 31);
}

uint32_t Rng::rand() {
	uint64_t x = state;
	unsigned count = (unsigned)(x >> 59);

	state = x * Rng::multiplier + Rng::increment;
	x ^= x >> 18;
	return rotr32((uint32_t)(x >> 27), count);
}
