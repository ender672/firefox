/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_image_OilDownscaler_h
#define mozilla_image_OilDownscaler_h

#include "mozilla/gfx/2D.h"
#include "mozilla/UniquePtr.h"

extern "C" {
#include "oil_resample.h"
}

namespace mozilla {
namespace image {

// Thin wrapper around liboil's streaming scaler for use in DownscalingFilter.
// Maps the row-by-row SurfacePipe pattern to liboil's
// oil_scale_in()/oil_scale_out() API.
//
// Manages the scaler's backing buffer using Firefox's allocator so that
// allocations are tracked via jemalloc and the buffer can be reused across
// progressive passes without re-allocating.
class OilDownscaler {
 public:
  OilDownscaler();
  ~OilDownscaler();

  OilDownscaler(const OilDownscaler&) = delete;
  OilDownscaler& operator=(const OilDownscaler&) = delete;

  bool Init(int32_t aInputWidth, int32_t aInputHeight, int32_t aOutputWidth,
            int32_t aOutputHeight, gfx::SurfaceFormat aFormat);

  // Reset for a new progressive pass over the same frame dimensions.
  void Reset();

  void Free();

  // Number of input rows the scaler can accept before producing output.
  int Slots() const;
  // Feed one input row to the scaler.
  void FeedRow(const uint8_t* aInputRow);
  // Produce one output row.
  void ProduceRow(uint8_t* aOutputRow);
  // True when all output rows have been produced.
  bool OutputComplete() const;

 private:
  static void EnsureGlobalInit();

  oil_scale mScaler;
  UniquePtr<uint8_t[]> mBuffer;
  int mBufferSize;
  bool mInitialized;
};

}  // namespace image
}  // namespace mozilla

#endif  // mozilla_image_OilDownscaler_h
