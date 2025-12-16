#include "bitcrusher_audio_effect_instance.hpp"
#include "bitcrusher_audio_effect.hpp"

using namespace godot;

void BitcrusherAudioEffectInstance::_process(const void *p_src_buffer, AudioFrame *p_dst_buffer, int32_t p_frame_count) {
	if (base.is_null()) {
		ERR_FAIL_EDMSG("Invalid AudioEffect reference");
		return;
	}

	if (base->soxr1 == nullptr || base->soxr2 == nullptr) {
		ERR_FAIL_EDMSG("Resamplers not initialized");
		return;
	}

	// Guaranteed to be large enough when downsampling
	resampler_scratch.resize(p_frame_count);

	std::array<AudioFrame, 512> foo;

	size_t idone;
	size_t odone;
	soxr_error_t error;
	auto intermediate = reinterpret_cast<void *>(resampler_scratch.data());
	auto output = reinterpret_cast<void *>(foo.data());

	error = soxr_process(
			base->soxr1.get(),
			p_src_buffer,
			p_frame_count,
			&idone,
			intermediate,
			p_frame_count,
			&odone);
	if (error) {
		String msg = "Error during downsampling: ";
		msg += soxr_strerror(error);
		ERR_FAIL_EDMSG(msg);
	}

	// String debug = "";
	// debug += String::num_int64(index++);
	// debug += ", ";
	// debug += String::num_int64(p_frame_count);
	// debug += ", ";
	// debug += String::num_int64(idone);
	// debug += ", ";
	// debug += String::num_int64(odone);
	// debug += ", ";
	// debug += String::num_int64(odone);

	size_t downsample_frame_count = odone;

	error = soxr_process(
			base->soxr2.get(),
			intermediate,
			downsample_frame_count,
			&idone,
			output,
			p_frame_count,
			&odone);
	if (error) {
		String msg = "Error during upsampling: ";
		msg += soxr_strerror(error);
		ERR_FAIL_EDMSG(msg);
	}

	output_buffer.push_back(foo);
	if (output_buffer.size() > 100) {
		index = 1;
	}

	if (index == 1) {
		if (output_buffer.empty()) {
			index = 0;
			return;
		}

		std::array<AudioFrame, 512> &bar = output_buffer[0];

		for (int i = 0; i < 512; i++) {
			p_dst_buffer[i] = bar[i];
		}

		output_buffer.pop_front();
	}

	// debug += ", ";
	// debug += String::num_int64(idone);
	// debug += ", ";
	// debug += String::num_int64(odone);

	// WARN_PRINT_ED(debug);
}