#include "bitcrusher_audio_effect_instance.hpp"
#include "bitcrusher_audio_effect.hpp"

#include <algorithm>

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

	auto mute_before_return = [&] {
		float *output = reinterpret_cast<float *>(p_dst_buffer);
		std::fill(output, output + 2 * p_frame_count, 0);
	};

	auto ratio = base->godot_sample_rate / base->target_sample_rate;
	intermediate_buffer.resize(p_frame_count);
	intermediate_buffer2.resize(ceil(ratio * p_frame_count));

	size_t idone;
	size_t odone;
	soxr_error_t error;

	error = soxr_process(
			base->soxr1.get(),
			p_src_buffer,
			p_frame_count,
			&idone,
			reinterpret_cast<void *>(intermediate_buffer.data()),
			intermediate_buffer.size(),
			&odone);
	if (error) {
		String msg = "Error during downsampling: ";
		msg += soxr_strerror(error);
		ERR_FAIL_EDMSG(msg);
	}
	// stat1.push(idone, odone);

	size_t written = odone;
	error = soxr_process(
			base->soxr2.get(),
			reinterpret_cast<void *>(intermediate_buffer.data()),
			written,
			&idone,
			reinterpret_cast<void *>(intermediate_buffer2.data()),
			intermediate_buffer2.size(),
			&odone);
	if (error) {
		String msg = "Error during upsampling: ";
		msg += soxr_strerror(error);
		ERR_FAIL_EDMSG(msg);
	}
	// stat2.push(idone, odone);

	if (odone > 0) {
		samples_in_buffer += odone;
		output_deque.push_back(intermediate_buffer2);

		auto &back = output_deque.back();
		back.resize(odone);
	}

	if (!started) {
		if (samples_in_buffer < Statistics::WINDOW * p_frame_count) {
			mute_before_return();
			return;
		} else {
			started = true;
		}
	}

	size_t index = 0;
	int32_t remaining = p_frame_count;
	while (!output_deque.empty() && remaining > 0) {
		auto &front = output_deque.front();
		if (front.size() > remaining) {
			std::copy(front.begin(), front.begin() + remaining, p_dst_buffer + index);
			front.erase(front.begin(), front.begin() + remaining);
			index += remaining;
			remaining = 0;
		} else {
			std::copy(front.begin(), front.end(), p_dst_buffer + index);
			index += front.size();
			remaining -= front.size();
			output_deque.pop_front();
		}
	}
}