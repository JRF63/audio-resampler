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

double BitcrusherAudioEffect::get_sample_rate() const {
	return target_sample_rate;
}

void BitcrusherAudioEffect::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_sample_rate", "sample_rate"), &BitcrusherAudioEffect::set_sample_rate);
	ClassDB::bind_method(D_METHOD("get_sample_rate"), &BitcrusherAudioEffect::get_sample_rate);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sample_rate", PROPERTY_HINT_RANGE, "0,48000,0.1"), "set_sample_rate", "get_sample_rate");
}

void BitcrusherAudioEffect::create_resamplers() {
	soxr_error_t error;

	godot_sample_rate = AudioServer::get_singleton()->get_mix_rate();
	if (target_sample_rate == 0.0) {
		target_sample_rate = godot_sample_rate;
	}

	unsigned num_channels = 2; // Always 2

	unsigned num_threads = 1;
	soxr_runtime_spec_t runtime_spec = soxr_runtime_spec(num_threads);

	// 32-bit float interleaved
	soxr_datatype_t input_data_type = SOXR_FLOAT32_I;
	soxr_datatype_t intermediate_data_type = SOXR_FLOAT32_I;
	soxr_datatype_t output_data_type = SOXR_FLOAT32_I;

	{
		soxr_io_spec_t io_spec_downsampler = soxr_io_spec(input_data_type, intermediate_data_type);

		unsigned long quality_spec_recipe = SOXR_HQ;
		unsigned long quality_spec_flags = 0;
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

	{
		soxr_io_spec_t io_spec_upsampler = soxr_io_spec(intermediate_data_type, output_data_type);

		unsigned long quality_spec_recipe = SOXR_HQ;
		unsigned long quality_spec_flags = 0;
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
