#include "custom_audio_effect_instance.hpp"
#include "custom_audio_effect.hpp"
#include "reexport.h"

#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

void CustomAudioEffectInstance::_process(const void *p_src_buffer, AudioFrame *p_dst_buffer, int32_t p_frame_count) {
	if (base.is_null()) {
		ERR_FAIL_EDMSG("Invalid AudioEffect reference");
	} else {
		// If Godot's sample rate changed
		if (base->sample_rate_changed()) {
			base->build_resampler();
		}

		int num_input_frames = p_frame_count;
		int num_output_frames = p_frame_count;

		// If resampler is initialized
		if (base->resampler.get() != nullptr) {
			double sample_ratio = base->dest_rate / base->source_rate;
			auto buf_size = static_cast<int>(floor(sample_ratio * num_output_frames));
			resampler_buf.resize(buf_size);

			resampleProcessInterleaved(
					base->resampler.get(),
					reinterpret_cast<const float *>(p_src_buffer),
					num_input_frames,
					reinterpret_cast<float *>(resampler_buf.data()),
					buf_size,
					0.0 /* not really used */);

			resampleProcessInterleaved(
					base->resampler2.get(),
					reinterpret_cast<const float *>(resampler_buf.data()),
					buf_size,
					reinterpret_cast<float *>(p_dst_buffer),
					num_output_frames,
					0.0 /* not really used */);
		}

		if (base->output_bits != 32) {
			// Simple bitcrusher
			float *ptr = reinterpret_cast<float *>(p_dst_buffer);
			float step = base->output_bits_step;

			float crushed_sample;
			for (int i = 0; i < 2 * p_frame_count; i++) {
				ptr[i] = floorf(ptr[i] / step) * step;
			}
		}
	}
}
