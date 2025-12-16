#include "custom_audio_effect.hpp"
#include "custom_audio_effect_instance.hpp"

#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

CustomAudioEffect::CustomAudioEffect() {
	uint64_t seed = Rng::init_state - Rng::increment;
	channel_rngs[0].init(seed);
	channel_rngs[1].init(~seed);
}

Ref<AudioEffectInstance> CustomAudioEffect::_instantiate() {
	Ref<CustomAudioEffectInstance> ins;
	ins.instantiate();
	ins->base = Ref<CustomAudioEffect>(this);

	return ins;
}

void CustomAudioEffect::set_downsample_factor(int factor) {
	if (0 <= factor && factor <= 512) {
		downsample_factor = factor;
	} else {
		ERR_FAIL_EDMSG("Invalid downsample factor");
	}
}

int CustomAudioEffect::get_downsample_factor() const {
	return downsample_factor;
}

void CustomAudioEffect::set_lowpass_alpha(float alpha) {
	if (0.0 <= alpha && alpha <= 1.0) {
		lowpass_alpha = alpha;
	} else {
		ERR_FAIL_EDMSG("Invalid lowpass filter alpha factor");
	}
}

float CustomAudioEffect::get_lowpass_alpha() const {
	return lowpass_alpha;
}

void CustomAudioEffect::set_bit_depth(float bits) {
	if (1 <= bits && bits <= 32) {
		bit_depth = bits;

		// bit_depth_step = powf(0.5f, bits);
		bit_depth_step = ldexpf(1.0f, -bits);
	} else {
		ERR_FAIL_EDMSG("Invalid output bit depth");
	}
}

float CustomAudioEffect::get_bit_depth() const {
	return bit_depth;
}

void CustomAudioEffect::set_noise_shaping(float k) {
	if (0.0 <= k && k <= 1.0) {
		noise_shaping_k = k;
	} else {
		ERR_FAIL_EDMSG("Invalid noise shaping constant");
	}
}

float CustomAudioEffect::get_noise_shaping() const {
	return noise_shaping_k;
}

void CustomAudioEffect::set_dither_scale(float scale) {
	if (0.0 <= scale && scale <= 1.0) {
		dither_scale = scale;
	} else {
		ERR_FAIL_EDMSG("Invalid dither scale");
	}
}

float CustomAudioEffect::get_dither_scale() const {
	return dither_scale;
}

void CustomAudioEffect::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_downsample_factor", "downsample_factor"), &CustomAudioEffect::set_downsample_factor);
	ClassDB::bind_method(D_METHOD("get_downsample_factor"), &CustomAudioEffect::get_downsample_factor);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "downsample_factor", PROPERTY_HINT_RANGE, "0,512,1"), "set_downsample_factor", "get_downsample_factor");

	ClassDB::bind_method(D_METHOD("set_lowpass_alpha", "lowpass_alpha"), &CustomAudioEffect::set_lowpass_alpha);
	ClassDB::bind_method(D_METHOD("get_lowpass_alpha"), &CustomAudioEffect::get_lowpass_alpha);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lowpass_alpha", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_lowpass_alpha", "get_lowpass_alpha");

	ClassDB::bind_method(D_METHOD("set_dither_scale", "dither_scale"), &CustomAudioEffect::set_dither_scale);
	ClassDB::bind_method(D_METHOD("get_dither_scale"), &CustomAudioEffect::get_dither_scale);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "dither_scale", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_dither_scale", "get_dither_scale");

	ClassDB::bind_method(D_METHOD("set_noise_shaping", "noise_shaping"), &CustomAudioEffect::set_noise_shaping);
	ClassDB::bind_method(D_METHOD("get_noise_shaping"), &CustomAudioEffect::get_noise_shaping);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "noise_shaping", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_noise_shaping", "get_noise_shaping");

	ClassDB::bind_method(D_METHOD("set_bit_depth", "bit_depth"), &CustomAudioEffect::set_bit_depth);
	ClassDB::bind_method(D_METHOD("get_bit_depth"), &CustomAudioEffect::get_bit_depth);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "bit_depth", PROPERTY_HINT_RANGE, "1,32,0.01"), "set_bit_depth", "get_bit_depth");
}
