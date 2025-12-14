#pragma once

#include "custom_audio_effect_instance.hpp"
#include "resampler_export.h"

#include <godot_cpp/classes/audio_effect.hpp>

#include <memory>

struct ResampleDeleter {
	void operator()(Resample *p) const noexcept {
		if (p) {
			resampleFree(p);
		}
	}
};

using ResamplePtr = std::unique_ptr<Resample, ResampleDeleter>;

namespace godot {
class CustomAudioEffect : public AudioEffect {
	GDCLASS(CustomAudioEffect, AudioEffect)
	friend class CustomAudioEffectInstance;

public:
	virtual Ref<AudioEffectInstance> _instantiate() override;

	void set_lowpass_ratio(double ratio);
	double get_lowpass_ratio() const;

	void set_ratio(double ratio);
	double get_ratio() const;

protected:
	static void _bind_methods();

private:
	ResamplePtr resampler;

	int num_channels = 2;
	int num_taps = 256;
	int num_filters = 256;
	double lowpass_ratio = 0.1;
	int flags = SUBSAMPLE_INTERPOLATE | BLACKMAN_HARRIS;
	double ratio = 0.1;

	void build_resampler();
};
} //namespace godot