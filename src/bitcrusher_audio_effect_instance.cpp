#include "bitcrusher_audio_effect_instance.hpp"
#include "bitcrusher_audio_effect.hpp"

#include <algorithm>

using namespace godot;

inline float quantize(float x, float dither, double scale, double scale_inv) {
	double y = static_cast<double>(x) + dither;
	return floor(y * scale_inv) * scale;
}

inline float gen_dither(float dither_mult, Rng &rng, float scale) {
	if (dither_mult > 0.0f) {
		float u1 = rng.generate();
		float u2 = rng.generate();

		// float in [-1, +1)
		float diff = u1 - u2;

		// float in [-1 LSB, +1 LSB)
		return diff * scale * dither_mult;
	} else {
		return 0.0f;
	}
}

void BitcrusherAudioEffectInstance::_process(const void *p_src_buffer, AudioFrame *p_dst_buffer, int32_t p_frame_count) {
	if (base.is_null()) {
		ERR_FAIL_EDMSG("Invalid AudioEffect reference");
		return;
	}

	if (base->soxr1 == nullptr || base->soxr2 == nullptr) {
		ERR_FAIL_EDMSG("Resamplers not initialized");
		return;
	}

	auto mute_before_return = [&] {
		float *output = reinterpret_cast<float *>(p_dst_buffer);
		std::fill(output, output + 2 * p_frame_count, 0);
	};

	auto desired_samples = base->num_samples_before_starting;

	{
		auto ratio = base->godot_sample_rate / base->target_sample_rate;

		// Could be `p_frame_count / ratio` but we just make it larger
		downsampler_output.resize(p_frame_count);

		// Should be more than `ratio * downsampler_output.size()` to avoid bottlenecking upsampler
		upsampler_output.resize(ceil(ratio * downsampler_output.size()) + p_frame_count);

		// Avoid overflow when inserting `upsampler_output`
		ring_buffer.set_capacity(desired_samples + 2 * upsampler_output.size());
	}

	size_t idone;
	size_t odone;
	soxr_error_t error;

	error = soxr_process(
			base->soxr1.get(),
			p_src_buffer,
			p_frame_count,
			&idone,
			reinterpret_cast<void *>(downsampler_output.data()),
			downsampler_output.size(),
			&odone);
	if (error) {
		String msg = "Error during downsampling: ";
		msg += soxr_strerror(error);
		ERR_FAIL_EDMSG(msg);
	}

	// Process downsampled data
	if (odone > 0) {
		constexpr int NUM_CHANNELS = 2;
		const auto scale = base->bit_depth_step;
		const auto scale_inv = base->bit_depth_step_inv;
		float (*dst)[NUM_CHANNELS] = reinterpret_cast<float (*)[NUM_CHANNELS]>(downsampler_output.data());

		for (int i = 0; i < odone; i++) {
			for (int ch = 0; ch < NUM_CHANNELS; ch++) {
				ChannelFilter &st = base->channel_filters[ch];

				float x = dst[i][ch];

				// Triangle PDF dithering
				float dither = gen_dither(base->dither_scale, base->channel_rngs[ch], scale);

				// Noise shaping
				float shaped = base->channel_filters[ch].process(base->noise_shaping_filter);
				x -= shaped;

				// Bit depth reduction
				float quantized = quantize(x, dither, scale, scale_inv);

				if (base->noise_shaping_filter != NoiseShapingFilter::NO_FILTER) {
					auto error = quantized - x;
					base->channel_filters[ch].save_quant_error(error);
				}

				dst[i][ch] = quantized;
			}
		}
	}

	// Upsample back for playback in Godot
	size_t written = odone;
	error = soxr_process(
			base->soxr2.get(),
			reinterpret_cast<void *>(downsampler_output.data()),
			written,
			&idone,
			reinterpret_cast<void *>(upsampler_output.data()),
			upsampler_output.size(),
			&odone);
	if (error) {
		String msg = "Error during upsampling: ";
		msg += soxr_strerror(error);
		ERR_FAIL_EDMSG(msg);
	}

	if (odone > 0) {
		if (ring_buffer.reserve() < odone) {
			ERR_FAIL_EDMSG("Ring buffer capacity too small");
		}
		ring_buffer.insert(ring_buffer.end(), upsampler_output.begin(), upsampler_output.begin() + odone);
	}

	// Don't start until enough samples have been generated
	if (!started) {
		if (ring_buffer.size() < desired_samples) {
			mute_before_return();
			return;
		} else {
			started = true;
		}
	}

	if (ring_buffer.size() >= p_frame_count) {
		std::copy(ring_buffer.begin(), ring_buffer.begin() + p_frame_count, p_dst_buffer);
		ring_buffer.erase_begin(p_frame_count);
	} else {
		mute_before_return();
		WARN_PRINT_ED("Output buffer too small - skipping samples");
	}
}