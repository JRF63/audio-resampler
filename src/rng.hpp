#include <cstdint>

// Taken from WP:
// https://en.wikipedia.org/wiki/Permuted_congruential_generator

class Rng {
	static constexpr uint64_t init_state = 0x4d595df4d0f33173;
	static constexpr uint64_t multiplier = 6364136223846793005ULL;
	static constexpr uint64_t increment = 1442695040888963407ULL;

	uint64_t state = Rng::init_state;

public:
	float generate();

private:
	uint32_t rand();
};