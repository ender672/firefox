/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZILLA_GFX_STREAMINGSCALER_H_
#define MOZILLA_GFX_STREAMINGSCALER_H_

#include "mozilla/gfx/2D.h"
#include "mozilla/UniquePtr.h"

namespace mozilla {
namespace gfx {

/**
 * StreamingScaler is a streaming image downscaler.
 */
class StreamingScaler final {
 public:
  StreamingScaler();
  ~StreamingScaler();

  StreamingScaler(const StreamingScaler&) = delete;
  StreamingScaler& operator=(const StreamingScaler&) = delete;

  [[nodiscard]] bool Init(const IntSize& aInputSize, const IntSize& aOutputSize,
                          SurfaceFormat aFormat);

  // Reset for a new progressive pass over the same frame dimensions.
  void Reset();

  // Number of input rows the scaler can accept before producing output.
  int Slots() const;
  // Feed one input row to the scaler.
  void FeedRow(const uint8_t* aInputRow);
  // Produce one output row.
  void ProduceRow(uint8_t* aOutputRow);
  // True when all output rows have been produced.
  bool OutputComplete() const;

  struct State {
    int32_t mInHeight;
    int32_t mOutHeight;
    int32_t mInWidth;
    int32_t mOutWidth;
    bool mHasAlpha;
    int32_t mInPos;
    int32_t mOutPos;
    float* mCoeffsY;
    float* mCoeffsX;
    int* mBordersX;
    int* mBordersY;
    float* mSumsY;
    float* mTmpCoeffs;
    uint8_t* mBuf;
    int32_t mSumsYTap;
  };

 private:
  void Free();

#ifdef USE_SSE2
  static void InSse2(State* aOs, const uint8_t* aIn);
  static void OutSse2(State* aOs, uint8_t* aOut);
  static void InAvx2(State* aOs, const uint8_t* aIn);
  static void OutAvx2(State* aOs, uint8_t* aOut);
#endif

#ifdef USE_NEON
  static void InNeon(State* aOs, const uint8_t* aIn);
  static void OutNeon(State* aOs, uint8_t* aOut);
#endif

  State mScaler;
  UniquePtr<uint8_t[]> mBuffer;
  int mBufferSize;
  bool mInitialized;
};

}  // namespace gfx
}  // namespace mozilla

#endif  // MOZILLA_GFX_STREAMINGSCALER_H_
