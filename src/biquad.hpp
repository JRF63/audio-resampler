#pragma once

#include <cmath>

struct Biquad {
	float b0, b1, b2;
	float a1, a2;
	float z1 = 0.0f;
	float z2 = 0.0f;

	inline float process(float x) {
		float y = b0 * x + z1;
		z1 = b1 * x - a1 * y + z2;
		z2 = b2 * x - a2 * y;
		return y;
	}

	void reset() {
		z1 = 0.0f;
		z2 = 0.0f;
	}

	void init_shaper(float fs, float fc, float Q) {
		constexpr float PI = 3.14159265358979323846264338327950288;
		float w0 = 2.0f * PI * fc / fs;
		float cosw0 = cosf(w0);
		float sinw0 = sinf(w0);
		float alpha = sinw0 / (2.0f * Q);

		float b0 = (1 + cosw0) / 2;
		float b1 = -(1 + cosw0);
		float b2 = (1 + cosw0) / 2;
		float a0 = 1 + alpha;
		float a1 = -2 * cosw0;
		float a2 = 1 - alpha;

		b0 = b0 / a0;
		b1 = b1 / a0;
		b2 = b2 / a0;
		a1 = a1 / a0;
		a2 = a2 / a0;
	}
};
