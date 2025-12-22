#pragma once

#include "biquad.hpp"
#include "rng.hpp"

#include <godot_cpp/classes/audio_effect.hpp>

#include <soxr.h>

#include <array>
#include <memory>

template <typename T, void (*FreeFn)(T *)>
struct CDeleter {
	void operator()(T *p) const noexcept {
		if (p) {
			FreeFn(p);
		}
	}
};

using SoxrPtr = std::unique_ptr<soxr, CDeleter<soxr, soxr_delete>>;

enum SamplerQuality {
	QUICK = SOXR_QQ,
	LOW = SOXR_LQ,
	MEDIUM = SOXR_MQ,
	HIGH = SOXR_HQ,
	VERY_HIGH = SOXR_VHQ,
};

enum SamplerPhase {
	LINEAR = SOXR_LINEAR_PHASE,
	INTERMEDIATE = SOXR_INTERMEDIATE_PHASE,
	MINIMUM = SOXR_MINIMUM_PHASE
};

enum SamplerRolloff {
	SMALL_ROLLOFF = SOXR_ROLLOFF_SMALL,
	MEDIUM_ROLLOFF = SOXR_ROLLOFF_MEDIUM,
	NO_ROLLOFF = SOXR_ROLLOFF_NONE,
};

enum NoiseShapingFilter {
	NO_FILTER = 0,
	FIRST_ORDER = 1,
	SECOND_ORDER = 2,
};

VARIANT_ENUM_CAST(SamplerQuality);
VARIANT_ENUM_CAST(SamplerPhase);
VARIANT_ENUM_CAST(SamplerRolloff);
VARIANT_ENUM_CAST(NoiseShapingFilter);

struct ChannelFilter {
	float quant_error = 0.0f;
	Biquad biquad;

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

namespace godot {
class BitcrusherAudioEffect : public AudioEffect {
	GDCLASS(BitcrusherAudioEffect, AudioEffect)
	friend class BitcrusherAudioEffectInstance;

public:
	BitcrusherAudioEffect();

	virtual Ref<AudioEffectInstance> _instantiate() override;

	void set_sample_rate(double sample_rate);
	double get_sample_rate() const { return target_sample_rate; }

	void set_downsampler_quality(SamplerQuality quality);
	SamplerQuality get_downsampler_quality() const { return downsampler_quality; }

	void set_upsampler_quality(SamplerQuality quality);
	SamplerQuality get_upsampler_quality() const { return upsampler_quality; }

	void set_downsampler_rolloff(SamplerRolloff rolloff);
	SamplerRolloff get_downsampler_rolloff() const { return downsampler_rolloff; }

	void set_upsampler_rolloff(SamplerRolloff rolloff);
	SamplerRolloff get_upsampler_rolloff() const { return upsampler_rolloff; }

	void set_downsampler_phase(SamplerPhase phase);
	SamplerPhase get_downsampler_phase() const { return downsampler_phase; }

	void set_upsampler_phase(SamplerPhase phase);
	SamplerPhase get_upsampler_phase() const { return upsampler_phase; }

	void set_downsampler_steep_filter(bool enable);
	bool get_downsampler_steep_filter() const { return downsampler_steep_filter; }

	void set_upsampler_steep_filter(bool enable);
	bool get_upsampler_steep_filter() const { return upsampler_steep_filter; }

	void set_bit_depth(float bits);
	float get_bit_depth() const { return bit_depth; }

	void set_dither_scale(float scale);
	float get_dither_scale() const { return dither_scale; }

	void set_noise_shaping_filter(NoiseShapingFilter order);
	NoiseShapingFilter get_noise_shaping_filter() const { return noise_shaping_filter; }

	void set_filter_cutoff_frequency(float freq);
	float get_filter_cutoff_frequency() const { return filter_cutoff_frequency; }

	void set_filter_q(float q);
	float get_filter_q() const { return filter_q; }

	void set_output_buffer(int64_t samples);
	int64_t get_output_buffer() const { return num_samples_before_starting; }

protected:
	static void _bind_methods();

private:
	SoxrPtr soxr1;
	SoxrPtr soxr2;

	double godot_sample_rate;
	double target_sample_rate;

	SamplerQuality downsampler_quality = SamplerQuality::HIGH;
	SamplerQuality upsampler_quality = SamplerQuality::HIGH;

	SamplerRolloff downsampler_rolloff = SamplerRolloff::SMALL_ROLLOFF;
	SamplerRolloff upsampler_rolloff = SamplerRolloff::SMALL_ROLLOFF;

	SamplerPhase downsampler_phase = SamplerPhase::LINEAR;
	SamplerPhase upsampler_phase = SamplerPhase::LINEAR;

	bool downsampler_steep_filter = false;
	bool upsampler_steep_filter = false;

	std::array<ChannelFilter, 2> channel_filters{};
	std::array<Rng, 2> channel_rngs;

	float bit_depth = 32.0f;
	float dither_scale = 0.0f;
	float bit_depth_step = 0x1p-32; // Exactly equal to ldexpf(1.0f, -32.0f);
	size_t num_samples_before_starting = 10 * 512;

	NoiseShapingFilter noise_shaping_filter = NoiseShapingFilter::NO_FILTER;
	float filter_cutoff_frequency = 3000.0f; // 3 kHz
	float filter_q = 0.70710678f; // 1/sqrt(2)

	void create_resamplers();
};

} //namespace godot