#include "bitcrusher_audio_effect_instance.hpp"
#include "bitcrusher_audio_effect.hpp"

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

	intermediate_buffer.resize(p_frame_count);

	size_t idone;
	size_t odone;
	soxr_error_t error;
	auto intermediate = reinterpret_cast<void *>(intermediate_buffer.data());
	auto output = reinterpret_cast<char *>(p_dst_buffer);

	error = soxr_process(
			base->soxr1.get(),
			p_src_buffer,
			p_frame_count,
			&idone,
			intermediate,
			p_frame_count,
			&odone);
	if (error) {
		String msg = "Error during downsampling: ";
		msg += soxr_strerror(error);
		ERR_FAIL_EDMSG(msg);
	}

	stat1.push(idone, odone);

	samples_in_buffer += odone;
	foo.push_back(intermediate_buffer);

	if (!started) {
		if (samples_in_buffer < 8 * p_frame_count) {
			for (int i = 0; i < p_frame_count; i++) {
				p_dst_buffer[i].left = 0;
				p_dst_buffer[i].right = 0;
			}
			return;
		} else {
			started = true;
		}
	}

	size_t idone_acc = 0;
	size_t odone_acc = 0;
	size_t offset = 0;
	while (!foo.empty() && offset < p_frame_count) {
		auto &first = foo[0];
		error = soxr_process(
				base->soxr2.get(),
				reinterpret_cast<void *>(first.data()),
				first.size(),
				&idone,
				output + offset,
				p_frame_count - offset,
				&odone);
		if (error) {
			String msg = "Error during upsampling: ";
			msg += soxr_strerror(error);
			ERR_FAIL_EDMSG(msg);
		}
		foo.pop_front();

		offset += odone;
		idone_acc += idone;
		odone_acc += odone;
	}

	stat2.push(idone_acc, odone_acc);

	if (odone == 0) {
		for (int i = 0; i < p_frame_count; i++) {
			p_dst_buffer[i].left = 0;
			p_dst_buffer[i].right = 0;
		}
	}

	String debug = "";
	debug += String::num_real(stat1.ave_consumed());
	debug += ", ";
	debug += String::num_real(stat1.ave_produced());
	debug += ", ";
	debug += String::num_real(stat2.ave_consumed());
	debug += ", ";
	debug += String::num_real(stat2.ave_produced());
	WARN_PRINT_ED(debug);
}