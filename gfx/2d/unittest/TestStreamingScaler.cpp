/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "TestStreamingScaler.h"

#include "StreamingScaler.h"

using namespace mozilla::gfx;

TestStreamingScaler::TestStreamingScaler() {
  REGISTER_TEST(TestStreamingScaler, BasicScale);
  REGISTER_TEST(TestStreamingScaler, ProgressiveScale);
  REGISTER_TEST(TestStreamingScaler, FormatSupport);
}

void TestStreamingScaler::BasicScale() {
  StreamingScaler scaler;
  VERIFY(scaler.Init(2, 2, 1, 1, SurfaceFormat::B8G8R8A8));

  uint32_t input[4] = {0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00};
  uint32_t output[1] = {0};

  VERIFY(scaler.Slots() == 2);
  scaler.FeedRow(reinterpret_cast<uint8_t*>(&input[0]));
  VERIFY(scaler.Slots() == 1);
  scaler.FeedRow(reinterpret_cast<uint8_t*>(&input[2]));

  VERIFY(scaler.Slots() == 0);
  VERIFY(!scaler.OutputComplete());
  scaler.ProduceRow(reinterpret_cast<uint8_t*>(&output[0]));
  VERIFY(scaler.OutputComplete());

  // Should be green.
  VERIFY(output[0] == 0xff00ff00);
}

void TestStreamingScaler::ProgressiveScale() {
  StreamingScaler scaler;
  VERIFY(scaler.Init(2, 2, 1, 1, SurfaceFormat::B8G8R8A8));

  uint32_t input[4] = {0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00};
  uint32_t output[1] = {0};

  // Pass 1
  scaler.FeedRow(reinterpret_cast<uint8_t*>(&input[0]));
  scaler.FeedRow(reinterpret_cast<uint8_t*>(&input[2]));
  scaler.ProduceRow(reinterpret_cast<uint8_t*>(&output[0]));
  VERIFY(output[0] == 0xff00ff00);

  // Pass 2 - Reset should allow reuse
  scaler.Reset();
  output[0] = 0;
  scaler.FeedRow(reinterpret_cast<uint8_t*>(&input[0]));
  scaler.FeedRow(reinterpret_cast<uint8_t*>(&input[2]));
  scaler.ProduceRow(reinterpret_cast<uint8_t*>(&output[0]));
  VERIFY(output[0] == 0xff00ff00);
}

void TestStreamingScaler::FormatSupport() {
  StreamingScaler scaler;
  // Supported formats
  VERIFY(scaler.Init(10, 10, 5, 5, SurfaceFormat::B8G8R8A8));
  VERIFY(scaler.Init(10, 10, 5, 5, SurfaceFormat::B8G8R8X8));
  VERIFY(scaler.Init(10, 10, 5, 5, SurfaceFormat::R8G8B8A8));
  VERIFY(scaler.Init(10, 10, 5, 5, SurfaceFormat::R8G8B8X8));

  // Unsupported formats (currently only 4-component formats are supported by
  // the wrapper, matching what was in image/DownscalingFilter.h)
  VERIFY(!scaler.Init(10, 10, 5, 5, SurfaceFormat::A8));
  VERIFY(!scaler.Init(10, 10, 5, 5, SurfaceFormat::R5G6B5_UINT16));
}
