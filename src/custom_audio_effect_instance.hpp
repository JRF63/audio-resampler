#pragma once

#include <godot_cpp/classes/audio_effect_instance.hpp>

namespace godot {

class CustomAudioEffect;

class CustomAudioEffectInstance : public AudioEffectInstance {
	GDCLASS(CustomAudioEffectInstance, AudioEffectInstance)
	friend class CustomAudioEffect;
	Ref<CustomAudioEffect> base;

protected:
	static void _bind_methods() {}

public:
	virtual void _process(const void *p_src_buffer, AudioFrame *p_dst_buffer, int32_t p_frame_count) override;
};

} //namespace godot