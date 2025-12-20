#pragma once

#include "custom_audio_effect_instance.hpp"
#include "rng.hpp"

#include <godot_cpp/classes/audio_effect.hpp>

#include <array>
#include <deque>

namespace godot {

struct HeldSample {
	float sample;
	int32_t index;
};

struct ChannelFilter {
	float phase = 0.0f;
	float hold = 0.0f;

	std::deque<HeldSample> samples;
	int32_t index;

	// 2nd-order lowpass state
	float lp1 = 0.0f;
	float lp2 = 0.0f;

	float quant_error = 0.0f;
};

class CustomAudioEffect : public AudioEffect {
	GDCLASS(CustomAudioEffect, AudioEffect)
	friend class CustomAudioEffectInstance;

public:
	CustomAudioEffect();

	virtual Ref<AudioEffectInstance> _instantiate() override;

	void set_target_sample_rate(float sample_rate);
	float get_target_sample_rate() const;

	void set_lowpass_alpha(float alpha);
	float get_lowpass_alpha() const;

	void set_bit_depth(float bits);
	float get_bit_depth() const;

	void set_noise_shaping(float k);
	float get_noise_shaping() const;

	void set_dither_scale(float scale);
	float get_dither_scale() const;

	void set_interpolation(bool enable);
	bool get_interpolation() const;

protected:
	static void _bind_methods();

private:
	float target_sample_rate = 44100.0f;
	float lowpass_alpha = 1.0f;
	float bit_depth = 32.0f;
	float noise_shaping_k = 0.0f;
	float dither_scale = 0.0f;
	bool interpolation = false;

	float sample_rate_ratio = 1.0f;
	float bit_depth_step = 0x1p-32; // Exactly equal to ldexpf(1.0f, -32.0f);

	std::array<ChannelFilter, 2> channel_filters{};
	std::array<Rng, 2> channel_rngs;
};

} //namespace godot