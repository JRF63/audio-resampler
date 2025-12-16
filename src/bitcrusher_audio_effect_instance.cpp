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
	resampler_buf.resize(p_frame_count);

	size_t idone;
	size_t odone;
	soxr_error_t error;
	auto intermediate_buffer = reinterpret_cast<void *>(resampler_buf.data());
	auto output_buffer = reinterpret_cast<void *>(p_dst_buffer);

	error = soxr_process(
			base->soxr1.get(),
			p_src_buffer,
			p_frame_count,
			&idone,
			intermediate_buffer,
			p_frame_count,
			&odone);
	if (error) {
		String msg = "Error during downsampling: ";
		msg += soxr_strerror(error);
		ERR_FAIL_EDMSG(msg);
	}

	size_t downsample_frame_count = odone;

	error = soxr_process(
			base->soxr2.get(),
			intermediate_buffer,
			downsample_frame_count,
			&idone,
			output_buffer,
			p_frame_count,
			&odone);
	if (error) {
		String msg = "Error during upsampling: ";
		msg += soxr_strerror(error);
		ERR_FAIL_EDMSG(msg);
	}
}