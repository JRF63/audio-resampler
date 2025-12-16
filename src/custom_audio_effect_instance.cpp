#include "custom_audio_effect_instance.hpp"
#include "custom_audio_effect.hpp"

#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

void CustomAudioEffectInstance::_process(const void *p_src_buffer, AudioFrame *p_dst_buffer, int32_t p_frame_count) {
	if (base.is_null()) {
		ERR_FAIL_EDMSG("Invalid AudioEffect reference");
	} else {
		constexpr int NUM_CHANNELS = 2;

		const float (*src)[NUM_CHANNELS] = static_cast<const float (*)[NUM_CHANNELS]>(p_src_buffer);
		float (*dst)[NUM_CHANNELS] = reinterpret_cast<float (*)[NUM_CHANNELS]>(p_dst_buffer);

		const float ratio = base->sample_rate_ratio;
		const float alpha = base->lowpass_alpha;
		const float scale = base->bit_depth_step;

		for (int i = 0; i < p_frame_count; i++) {
			for (int ch = 0; ch < NUM_CHANNELS; ch++) {
				ChannelFilter &st = base->channel_filters[ch];

				base->channel_filters[ch].phase += ratio;

				// Fractional sample-and-hold
				if (base->channel_filters[ch].phase >= 1.0f) {
					base->channel_filters[ch].phase = -1.0f;

					float x = src[i][ch];

					// Triangle PDF dithering
					if (base->dither_scale > 0.0f) {
						float u1 = base->channel_rngs[ch].generate();
						float u2 = base->channel_rngs[ch].generate();

						// float in [-1, +1)
						float diff = u1 - u2;

						// float in [-1 LSB, +1 LSB)
						float dither = diff * scale * base->dither_scale;

						x += dither;
					}

					// 2nd-order lowpass (two cascaded single-pole filters)
					st.lp1 = alpha * x + (1.0f - alpha) * st.lp1;
					st.lp2 = alpha * st.lp1 + (1.0f - alpha) * st.lp2;
					x = st.lp2;

					if (base->noise_shaping_k > 0.0f) {
						x -= base->noise_shaping_k * base->channel_filters[ch].quant_error;
					}

					// Bit depth reduction
					// st.hold_past = st.hold;
					st.hold = floorf(x / scale) * scale;

					if (base->noise_shaping_k > 0.0f) {
						base->channel_filters[ch].quant_error = st.hold - x;
					}
				}

				dst[i][ch] = st.hold;
			}
		}
	}
}
