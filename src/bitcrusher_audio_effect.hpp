#pragma once

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

namespace godot {

class BitcrusherAudioEffect : public AudioEffect {
	GDCLASS(BitcrusherAudioEffect, AudioEffect)
	friend class BitcrusherAudioEffectInstance;

public:
	BitcrusherAudioEffect();

	virtual Ref<AudioEffectInstance> _instantiate() override;

	void set_sample_rate(double sample_rate);
	double get_sample_rate() const;

protected:
	static void _bind_methods();

private:
	SoxrPtr soxr1;
	SoxrPtr soxr2;

	double godot_sample_rate;
	double target_sample_rate;

	std::array<Rng, 2> channel_rngs;

	void create_resamplers();
};

} //namespace godot