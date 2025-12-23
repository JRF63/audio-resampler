#pragma once

#include "biquad.hpp"

#include <boost/circular_buffer.hpp>

#include <cmath>

enum NoiseShapingFilter {
	NO_FILTER = 0,
	FIRST_ORDER = 1,
	SECOND_ORDER = 2,
};

struct ChannelFilter {
	float quant_error = 0.0f;
	Biquad biquad;

	boost::circular_buffer<float> sample_window;
	float sum_samples = 0.0f;
	float kahan_c = 0.0f;

	ChannelFilter() {}

	// Kahan sum. Adds input to sum_samples
	void add_to_sum(float input) {
		float y = input - kahan_c;
		float t = sum_samples + y;
		kahan_c = (t - sum_samples) - y;
		sum_samples = t;
	}

	inline void push_back_sample(float sample) {
		if (sample_window.capacity() == 0) {
			return;
		}
		if (sample_window.reserve() == 0) {
			// Subtract leaving data
			add_to_sum(-sample_window.front());
		}
		float input = fabsf(sample);
		add_to_sum(input);
		sample_window.push_back(input);
	}

	inline void set_volume_average_window(size_t size) {
		sample_window.set_capacity(size);
	}

	inline float volume_average() const {
		// Prevent divide by zero
		size_t denominator = (sample_window.size() == 0) ? 1 : sample_window.size();

		return sum_samples / denominator;
	}

	inline void save_quant_error(float error) {
		quant_error = error;
	}

	inline float process(NoiseShapingFilter order) {
		float shaping;
		switch (order) {
			case NoiseShapingFilter::FIRST_ORDER:
				shaping = quant_error;
			case NoiseShapingFilter::SECOND_ORDER:
				shaping = biquad.process(quant_error);
			default:
				shaping = 0.0;
		}
		return shaping;
	}
};