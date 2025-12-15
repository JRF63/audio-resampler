// Reexport to fix missing include guard
#pragma once

#include "resampler.h"

// Code below is placed here just to tidy up custom_audio_effect.hpp
#include <memory>

template <typename T, void (*FreeFn)(T *)>
struct CDeleter {
	void operator()(T *p) const noexcept {
		if (p) {
			FreeFn(p);
		}
	}
};

using ResamplePtr = std::unique_ptr<Resample, CDeleter<Resample, resampleFree>>;