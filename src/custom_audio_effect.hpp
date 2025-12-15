#pragma once

#include "custom_audio_effect_instance.hpp"
#include "reexport.h"

#include <godot_cpp/classes/audio_effect.hpp>

constexpr int DEFAULT_NUM_TAPS = 380;
constexpr int DEFAULT_NUM_FILTERS = DEFAULT_NUM_TAPS;

namespace godot {
class CustomAudioEffect : public AudioEffect {
	GDCLASS(CustomAudioEffect, AudioEffect)
	friend class CustomAudioEffectInstance;

public:
	virtual Ref<AudioEffectInstance> _instantiate() override;

	void set_num_sinc_taps(int num_sinc_taps);
	int get_num_sinc_taps() const;

	void set_num_sinc_filters(int num_sinc_filters);
	int get_num_sinc_filters() const;

	void set_lowpass_freq(double freq);
	double get_lowpass_freq() const;

	void set_output_sample_rate(double sample_rate);
	double get_output_sample_rate() const;

	void set_bit_depth(double bits);
	double get_bit_depth() const;

	bool sample_rate_changed() const;

protected:
	static void _bind_methods();

private:
	ResamplePtr resampler;
	ResamplePtr resampler2;

	int num_channels = 2; // Always 2
	double source_rate = 0;
	double dest_rate = 0;

	int num_taps = DEFAULT_NUM_TAPS;
	int num_filters = DEFAULT_NUM_FILTERS;
	double lowpass_freq = 0;

	double output_bits = 32;
	float output_bits_step = 1.0;

	void build_resampler();
};
} //namespace godot