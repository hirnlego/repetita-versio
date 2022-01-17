#pragma once

#include "daisy_versio.h"

namespace wreath
{
    using namespace daisy;

    // The minimum difference in parameter value to be registered.
    constexpr float kMinValueDelta{0.01f};
    // The trigger threshold value.
    constexpr float kTriggerThres{0.3f};
    // Maximum BPM supported.
    constexpr int kMaxBpm{300};

    DaisyVersio hw;

    char switch1Pos{};
    size_t switch1Begin{};
    size_t switch1End{};
    char switch2Pos{};
    size_t switch2Begin{};
    size_t switch2End{};

    static size_t begin{};
    static size_t end{};

    size_t ms{};
    size_t cv1Bpm{};

    inline static int CalculateBpm()
    {
        end = ms;
        // Handle the ms reset.
        if (end < begin)
        {
            end += 10000;
        }

        return std::round((1000.f / (end - begin)) * 60);
    }

    inline void InitHw(float knobSlewSeconds, float cvSlewSeconds)
    {
        hw.Init(true);
        hw.StartAdc();
    }

    inline void UpdateClock()
    {
        if (ms > 10000)
        {
            ms = 0;
        }
        ms++;
    }

    inline void ProcessControls()
    {
        hw.ProcessAllControls();
        hw.tap.Debounce();
/*
        if (switch1Pos != hw.sw[0].Read())
        {
            switch1End = ms;
            if (switch1End - switch1Begin > 500.f)
            {
                switch1Pos = hw.sw[0].Read();
            }
            switch1Begin = ms;
        }

        if (switch2Pos != hw.sw[1].Read())
        {
            switch2End = ms;
            if (switch2End - switch2Begin > 500.f)
            {
                switch2Pos = hw.sw[1].Read();
            }
            switch2Begin = ms;
        }
*/
    }
}