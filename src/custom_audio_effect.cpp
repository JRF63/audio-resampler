#include "custom_audio_effect.hpp"
#include "custom_audio_effect_instance.hpp"

#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

Ref<AudioEffectInstance> CustomAudioEffect::_instantiate() {
	build_resampler();

	Ref<CustomAudioEffectInstance> ins;
	ins.instantiate();
	ins->base = Ref<CustomAudioEffect>(this);

	return ins;
}

void CustomAudioEffect::build_resampler() {
	auto raw = resampleInit(num_channels,
			num_taps,
			num_filters,
			lowpass_ratio,
			flags);
	if (raw == nullptr) {
		ERR_FAIL_NULL_EDMSG(raw, "Unable to instantiate resampler");
	} else {
		resampler = ResamplePtr(raw);
	}
}

void CustomAudioEffect::set_lowpass_ratio(double ratio) {
	lowpass_ratio = ratio;
	build_resampler();
}

double CustomAudioEffect::get_lowpass_ratio() const {
	return lowpass_ratio;
}

void CustomAudioEffect::set_ratio(double ratio) {
	this->ratio = ratio;
}

double CustomAudioEffect::get_ratio() const {
	return ratio;
}

void CustomAudioEffect::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_lowpass_ratio", "lowpass_ratio"),
			&CustomAudioEffect::set_lowpass_ratio);
	ClassDB::bind_method(D_METHOD("get_lowpass_ratio"), &CustomAudioEffect::get_lowpass_ratio);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lowpass_ratio"), "set_lowpass_ratio", "get_lowpass_ratio");

	ClassDB::bind_method(D_METHOD("set_ratio", "ratio"),
			&CustomAudioEffect::set_ratio);
	ClassDB::bind_method(D_METHOD("get_ratio"), &CustomAudioEffect::get_ratio);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ratio"), "set_ratio", "get_ratio");
}
