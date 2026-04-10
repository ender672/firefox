/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef GFX_2D_STREAMINGSCALER_H_
#define GFX_2D_STREAMINGSCALER_H_

#include "mozilla/gfx/2D.h"
#include "mozilla/UniquePtr.h"

namespace mozilla::gfx {

/**
 * StreamingScaler is a streaming image downscaler.
 */
class StreamingScaler {
 public:
  enum class Colorspace {
    Bgra = 0x0604,
    Bgrx = 0x0704,
  };

  /**
   * Struct to hold state for scaling.
   */
  struct State {
    int mInHeight;
    int mOutHeight;
    int mInWidth;
    int mOutWidth;
    Colorspace mCs;
    int mInPos;
    int mOutPos;
    float* mCoeffsY;
    float* mCoeffsX;
    int* mBordersX;
    int* mBordersY;
    float* mSumsY;
    float* mTmpCoeffs;
    void* mBuf;
    int mSumsYTap;
  };

  StreamingScaler();
  ~StreamingScaler();

  StreamingScaler(const StreamingScaler&) = delete;
  StreamingScaler& operator=(const StreamingScaler&) = delete;

  [[nodiscard]] bool Init(int32_t aInputWidth, int32_t aInputHeight,
                          int32_t aOutputWidth, int32_t aOutputHeight,
                          SurfaceFormat aFormat);

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

#ifdef USE_SSE2
  static int InSse2(State* aOs, const unsigned char* aIn);
  static int OutSse2(State* aOs, unsigned char* aOut);
  static int InAvx2(State* aOs, const unsigned char* aIn);
  static int OutAvx2(State* aOs, unsigned char* aOut);
#endif

#ifdef USE_NEON
  static int InNeon(State* aOs, const unsigned char* aIn);
  static int OutNeon(State* aOs, unsigned char* aOut);
#endif

 private:
  State mScaler;
  UniquePtr<uint8_t[]> mBuffer;
  int mBufferSize;
  bool mInitialized;
};

}  // namespace mozilla::gfx

#endif  // GFX_2D_STREAMINGSCALER_H_
