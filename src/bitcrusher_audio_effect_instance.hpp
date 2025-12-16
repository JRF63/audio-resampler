#pragma once

#include <godot_cpp/classes/audio_effect_instance.hpp>

#include <deque>
#include <vector>

namespace godot {

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
	std::vector<AudioFrame> resampler_scratch;
	size_t index;

	std::deque<std::array<AudioFrame, 512>> output_buffer;
};

} //namespace godot