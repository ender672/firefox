/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "StreamingScaler.h"

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

StreamingScaler::StreamingScaler() : mScaler{}, mBufferSize(0), mInitialized(false) {}

StreamingScaler::~StreamingScaler() { Free(); }

void StreamingScaler::Free() {
  if (mInitialized) {
    OilScaleFree(&mScaler);
    mInitialized = false;
  }
  mBuffer = nullptr;
  mBufferSize = 0;
}

bool StreamingScaler::Init(int32_t aInputWidth, int32_t aInputHeight,
                           int32_t aOutputWidth, int32_t aOutputHeight,
                           SurfaceFormat aFormat) {
  Free();

  OilColorspace cs;
  switch (aFormat) {
    case SurfaceFormat::B8G8R8A8:
    case SurfaceFormat::B8G8R8X8:
    case SurfaceFormat::R8G8B8A8:
    case SurfaceFormat::R8G8B8X8:
      // These are all 4-component; resampling is channel-independent
      // so byte order doesn't matter. Use RGBX for opaque to skip alpha
      // premultiply.
      cs = IsOpaque(aFormat) ? OilColorspace::Bgrx : OilColorspace::Bgra;
      break;
    default:
      return false;
  }

  int allocSize =
      OilScaleAllocSize(aInputHeight, aOutputHeight, aInputWidth, aOutputWidth, cs);
  if (allocSize <= 0) {
    return false;
  }

  mBuffer.reset(new (fallible) uint8_t[allocSize]);
  if (MOZ_UNLIKELY(!mBuffer)) {
    return false;
  }
  memset(mBuffer.get(), 0, allocSize);
  mBufferSize = allocSize;

  int ret = OilScaleInitAllocated(&mScaler, aInputHeight, aOutputHeight,
                                  aInputWidth, aOutputWidth, cs, mBuffer.get());
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
  return OilScaleSlots(const_cast<OilScale*>(&mScaler));
}

void StreamingScaler::FeedRow(const uint8_t* aInputRow) {
  MOZ_ASSERT(mInitialized);
  unsigned char* in = const_cast<unsigned char*>(aInputRow);
#ifdef USE_SSE2
  if (mozilla::supports_avx2()) {
    (void)OilScaleInAvx2(&mScaler, in);
    return;
  }
  if (mozilla::supports_sse2()) {
    (void)OilScaleInSse2(&mScaler, in);
    return;
  }
#elif defined(USE_NEON)
  if (mozilla::supports_neon()) {
    (void)OilScaleInNeon(&mScaler, in);
    return;
  }
#endif
  (void)OilScaleIn(&mScaler, in);
}

void StreamingScaler::ProduceRow(uint8_t* aOutputRow) {
  MOZ_ASSERT(mInitialized);
#ifdef USE_SSE2
  if (mozilla::supports_avx2()) {
    (void)OilScaleOutAvx2(&mScaler, aOutputRow);
    return;
  }
  if (mozilla::supports_sse2()) {
    (void)OilScaleOutSse2(&mScaler, aOutputRow);
    return;
  }
#elif defined(USE_NEON)
  if (mozilla::supports_neon()) {
    (void)OilScaleOutNeon(&mScaler, aOutputRow);
    return;
  }
#endif
  (void)OilScaleOut(&mScaler, aOutputRow);
}

bool StreamingScaler::OutputComplete() const {
  MOZ_ASSERT(mInitialized);
  return mScaler.mOutPos >= mScaler.mOutHeight;
}

void StreamingScaler::Reset() {
  if (mInitialized) {
    memset(mBuffer.get(), 0, mBufferSize);
    int ret = OilScaleInitAllocated(&mScaler, mScaler.mInHeight, mScaler.mOutHeight,
                                    mScaler.mInWidth, mScaler.mOutWidth,
                                    mScaler.mCs, mBuffer.get());
    if (ret != 0) {
      mInitialized = false;
    }
  }
}

}  // namespace gfx
}  // namespace mozilla
