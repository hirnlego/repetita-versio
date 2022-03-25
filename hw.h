#pragma once

#include "daisy_versio.h"

namespace wreath
{
    using namespace daisy;

    // The minimum difference in parameter value to be registered.
    constexpr float kMinValueDelta{0.002f};
    // The minimum difference in parameter value to be considered picked up.
    constexpr float kMinPickupValueDelta{0.01f};
    // The trigger threshold value.
    constexpr float kTriggerThres{0.3f};

    DaisyVersio hw;

    Parameter knobs[DaisyVersio::KNOB_LAST]{};

    inline void InitHw()
    {
        hw.Init(true);
        hw.StartAdc();

        for (short i = 0; i < DaisyVersio::KNOB_LAST; i++)
        {
            hw.knobs[i].SetCoeff(1.f); // No slew;
            knobs[i].Init(hw.knobs[i], 0.0f, 1.0f, Parameter::LINEAR);
        }
    }

    inline void ProcessControls()
    {
        hw.ProcessAllControls();
        hw.tap.Debounce();
        hw.UpdateLeds();
    }
}