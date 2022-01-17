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

    Parameter knob1;
    Parameter knob2;
    Parameter knob1_dac;
    Parameter knob2_dac;
    Parameter cv1;
    Parameter cv2;

    float knob1Value{};
    float knob2Value{};
    float cv1Value{};
    float cv2Value{};
    bool knob1Changed{};
    bool knob2Changed{};
    bool cv1Trigger{};
    bool raising{};
    bool triggered{};
    bool isCv1Connected{};
    bool isCv2Connected{};

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
        hw.Init();
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

        hw.seed.dac.WriteValue(daisy::DacHandle::Channel::ONE, static_cast<uint16_t>(knob1_dac.Process()));
        hw.seed.dac.WriteValue(daisy::DacHandle::Channel::TWO, static_cast<uint16_t>(knob2_dac.Process()));

        knob1Changed = std::abs(knob1Value - knob1.Process()) > kMinValueDelta;
        if (knob1Changed)
        {
            knob1Value = knob1.Process();
        }
        knob2Changed = std::abs(knob2Value - knob2.Process()) > kMinValueDelta;
        if (knob2Changed)
        {
            knob2Value = knob2.Process();
        }

        isCv1Connected = std::abs(cv1Value - cv1.Process()) > kMinValueDelta;
        if (isCv1Connected)
        {
            cv1Trigger = false;
            raising = cv1.Process() < cv1Value;
            if (!triggered && raising && cv1.Process() >= kTriggerThres)
            {
                int bpm = CalculateBpm();
                if (bpm < kMaxBpm)
                {
                    cv1Bpm = bpm;
                }
                triggered = true;
                cv1Trigger = true;
                begin = ms;
            }
            else if (!raising || cv1.Process() < kTriggerThres)
            {
                triggered = false;
            }
        }
        cv1Value = cv1.Process();

        isCv2Connected = std::abs(cv2Value - cv2.Process()) > kMinValueDelta;
        cv2Value = cv2.Process();
    }
}