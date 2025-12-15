#include "custom_audio_effect.hpp"
#include "custom_audio_effect_instance.hpp"

#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

Ref<AudioEffectInstance> CustomAudioEffect::_instantiate() {
	build_resampler();

	Ref<CustomAudioEffectInstance> ins;
	ins.instantiate();
	ins->base = Ref<CustomAudioEffect>(this);

	return ins;
}

void CustomAudioEffect::set_num_sinc_taps(int num_sinc_taps) {
	if (4 <= num_sinc_taps && num_sinc_taps <= 1024 && (num_sinc_taps % 4 == 0)) {
		if (num_taps != num_sinc_taps) {
			num_taps = num_sinc_taps;
			build_resampler();
		}
	} else {
		ERR_FAIL_EDMSG("Invalid number of sinc taps");
	}
}

int CustomAudioEffect::get_num_sinc_taps() const {
	return num_taps;
}

void CustomAudioEffect::set_num_sinc_filters(int num_sinc_filters) {
	if (1 <= num_sinc_filters && num_sinc_filters <= 1024) {
		if (num_filters != num_sinc_filters) {
			num_filters = num_sinc_filters;
			build_resampler();
		}
	} else {
		ERR_FAIL_EDMSG("Invalid number of sinc filters");
	}
}

int CustomAudioEffect::get_num_sinc_filters() const {
	return num_filters;
}

void CustomAudioEffect::set_lowpass_freq(double freq) {
	if (lowpass_freq != freq) {
		lowpass_freq = freq;
		build_resampler();
	}
}

double CustomAudioEffect::get_lowpass_freq() const {
	return lowpass_freq;
}

void CustomAudioEffect::set_output_sample_rate(double sample_rate) {
	// Output sample rate affects resampler AND decimator
	if (dest_rate != sample_rate) {
		dest_rate = sample_rate;
		build_resampler();
	}
}

double CustomAudioEffect::get_output_sample_rate() const {
	return dest_rate;
}

void CustomAudioEffect::set_bit_depth(double bits) {
	if (1 <= bits && bits <= 32) {
		if (output_bits != bits) {
			output_bits = bits;
		}
	} else {
		ERR_FAIL_EDMSG("Invalid output bit depth");
	}
}

double CustomAudioEffect::get_bit_depth() const {
	return output_bits;
}

void CustomAudioEffect::build_resampler() {
	int resample_flags = SUBSAMPLE_INTERPOLATE | BLACKMAN_HARRIS;
	if (lowpass_freq != 0) {
		resample_flags |= INCLUDE_LOWPASS;
	}

	source_rate = AudioServer::get_singleton()->get_mix_rate();
	if (dest_rate == 0.0) {
		dest_rate = source_rate;
	}

	{
		auto raw = resampleFixedRatioInit(num_channels,
				num_taps,
				num_filters,
				source_rate,
				dest_rate,
				lowpass_freq,
				resample_flags);
		if (raw == nullptr) {
			String msg = "Unable to instantiate resampler - ";
			msg += " num_taps: ";
			msg += String::num_int64(num_taps);
			msg += " num_filters: ";
			msg += String::num_int64(num_filters);
			msg += " source_rate: ";
			msg += String::num_real(source_rate);
			msg += " dest_rate: ";
			msg += String::num_real(dest_rate);
			msg += " lowpass_freq: ";
			msg += String::num_real(lowpass_freq);
			msg += " resample_flags: ";
			msg += String::num_int64(resample_flags);
			ERR_FAIL_NULL_EDMSG(raw, msg);
		} else {
			resampler = ResamplePtr(raw);
		}
	}

	{
		auto raw = resampleFixedRatioInit(num_channels,
				num_taps,
				num_filters,
				dest_rate,
				source_rate,
				lowpass_freq,
				resample_flags);
		if (raw == nullptr) {
			String msg = "Unable to instantiate resampler - ";
			msg += " num_taps: ";
			msg += String::num_int64(num_taps);
			msg += " num_filters: ";
			msg += String::num_int64(num_filters);
			msg += " source_rate: ";
			msg += String::num_real(source_rate);
			msg += " dest_rate: ";
			msg += String::num_real(dest_rate);
			msg += " lowpass_freq: ";
			msg += String::num_real(lowpass_freq);
			msg += " resample_flags: ";
			msg += String::num_int64(resample_flags);
			ERR_FAIL_NULL_EDMSG(raw, msg);
		} else {
			resampler2 = ResamplePtr(raw);
		}
	}
}

bool CustomAudioEffect::sample_rate_changed() const {
	return source_rate != AudioServer::get_singleton()->get_mix_rate();
}

void CustomAudioEffect::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_num_sinc_taps", "num_sinc_taps"), &CustomAudioEffect::set_num_sinc_taps);
	ClassDB::bind_method(D_METHOD("get_num_sinc_taps"), &CustomAudioEffect::get_num_sinc_taps);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "num_sinc_taps", PROPERTY_HINT_RANGE, "4,1024,4"), "set_num_sinc_taps", "get_num_sinc_taps");

	ClassDB::bind_method(D_METHOD("set_num_sinc_filters", "num_sinc_filters"), &CustomAudioEffect::set_num_sinc_filters);
	ClassDB::bind_method(D_METHOD("get_num_sinc_filters"), &CustomAudioEffect::get_num_sinc_filters);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "num_sinc_filters", PROPERTY_HINT_RANGE, "1,1024,1"), "set_num_sinc_filters", "get_num_sinc_filters");

	ClassDB::bind_method(D_METHOD("set_lowpass_freq", "lowpass_freq"), &CustomAudioEffect::set_lowpass_freq);
	ClassDB::bind_method(D_METHOD("get_lowpass_freq"), &CustomAudioEffect::get_lowpass_freq);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lowpass_freq"), "set_lowpass_freq", "get_lowpass_freq");

	ClassDB::bind_method(D_METHOD("set_output_sample_rate", "sample_rate"), &CustomAudioEffect::set_output_sample_rate);
	ClassDB::bind_method(D_METHOD("get_output_sample_rate"), &CustomAudioEffect::get_output_sample_rate);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sample_rate"), "set_output_sample_rate", "get_output_sample_rate");

	ClassDB::bind_method(D_METHOD("set_bit_depth", "bit_depth"), &CustomAudioEffect::set_bit_depth);
	ClassDB::bind_method(D_METHOD("get_bit_depth"), &CustomAudioEffect::get_bit_depth);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "bit_depth", PROPERTY_HINT_RANGE, "1,32,0.01"), "set_bit_depth", "get_bit_depth");
}
