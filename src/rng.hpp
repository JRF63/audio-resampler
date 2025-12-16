#pragma once

#include <cstdint>

// Taken from WP:
// https://en.wikipedia.org/wiki/Permuted_congruential_generator

class Rng {
public:
	static constexpr uint64_t init_state = 0x4d595df4d0f33173;
	static constexpr uint64_t multiplier = 6364136223846793005ULL;
	static constexpr uint64_t increment = 1442695040888963407ULL;

	float generate();
	void init(uint64_t seed);

private:
	uint64_t state = Rng::init_state;

	uint32_t rand();
};