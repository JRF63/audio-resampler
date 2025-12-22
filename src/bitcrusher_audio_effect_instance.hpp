#pragma once

#include <godot_cpp/classes/audio_effect_instance.hpp>

#include <boost/circular_buffer.hpp>

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
	std::vector<AudioFrame> downsampler_output;
	std::vector<AudioFrame> upsampler_output;
	boost::circular_buffer<AudioFrame> ring_buffer;

	size_t samples_in_buffer;
	bool started = false;
};

} //namespace godot