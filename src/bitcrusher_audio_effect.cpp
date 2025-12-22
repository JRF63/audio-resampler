#include "bitcrusher_audio_effect.hpp"
#include "bitcrusher_audio_effect_instance.hpp"

#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

BitcrusherAudioEffect::BitcrusherAudioEffect() {
	create_resamplers();

	uint64_t seed = Rng::init_state - Rng::increment;
	channel_rngs[0].init(seed);
	channel_rngs[1].init(~seed);
}

Ref<AudioEffectInstance> BitcrusherAudioEffect::_instantiate() {
	Ref<BitcrusherAudioEffectInstance> ins;
	ins.instantiate();
	ins->base = Ref<BitcrusherAudioEffect>(this);

	return ins;
}

void BitcrusherAudioEffect::set_sample_rate(double sample_rate) {
	if (sample_rate >= 0) {
		target_sample_rate = sample_rate;
		create_resamplers();
	} else {
		ERR_FAIL_EDMSG("Invalid sample rate");
	}
}

void BitcrusherAudioEffect::set_downsampler_quality(SamplerQuality quality) {
	downsampler_quality = quality;
	create_resamplers();
}

void BitcrusherAudioEffect::set_upsampler_quality(SamplerQuality quality) {
	upsampler_quality = quality;
	create_resamplers();
}

void BitcrusherAudioEffect::set_downsampler_rolloff(SamplerRolloff rolloff) {
	downsampler_rolloff = rolloff;
	create_resamplers();
}

void BitcrusherAudioEffect::set_upsampler_rolloff(SamplerRolloff rolloff) {
	upsampler_rolloff = rolloff;
	create_resamplers();
}

void BitcrusherAudioEffect::set_downsampler_phase(SamplerPhase phase) {
	downsampler_phase = phase;
	create_resamplers();
}

void BitcrusherAudioEffect::set_upsampler_phase(SamplerPhase phase) {
	upsampler_phase = phase;
	create_resamplers();
}

void BitcrusherAudioEffect::set_downsampler_steep_filter(bool enable) {
	downsampler_steep_filter = enable;
	create_resamplers();
}

void BitcrusherAudioEffect::set_upsampler_steep_filter(bool enable) {
	upsampler_steep_filter = enable;
	create_resamplers();
}

// end resampler methods

void BitcrusherAudioEffect::set_bit_depth(float bits) {
	if (1 <= bits && bits <= 32) {
		bit_depth = bits;

		// bit_depth_step = powf(0.5f, bits);
		bit_depth_step = ldexpf(1.0f, -bits);
	} else {
		ERR_FAIL_EDMSG("Invalid output bit depth");
	}
}

void BitcrusherAudioEffect::set_dither_scale(float scale) {
	if (0.0 <= scale && scale <= 1.0) {
		dither_scale = scale;
	} else {
		ERR_FAIL_EDMSG("Invalid dither scale");
	}
}

void BitcrusherAudioEffect::set_noise_shaping_filter(NoiseShapingFilter order) {
	noise_shaping_filter = order;
}

void BitcrusherAudioEffect::set_filter_cutoff_frequency(float freq) {
	if (0.0 <= freq && freq <= 22000.0) {
		filter_cutoff_frequency = freq;
		for (int i = 0; i < 2; i++) {
			channel_filters[i].biquad.reset();
			channel_filters[i].biquad.init_shaper(target_sample_rate, filter_cutoff_frequency, filter_q);
		}
	} else {
		ERR_FAIL_EDMSG("Invalid 2nd-order filter cutoff frequency");
	}
}

void BitcrusherAudioEffect::set_filter_q(float q) {
	if (0.0 <= q && q <= 2.0) {
		filter_q = q;
		for (int i = 0; i < 2; i++) {
			channel_filters[i].biquad.reset();
			channel_filters[i].biquad.init_shaper(target_sample_rate, filter_cutoff_frequency, filter_q);
		}
	} else {
		ERR_FAIL_EDMSG("Invalid 2nd-order filter Q");
	}
}

void BitcrusherAudioEffect::set_output_buffer(int64_t samples) {
	num_samples_before_starting = samples;
}

void BitcrusherAudioEffect::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_sample_rate", "sample_rate"), &BitcrusherAudioEffect::set_sample_rate);
	ClassDB::bind_method(D_METHOD("get_sample_rate"), &BitcrusherAudioEffect::get_sample_rate);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sample_rate", PROPERTY_HINT_RANGE, "0,48000,0.1"), "set_sample_rate", "get_sample_rate");

	ClassDB::bind_method(D_METHOD("set_downsampler_quality", "quality"), &BitcrusherAudioEffect::set_downsampler_quality);
	ClassDB::bind_method(D_METHOD("get_downsampler_quality"), &BitcrusherAudioEffect::get_downsampler_quality);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "downsampler_quality", PROPERTY_HINT_ENUM, "Quick:0,Low:1,Medium:2,High:4,VeryHigh:6"), "set_downsampler_quality", "get_downsampler_quality");

	ClassDB::bind_method(D_METHOD("set_upsampler_quality", "quality"), &BitcrusherAudioEffect::set_upsampler_quality);
	ClassDB::bind_method(D_METHOD("get_upsampler_quality"), &BitcrusherAudioEffect::get_upsampler_quality);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "upsampler_quality", PROPERTY_HINT_ENUM, "Quick:0,Low:1,Medium:2,High:4,VeryHigh:6"), "set_upsampler_quality", "get_upsampler_quality");

	ClassDB::bind_method(D_METHOD("set_downsampler_rolloff", "rolloff"), &BitcrusherAudioEffect::set_downsampler_rolloff);
	ClassDB::bind_method(D_METHOD("get_downsampler_rolloff"), &BitcrusherAudioEffect::get_downsampler_rolloff);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "downsampler_rolloff", PROPERTY_HINT_ENUM, "Small:0,Medium:1,None:2"), "set_downsampler_rolloff", "get_downsampler_rolloff");

	ClassDB::bind_method(D_METHOD("set_upsampler_rolloff", "rolloff"), &BitcrusherAudioEffect::set_upsampler_rolloff);
	ClassDB::bind_method(D_METHOD("get_upsampler_rolloff"), &BitcrusherAudioEffect::get_upsampler_rolloff);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "upsampler_rolloff", PROPERTY_HINT_ENUM, "Small:0,Medium:1,None:2"), "set_upsampler_rolloff", "get_upsampler_rolloff");

	ClassDB::bind_method(D_METHOD("set_downsampler_phase", "phase"), &BitcrusherAudioEffect::set_downsampler_phase);
	ClassDB::bind_method(D_METHOD("get_downsampler_phase"), &BitcrusherAudioEffect::get_downsampler_phase);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "downsampler_phase", PROPERTY_HINT_ENUM, "Linear:0,Intermediate:16,Minimum:48"), "set_downsampler_phase", "get_downsampler_phase");

	ClassDB::bind_method(D_METHOD("set_upsampler_phase", "phase"), &BitcrusherAudioEffect::set_upsampler_phase);
	ClassDB::bind_method(D_METHOD("get_upsampler_phase"), &BitcrusherAudioEffect::get_upsampler_phase);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "upsampler_phase", PROPERTY_HINT_ENUM, "Linear:0,Intermediate:16,Minimum:48"), "set_upsampler_phase", "get_upsampler_phase");

	ClassDB::bind_method(D_METHOD("set_downsampler_steep_filter", "enable"), &BitcrusherAudioEffect::set_downsampler_steep_filter);
	ClassDB::bind_method(D_METHOD("get_downsampler_steep_filter"), &BitcrusherAudioEffect::get_downsampler_steep_filter);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "downsampler_steep_filter"), "set_downsampler_steep_filter", "get_downsampler_steep_filter");

	ClassDB::bind_method(D_METHOD("set_upsampler_steep_filter", "enable"), &BitcrusherAudioEffect::set_upsampler_steep_filter);
	ClassDB::bind_method(D_METHOD("get_upsampler_steep_filter"), &BitcrusherAudioEffect::get_upsampler_steep_filter);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "upsampler_steep_filter"), "set_upsampler_steep_filter", "get_upsampler_steep_filter");

	// end resampler properties

	ClassDB::bind_method(D_METHOD("set_dither_scale", "dither_scale"), &BitcrusherAudioEffect::set_dither_scale);
	ClassDB::bind_method(D_METHOD("get_dither_scale"), &BitcrusherAudioEffect::get_dither_scale);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "dither_scale", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_dither_scale", "get_dither_scale");

	ClassDB::bind_method(D_METHOD("set_bit_depth", "bit_depth"), &BitcrusherAudioEffect::set_bit_depth);
	ClassDB::bind_method(D_METHOD("get_bit_depth"), &BitcrusherAudioEffect::get_bit_depth);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "bit_depth", PROPERTY_HINT_RANGE, "1,32,0.01"), "set_bit_depth", "get_bit_depth");

	ClassDB::bind_method(D_METHOD("set_noise_shaping_filter", "order"), &BitcrusherAudioEffect::set_noise_shaping_filter);
	ClassDB::bind_method(D_METHOD("get_noise_shaping_filter"), &BitcrusherAudioEffect::get_noise_shaping_filter);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "noise_shaping_filter", PROPERTY_HINT_ENUM, "NoFilter:0,FirstOrder:1,SecondOrder:2"), "set_noise_shaping_filter", "get_noise_shaping_filter");

	ClassDB::bind_method(D_METHOD("set_filter_cutoff_frequency", "filter_cutoff_frequency"), &BitcrusherAudioEffect::set_filter_cutoff_frequency);
	ClassDB::bind_method(D_METHOD("get_filter_cutoff_frequency"), &BitcrusherAudioEffect::get_filter_cutoff_frequency);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "filter_cutoff_frequency", PROPERTY_HINT_RANGE, "0,22000,0.01"), "set_filter_cutoff_frequency", "get_filter_cutoff_frequency");

	ClassDB::bind_method(D_METHOD("set_filter_q", "filter_q"), &BitcrusherAudioEffect::set_filter_q);
	ClassDB::bind_method(D_METHOD("get_filter_q"), &BitcrusherAudioEffect::get_filter_q);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "filter_q", PROPERTY_HINT_RANGE, "0,2"), "set_filter_q", "get_filter_q");

	ClassDB::bind_method(D_METHOD("set_output_buffer", "samples"), &BitcrusherAudioEffect::set_output_buffer);
	ClassDB::bind_method(D_METHOD("get_output_buffer"), &BitcrusherAudioEffect::get_output_buffer);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "output_buffer", PROPERTY_HINT_RANGE, "1,51200"), "set_output_buffer", "get_output_buffer");
}

void BitcrusherAudioEffect::create_resamplers() {
	godot_sample_rate = AudioServer::get_singleton()->get_mix_rate();
	if (target_sample_rate == 0.0) {
		target_sample_rate = godot_sample_rate;
	}

	for (int i = 0; i < 2; i++) {
		channel_filters[i].biquad.init_shaper(target_sample_rate, filter_cutoff_frequency, filter_q);
	}

	soxr_datatype_t input_data_type = SOXR_FLOAT32_I; // 32-bit float interleaved
	soxr_datatype_t intermediate_data_type = SOXR_FLOAT32_I;
	soxr_datatype_t output_data_type = SOXR_FLOAT32_I;
	unsigned num_channels = 2; // Always 2
	unsigned num_threads = 1;
	soxr_runtime_spec_t runtime_spec = soxr_runtime_spec(num_threads);

	soxr_error_t error; // Used in the 2 soxr_create calls below

	// Build downsampler
	{
		soxr_io_spec_t io_spec_downsampler = soxr_io_spec(input_data_type, intermediate_data_type);

		unsigned long quality_spec_recipe = downsampler_quality | downsampler_phase | (downsampler_steep_filter ? SOXR_STEEP_FILTER : 0);
		unsigned long quality_spec_flags = downsampler_rolloff;
		soxr_quality_spec_t quality_spec = soxr_quality_spec(quality_spec_recipe, quality_spec_flags);

		auto downsampler = soxr_create(
				godot_sample_rate,
				target_sample_rate,
				num_channels,
				&error,
				&io_spec_downsampler,
				&quality_spec,
				&runtime_spec);

		if (error) {
			String msg = "Downsampler creation failed: ";
			msg += soxr_strerror(error);
			ERR_FAIL_EDMSG(msg);
		} else {
			soxr1 = SoxrPtr(downsampler);
		}
	}

	// Build upsampler
	{
		soxr_io_spec_t io_spec_upsampler = soxr_io_spec(intermediate_data_type, output_data_type);

		unsigned long quality_spec_recipe = upsampler_quality | upsampler_phase | (upsampler_steep_filter ? SOXR_STEEP_FILTER : 0);
		unsigned long quality_spec_flags = upsampler_rolloff;
		soxr_quality_spec_t quality_spec = soxr_quality_spec(quality_spec_recipe, quality_spec_flags);

		auto upsampler = soxr_create(
				target_sample_rate,
				godot_sample_rate,
				num_channels,
				&error,
				&io_spec_upsampler,
				&quality_spec,
				&runtime_spec);

		if (error) {
			String msg = "Upsampler creation failed: ";
			msg += soxr_strerror(error);
			ERR_FAIL_EDMSG(msg);
		} else {
			soxr2 = SoxrPtr(upsampler);
		}
	}
}
