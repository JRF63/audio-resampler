#include "custom_audio_effect_instance.hpp"
#include "custom_audio_effect.hpp"
#include "resampler_export.h"

#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

void CustomAudioEffectInstance::_process(const void *p_src_buffer, AudioFrame *p_dst_buffer, int32_t p_frame_count) {
	if (base.is_null()) {
		ERR_FAIL_EDMSG("Invalid AudioEffect reference");
	} else {
		int num_input_frames = p_frame_count;
		int num_output_frames = p_frame_count;

		auto result = resampleProcessInterleaved(base->resampler.get(), reinterpret_cast<const float *>(p_src_buffer), num_input_frames, reinterpret_cast<float *>(p_dst_buffer), num_output_frames, base->ratio);
		if (result.input_used != num_input_frames && result.output_generated != num_output_frames) {
			ERR_FAIL_EDMSG("Error resampling");
		}
	}
}
