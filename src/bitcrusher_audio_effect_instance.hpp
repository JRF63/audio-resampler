#pragma once

#include <godot_cpp/classes/audio_effect_instance.hpp>

#include <deque>
#include <vector>

namespace godot {

class Statistics {
public:
	static constexpr size_t WINDOW = 16;

	std::deque<uint64_t> consumed;
	std::deque<uint64_t> produced;

	void push(uint64_t a, uint64_t b) {
		auto push = [](std::deque<uint64_t> &deque, uint64_t value) {
			deque.push_back(value);
			if (deque.size() > Statistics::WINDOW) {
				deque.pop_front();
			}
		};

		push(consumed, a);
		push(produced, b);
	}

	double average(const std::deque<uint64_t> &deque) const {
		uint64_t total = 0;
		for (auto val : deque) {
			total += val;
		}
		return static_cast<double>(total) / deque.size();
	}

	double ave_consumed() const {
		return average(consumed);
	}

	double ave_produced() const {
		return average(produced);
	}
};

class BitcrusherAudioEffect;

class BitcrusherAudioEffectInstance : public AudioEffectInstance {
	GDCLASS(BitcrusherAudioEffectInstance, AudioEffectInstance)
	friend class BitcrusherAudioEffect;
	Ref<BitcrusherAudioEffect> base;

protected:
	static void _bind_methods() {}

public:
	virtual void _process(const void *p_src_buffer, AudioFrame *p_dst_buffer, int32_t p_frame_count) override;

private:
	std::vector<AudioFrame> intermediate_buffer;
	std::vector<AudioFrame> intermediate_buffer2;
	std::deque<std::vector<AudioFrame>> output_deque;
	size_t samples_in_buffer;
	bool started = false;

	// Statistics stat1;
	// Statistics stat2;
};

} //namespace godot