#include "bitcrusher_audio_effect.hpp"
#include "bitcrusher_audio_effect_instance.hpp"

#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

BitcrusherAudioEffect::BitcrusherAudioEffect() {
	uint64_t seed = Rng::init_state - Rng::increment;
	channel_rngs[0].init(seed);
	channel_rngs[1].init(~seed);
}

Ref<AudioEffectInstance> BitcrusherAudioEffect::_instantiate() {
	Ref<BitcrusherAudioEffectInstance> ins;
	ins.instantiate();
	ins->base = Ref<BitcrusherAudioEffect>(this);

	create_resamplers();

	return ins;
}

void BitcrusherAudioEffect::set_sample_rate(double sample_rate) {
	if (sample_rate >= 0) {
		std::lock_guard<std::mutex> lock(resampler_mutex);
		godot_sample_rate = AudioServer::get_singleton()->get_mix_rate();

		// Clamp to Godot's sample rate
		if (sample_rate > godot_sample_rate) {
			WARN_PRINT_ED("Cannot set target sample rate higher than Godot's sample rate");
			sample_rate = godot_sample_rate;
		}

		target_sample_rate = sample_rate;

		if (variable_rate) {
			if (soxr1 != nullptr) {
				soxr_set_io_ratio(soxr1.get(), godot_sample_rate / target_sample_rate, 0);
			}
			if (soxr2 != nullptr) {
				soxr_set_io_ratio(soxr2.get(), target_sample_rate / godot_sample_rate, 0);
			}
		} else {
			create_resamplers();
		}
	} else {
		ERR_FAIL_EDMSG("Invalid sample rate");
	}
}

void BitcrusherAudioEffect::set_variable_rate_resampling(bool enable) {
	std::lock_guard<std::mutex> lock(resampler_mutex);
	variable_rate = enable;
	create_resamplers();
}

void BitcrusherAudioEffect::set_downsampler_quality(SamplerQuality quality) {
	std::lock_guard<std::mutex> lock(resampler_mutex);
	downsampler_quality = quality;
	create_resamplers();
}

void BitcrusherAudioEffect::set_upsampler_quality(SamplerQuality quality) {
	std::lock_guard<std::mutex> lock(resampler_mutex);
	upsampler_quality = quality;
	create_resamplers();
}

void BitcrusherAudioEffect::set_downsampler_rolloff(SamplerRolloff rolloff) {
	std::lock_guard<std::mutex> lock(resampler_mutex);
	downsampler_rolloff = rolloff;
	create_resamplers();
}

void BitcrusherAudioEffect::set_upsampler_rolloff(SamplerRolloff rolloff) {
	std::lock_guard<std::mutex> lock(resampler_mutex);
	upsampler_rolloff = rolloff;
	create_resamplers();
}

void BitcrusherAudioEffect::set_downsampler_phase(SamplerPhase phase) {
	std::lock_guard<std::mutex> lock(resampler_mutex);
	downsampler_phase = phase;
	create_resamplers();
}

void BitcrusherAudioEffect::set_upsampler_phase(SamplerPhase phase) {
	std::lock_guard<std::mutex> lock(resampler_mutex);
	upsampler_phase = phase;
	create_resamplers();
}

void BitcrusherAudioEffect::set_downsampler_steep_filter(bool enable) {
	std::lock_guard<std::mutex> lock(resampler_mutex);
	downsampler_steep_filter = enable;
	create_resamplers();
}

void BitcrusherAudioEffect::set_upsampler_steep_filter(bool enable) {
	std::lock_guard<std::mutex> lock(resampler_mutex);
	upsampler_steep_filter = enable;
	create_resamplers();
}

// end resampler methods

void BitcrusherAudioEffect::set_bit_depth(double bits) {
	if (1 <= bits && bits <= 32) {
		std::lock_guard<std::mutex> lock(resampler_mutex);
		bit_depth = bits;

		// bit_depth_step = powf(0.5f, bits);
		bit_depth_step = ldexp(1.0, -bits);
		bit_depth_step_inv = 1.0 / bit_depth_step;
	} else {
		ERR_FAIL_EDMSG("Invalid output bit depth");
	}
}

void BitcrusherAudioEffect::set_dither_scale(float scale) {
	if (0.0 <= scale && scale <= 1.0) {
		std::lock_guard<std::mutex> lock(resampler_mutex);
		dither_scale = scale;
	} else {
		ERR_FAIL_EDMSG("Invalid dither scale");
	}
}

void BitcrusherAudioEffect::set_dither_threshold(float threshold) {
	if (0 <= threshold) {
		std::lock_guard<std::mutex> lock(resampler_mutex);
		dither_threshold = threshold;
	} else {
		ERR_FAIL_EDMSG("Invalid dither window size");
	}
}

void BitcrusherAudioEffect::set_dither_window(int64_t size) {
	if (1 <= size) {
		std::lock_guard<std::mutex> lock(resampler_mutex);
		dither_window = size;

		for (int i = 0; i < 2; i++) {
			channel_filters[i].set_volume_average_window(size);
		}
	} else {
		ERR_FAIL_EDMSG("Invalid dither window size");
	}
}

void BitcrusherAudioEffect::set_noise_shaping_filter(NoiseShapingFilter order) {
	std::lock_guard<std::mutex> lock(resampler_mutex);
	noise_shaping_filter = order;
	notify_property_list_changed();
}

void BitcrusherAudioEffect::set_filter_cutoff_frequency(float freq) {
	if (0.0 <= freq && freq <= 22000.0) {
		std::lock_guard<std::mutex> lock(resampler_mutex);
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
		std::lock_guard<std::mutex> lock(resampler_mutex);
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
	std::lock_guard<std::mutex> lock(resampler_mutex);
	num_samples_before_starting = samples;
}

void BitcrusherAudioEffect::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_sample_rate", "sample_rate"), &BitcrusherAudioEffect::set_sample_rate);
	ClassDB::bind_method(D_METHOD("get_sample_rate"), &BitcrusherAudioEffect::get_sample_rate);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sample_rate"), "set_sample_rate", "get_sample_rate");

	ClassDB::bind_method(D_METHOD("set_bit_depth", "bit_depth"), &BitcrusherAudioEffect::set_bit_depth);
	ClassDB::bind_method(D_METHOD("get_bit_depth"), &BitcrusherAudioEffect::get_bit_depth);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "bit_depth", PROPERTY_HINT_RANGE, "1,32,0.01"), "set_bit_depth", "get_bit_depth");

	ClassDB::bind_method(D_METHOD("set_output_buffer", "samples"), &BitcrusherAudioEffect::set_output_buffer);
	ClassDB::bind_method(D_METHOD("get_output_buffer"), &BitcrusherAudioEffect::get_output_buffer);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "output_buffer", PROPERTY_HINT_RANGE, "1,51200"), "set_output_buffer", "get_output_buffer");

	ClassDB::bind_method(D_METHOD("set_variable_rate_resampling", "enable"), &BitcrusherAudioEffect::set_variable_rate_resampling);
	ClassDB::bind_method(D_METHOD("get_variable_rate_resampling"), &BitcrusherAudioEffect::get_variable_rate_resampling);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "variable_rate_resampling"), "set_variable_rate_resampling", "get_variable_rate_resampling");

	ADD_GROUP("Dithering", "dither_");

	ClassDB::bind_method(D_METHOD("set_dither_scale", "dither_scale"), &BitcrusherAudioEffect::set_dither_scale);
	ClassDB::bind_method(D_METHOD("get_dither_scale"), &BitcrusherAudioEffect::get_dither_scale);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "dither_scale", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_dither_scale", "get_dither_scale");

	ClassDB::bind_method(D_METHOD("set_dither_threshold", "dither_threshold"), &BitcrusherAudioEffect::set_dither_threshold);
	ClassDB::bind_method(D_METHOD("get_dither_threshold"), &BitcrusherAudioEffect::get_dither_threshold);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "dither_threshold", PROPERTY_HINT_RANGE, "0,0.5,0.0001"), "set_dither_threshold", "get_dither_threshold");

	ClassDB::bind_method(D_METHOD("set_dither_window", "dither_window"), &BitcrusherAudioEffect::set_dither_window);
	ClassDB::bind_method(D_METHOD("get_dither_window"), &BitcrusherAudioEffect::get_dither_window);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "dither_window", PROPERTY_HINT_RANGE, "1,1024"), "set_dither_window", "get_dither_window");

	ADD_GROUP("Downsampler", "downsampler_");

	ClassDB::bind_method(D_METHOD("set_downsampler_quality", "quality"), &BitcrusherAudioEffect::set_downsampler_quality);
	ClassDB::bind_method(D_METHOD("get_downsampler_quality"), &BitcrusherAudioEffect::get_downsampler_quality);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "downsampler_quality", PROPERTY_HINT_ENUM, "Quick:0,Low:1,Medium:2,High:4,VeryHigh:6"), "set_downsampler_quality", "get_downsampler_quality");

	ClassDB::bind_method(D_METHOD("set_downsampler_rolloff", "rolloff"), &BitcrusherAudioEffect::set_downsampler_rolloff);
	ClassDB::bind_method(D_METHOD("get_downsampler_rolloff"), &BitcrusherAudioEffect::get_downsampler_rolloff);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "downsampler_rolloff", PROPERTY_HINT_ENUM, "Small:0,Medium:1,None:2"), "set_downsampler_rolloff", "get_downsampler_rolloff");

	ClassDB::bind_method(D_METHOD("set_downsampler_phase", "phase"), &BitcrusherAudioEffect::set_downsampler_phase);
	ClassDB::bind_method(D_METHOD("get_downsampler_phase"), &BitcrusherAudioEffect::get_downsampler_phase);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "downsampler_phase", PROPERTY_HINT_ENUM, "Linear:0,Intermediate:16,Minimum:48"), "set_downsampler_phase", "get_downsampler_phase");

	ClassDB::bind_method(D_METHOD("set_downsampler_steep_filter", "enable"), &BitcrusherAudioEffect::set_downsampler_steep_filter);
	ClassDB::bind_method(D_METHOD("get_downsampler_steep_filter"), &BitcrusherAudioEffect::get_downsampler_steep_filter);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "downsampler_steep_filter"), "set_downsampler_steep_filter", "get_downsampler_steep_filter");

	ADD_GROUP("Upsampler", "upsampler_");

	ClassDB::bind_method(D_METHOD("set_upsampler_quality", "quality"), &BitcrusherAudioEffect::set_upsampler_quality);
	ClassDB::bind_method(D_METHOD("get_upsampler_quality"), &BitcrusherAudioEffect::get_upsampler_quality);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "upsampler_quality", PROPERTY_HINT_ENUM, "Quick:0,Low:1,Medium:2,High:4,VeryHigh:6"), "set_upsampler_quality", "get_upsampler_quality");

	ClassDB::bind_method(D_METHOD("set_upsampler_rolloff", "rolloff"), &BitcrusherAudioEffect::set_upsampler_rolloff);
	ClassDB::bind_method(D_METHOD("get_upsampler_rolloff"), &BitcrusherAudioEffect::get_upsampler_rolloff);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "upsampler_rolloff", PROPERTY_HINT_ENUM, "Small:0,Medium:1,None:2"), "set_upsampler_rolloff", "get_upsampler_rolloff");

	ClassDB::bind_method(D_METHOD("set_upsampler_phase", "phase"), &BitcrusherAudioEffect::set_upsampler_phase);
	ClassDB::bind_method(D_METHOD("get_upsampler_phase"), &BitcrusherAudioEffect::get_upsampler_phase);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "upsampler_phase", PROPERTY_HINT_ENUM, "Linear:0,Intermediate:16,Minimum:48"), "set_upsampler_phase", "get_upsampler_phase");

	ClassDB::bind_method(D_METHOD("set_upsampler_steep_filter", "enable"), &BitcrusherAudioEffect::set_upsampler_steep_filter);
	ClassDB::bind_method(D_METHOD("get_upsampler_steep_filter"), &BitcrusherAudioEffect::get_upsampler_steep_filter);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "upsampler_steep_filter"), "set_upsampler_steep_filter", "get_upsampler_steep_filter");

	ADD_GROUP("Noise Shaping", "noise_shaping_");

	ClassDB::bind_method(D_METHOD("set_noise_shaping_filter", "order"), &BitcrusherAudioEffect::set_noise_shaping_filter);
	ClassDB::bind_method(D_METHOD("get_noise_shaping_filter"), &BitcrusherAudioEffect::get_noise_shaping_filter);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "noise_shaping_filter", PROPERTY_HINT_ENUM, "NoFilter:0,FirstOrder:1,SecondOrder:2"), "set_noise_shaping_filter", "get_noise_shaping_filter");

	ADD_SUBGROUP("Filter Settings", "filter_");

	ClassDB::bind_method(D_METHOD("set_filter_cutoff_frequency", "filter_cutoff_frequency"), &BitcrusherAudioEffect::set_filter_cutoff_frequency);
	ClassDB::bind_method(D_METHOD("get_filter_cutoff_frequency"), &BitcrusherAudioEffect::get_filter_cutoff_frequency);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "filter_cutoff_frequency", PROPERTY_HINT_RANGE, "0,22000,0.01"), "set_filter_cutoff_frequency", "get_filter_cutoff_frequency");

	ClassDB::bind_method(D_METHOD("set_filter_q", "filter_q"), &BitcrusherAudioEffect::set_filter_q);
	ClassDB::bind_method(D_METHOD("get_filter_q"), &BitcrusherAudioEffect::get_filter_q);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "filter_q", PROPERTY_HINT_RANGE, "0,2"), "set_filter_q", "get_filter_q");
}

void BitcrusherAudioEffect::_validate_property(PropertyInfo &p_property) const {
	if (p_property.name == StringName("sample_rate")) {
		auto audio_server = AudioServer::get_singleton();
		if (audio_server != nullptr) {
			float max_rate = audio_server->get_mix_rate();
			float min_rate = max_rate / 32;

			p_property.hint = PROPERTY_HINT_RANGE;
			p_property.hint_string = String::num(min_rate) + "," + String::num(max_rate) + ",0.1";
		}
	}

	// Disable if not using 2nd-order filter
	if (p_property.name == StringName("filter_cutoff_frequency") || p_property.name == StringName("filter_q")) {
		if (noise_shaping_filter != 2) {
			p_property.usage |= PROPERTY_USAGE_READ_ONLY;
		}
	}
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
		unsigned long quality_spec_flags = downsampler_rolloff | (variable_rate ? SOXR_VR : 0);
		soxr_quality_spec_t quality_spec = soxr_quality_spec(quality_spec_recipe, quality_spec_flags);

		auto downsampler = soxr_create(
				variable_rate ? 32 : godot_sample_rate,
				variable_rate ? 1 : target_sample_rate,
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

		if (variable_rate) {
			soxr_set_io_ratio(soxr1.get(), godot_sample_rate / target_sample_rate, 0);
		}
	}

	// Build upsampler
	{
		soxr_io_spec_t io_spec_upsampler = soxr_io_spec(intermediate_data_type, output_data_type);

		unsigned long quality_spec_recipe = upsampler_quality | upsampler_phase | (upsampler_steep_filter ? SOXR_STEEP_FILTER : 0);
		unsigned long quality_spec_flags = upsampler_rolloff | (variable_rate ? SOXR_VR : 0);
		soxr_quality_spec_t quality_spec = soxr_quality_spec(quality_spec_recipe, quality_spec_flags);

		auto upsampler = soxr_create(
				variable_rate ? 1 : target_sample_rate,
				variable_rate ? 1 : godot_sample_rate,
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

		if (variable_rate) {
			soxr_set_io_ratio(soxr2.get(), target_sample_rate / godot_sample_rate, 0);
		}
	}
}
