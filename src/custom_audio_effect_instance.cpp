#include "custom_audio_effect_instance.hpp"
#include "custom_audio_effect.hpp"

#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

float catmull_rom(float p0, float p1, float p2, float p3, float t) {
	// Catmull-Rom formula
	float t2 = t * t;
	float t3 = t2 * t;

	return 0.5f * ((2.0f * p1) + (-p0 + p2) * t + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

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

				st.phase += ratio;

				// Fractional sample-and-hold
				if (st.phase >= 1.0f) {
					st.phase = -1.0f;

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
						x -= base->noise_shaping_k * st.quant_error;
					}

					// Bit depth reduction
					st.hold = floorf(x / scale) * scale;

					auto &deque = st.samples;
					deque.push_back(st.hold);
					if (deque.size() > 4) {
						deque.pop_front();
					}
					st.t = 0.0f;

					if (base->noise_shaping_k > 0.0f) {
						st.quant_error = st.hold - x;
					}
				}

				auto &deque = st.samples;
				if (deque.size() == 4) {
					float t = st.t;
					if (t > 1.0f) {
						t = 1.0f;
					}
					dst[i][ch] = catmull_rom(deque[0], deque[1], deque[2], deque[3], t);
				}
				st.t += ratio;

				// dst[i][ch] = st.hold;
			}
		}
	}
}
