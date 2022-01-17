#pragma once

#include "wreath/stereo_looper.h"

namespace wreath
{
    constexpr float kMinSpeedMult{0.02f};
    constexpr float kMaxSpeedMult{50.f};

    StereoLooper looper;
}