/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef GFX_2D_UNITTEST_TESTSTREAMINGSCALER_H_
#define GFX_2D_UNITTEST_TESTSTREAMINGSCALER_H_

#include "TestBase.h"

class TestStreamingScaler : public TestBase {
 public:
  TestStreamingScaler();

  void BasicScale();
  void ProgressiveScale();
  void FormatSupport();
};

#endif  // GFX_2D_UNITTEST_TESTSTREAMINGSCALER_H_
