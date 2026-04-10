/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "StreamingScaler.h"

#include <mutex>
#include <cstring>

#include "mozilla/fallible.h"

#ifdef USE_SSE2
#  include "mozilla/SSE.h"
#endif

#ifdef USE_NEON
#  include "mozilla/arm.h"
#endif

namespace mozilla {
namespace gfx {

/* static */
void StreamingScaler::EnsureGlobalInit() {
  static std::once_flag sOnce;
  std::call_once(sOnce, [] { oil_global_init(); });
}

StreamingScaler::StreamingScaler() : mBufferSize(0), mInitialized(false) {
  memset(&mScaler, 0, sizeof(mScaler));
}

StreamingScaler::~StreamingScaler() { Free(); }

void StreamingScaler::Free() {
  if (mInitialized) {
    // We own the buffer, so don't call oil_scale_free() which would
    // free() it. Just clear the struct and release our buffer.
    memset(&mScaler, 0, sizeof(mScaler));
    mInitialized = false;
  }
  mBuffer = nullptr;
  mBufferSize = 0;
}

bool StreamingScaler::Init(int32_t aInputWidth, int32_t aInputHeight,
                           int32_t aOutputWidth, int32_t aOutputHeight,
                           SurfaceFormat aFormat) {
  Free();
  EnsureGlobalInit();

  oil_colorspace cs;
  switch (aFormat) {
    case SurfaceFormat::B8G8R8A8:
    case SurfaceFormat::B8G8R8X8:
    case SurfaceFormat::R8G8B8A8:
    case SurfaceFormat::R8G8B8X8:
      // These are all 4-component; resampling is channel-independent
      // so byte order doesn't matter. Use RGBX for opaque to skip alpha
      // premultiply.
      cs = IsOpaque(aFormat) ? OIL_CS_RGBX_NOGAMMA : OIL_CS_RGBA_NOGAMMA;
      break;
    default:
      return false;
  }

  int allocSize = oil_scale_alloc_size(aInputHeight, aOutputHeight,
                                       aInputWidth, aOutputWidth, cs);
  if (allocSize <= 0) {
    return false;
  }

  mBuffer.reset(new (fallible) uint8_t[allocSize]);
  if (MOZ_UNLIKELY(!mBuffer)) {
    return false;
  }
  memset(mBuffer.get(), 0, allocSize);
  mBufferSize = allocSize;

  int ret = oil_scale_init_allocated(&mScaler, aInputHeight, aOutputHeight,
                                     aInputWidth, aOutputWidth, cs,
                                     mBuffer.get());
  if (ret != 0) {
    mBuffer = nullptr;
    mBufferSize = 0;
    return false;
  }

  mInitialized = true;
  return true;
}

int StreamingScaler::Slots() const {
  MOZ_ASSERT(mInitialized);
  return oil_scale_slots(const_cast<oil_scale*>(&mScaler));
}

void StreamingScaler::FeedRow(const uint8_t* aInputRow) {
  MOZ_ASSERT(mInitialized);
  unsigned char* in = const_cast<unsigned char*>(aInputRow);
#ifdef USE_SSE2
  if (mozilla::supports_avx2()) {
    oil_scale_in_avx2(&mScaler, in);
    return;
  }
  if (mozilla::supports_sse2()) {
    oil_scale_in_sse2(&mScaler, in);
    return;
  }
#elif defined(USE_NEON)
  if (mozilla::supports_neon()) {
    oil_scale_in_neon(&mScaler, in);
    return;
  }
#endif
  oil_scale_in(&mScaler, in);
}

void StreamingScaler::ProduceRow(uint8_t* aOutputRow) {
  MOZ_ASSERT(mInitialized);
#ifdef USE_SSE2
  if (mozilla::supports_avx2()) {
    oil_scale_out_avx2(&mScaler, aOutputRow);
    return;
  }
  if (mozilla::supports_sse2()) {
    oil_scale_out_sse2(&mScaler, aOutputRow);
    return;
  }
#elif defined(USE_NEON)
  if (mozilla::supports_neon()) {
    oil_scale_out_neon(&mScaler, aOutputRow);
    return;
  }
#endif
  oil_scale_out(&mScaler, aOutputRow);
}

bool StreamingScaler::OutputComplete() const {
  MOZ_ASSERT(mInitialized);
  return mScaler.out_pos >= mScaler.out_height;
}

void StreamingScaler::Reset() {
  if (mInitialized) {
    memset(mBuffer.get(), 0, mBufferSize);
    int ret = oil_scale_init_allocated(
        &mScaler, mScaler.in_height, mScaler.out_height, mScaler.in_width,
        mScaler.out_width, mScaler.cs, mBuffer.get());
    if (ret != 0) {
      mInitialized = false;
    }
  }
}

}  // namespace gfx
}  // namespace mozilla
