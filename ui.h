#pragma once

#include "hw.h"
#include "repetita.h"
#include "wreath/head.h"
#include "Utility/dsp.h"
#include <string>

namespace wreath
{
    using namespace daisy;
    using namespace daisysp;

    float knobValues[4][7]{};
    Color colors[7];

    enum class MenuClickOp
    {
        TRIGGER,
        CLEAR,
        RESET,
        EDIT,
    };
    MenuClickOp clickOp{MenuClickOp::TRIGGER};
    bool buttonPressed{};

    StereoLooper::TriggerMode currentTriggerMode{};
    char currentLooper{StereoLooper::BOTH};
    bool noteMode{};
    bool flangerMode{};
    bool gateTriggered{};
    bool knobChanged{};
    float startTime{};

    UiEventQueue eventQueue;

    // 0: center, 1: left, 2: right
    static void HandleLooperSwitch()
    {
        static char currentValue{};
        char value = hw.sw[DaisyVersio::SW_0].Read();
        if (value == currentValue)
        {
            return;
        }
        currentValue = value;

        switch (value)
        {
        case 0:
            currentLooper = StereoLooper::BOTH;
            looper.SetMode(StereoLooper::Mode::MONO);
            break;
        case 1:
            currentLooper = StereoLooper::LEFT;
            looper.SetMode(StereoLooper::Mode::DUAL);
            break;
        case 2:
            currentLooper = StereoLooper::RIGHT;
            looper.SetMode(StereoLooper::Mode::DUAL);
            break;
        default:
            break;
        }
    }

    // 0: center, 1: left, 2: right
    static void HandleTriggerSwitch()
    {
        char value = hw.sw[DaisyVersio::SW_1].Read();
        if (value == currentTriggerMode)
        {
            return;
        }

        switch (value)
        {
        case 0:
            currentTriggerMode = StereoLooper::TriggerMode::TRIGGER;
            break;
        case 1:
            currentTriggerMode = StereoLooper::TriggerMode::GATE;
            break;
        case 2:
            currentTriggerMode = StereoLooper::TriggerMode::LOOP;
            break;
        default:
            break;
        }

        looper.SetTriggerMode(currentTriggerMode);
    }

    inline void ClearLeds()
    {
        for (size_t i = 0; i < DaisyVersio::LED_LAST; i++)
        {
            hw.SetLed(i, 0, 0, 0);
        }
    }

    inline void LedMeter(float value, short colorIdx)
    {
        short idx = static_cast<short>(std::floor(value * 4));
        for (short i = 0; i <= DaisyVersio::LED_LAST; i++)
        {
            if (i <= idx)
            {
                float factor = ((i == idx) ? (value * 4 - idx) : 1.f);
                if (factor < kMinValueDelta)
                {
                    factor = 0.f;
                }
                hw.SetLed(i, colors[colorIdx].Red() * factor, colors[colorIdx].Green() * factor, colors[colorIdx].Blue() * factor);
            }
            else
            {
                hw.SetLed(i, 0, 0, 0);
            }
        }
    }

    float Map(float value, float aMin, float aMax, float bMin, float bMax)
    {
        float k = std::abs(bMax - bMin) / std::abs(aMax - aMin) * (bMax > bMin ? 1 : -1);

        return bMin + k * (value - aMin);
    }

    inline void ProcessKnob(int idx)
    {
        float value = knobs[idx].Process();
        // Handle range limits.
        if (value < kMinValueDelta)
        {
            value = 0.f;
        }
        else if (value > 1 - kMinValueDelta)
        {
            value = 1.f;
        }
        if (std::abs(knobValues[currentLooper][idx] - value) > kMinValueDelta)
        {
            LedMeter(value, idx);
            switch (idx)
            {
                // Blend
                case DaisyVersio::KNOB_0:
                    looper.nextMix = value;
                    break;
                // Start
                case DaisyVersio::KNOB_1:
                    {
                        int32_t loopEnd = looper.IsDualMode() ? looper.GetLoopEnd(currentLooper) : looper.GetLoopEnd(StereoLooper::LEFT);
                        looper.SetLoopStart(currentLooper, Map(value, 0.f, 1.f, 0.f, loopEnd));
                    }
                    break;
                // Tone
                case DaisyVersio::KNOB_2:
                    looper.nextFilterValue = Map(value, 0.f, 1.f, 0.f, 1000.f);
                    break;
                // Flip
                case DaisyVersio::KNOB_3:
                    {
                        looper.SetDirection(currentLooper, value < 0.5f ? Direction::BACKWARDS : Direction::FORWARD);
                        int32_t bufferSamples = looper.IsDualMode() ? looper.GetBufferSamples(currentLooper) : looper.GetBufferSamples(StereoLooper::LEFT);
                        if (value < 0.3f)
                        {
                            looper.SetLoopLength(currentLooper, Map(value, 0.f, 0.3f, bufferSamples, kMinSamplesForTone));
                            noteMode = false;
                            flangerMode = false;
                        }
                        else if (value < 0.45f)
                        {
                            looper.SetLoopLength(currentLooper, Map(value, 0.3f, 0.45f, kMinSamplesForTone, kMinLoopLengthSamples));
                            noteMode = false;
                            flangerMode = false;
                        }
                        else if (value > 0.55f && value < 0.7f)
                        {
                            looper.SetLoopLength(currentLooper, Map(value, 0.55f, 0.7f, kMinLoopLengthSamples, kMinSamplesForTone));
                            noteMode = false;
                            flangerMode = true;
                        }
                        else if (value > 0.7f)
                        {
                            looper.SetLoopLength(currentLooper, Map(value, 0.7f, 1.f, kMinSamplesForTone, bufferSamples));
                            noteMode = false;
                            flangerMode = false;
                        }
                        // Center dead zone.
                        else
                        {
                            looper.SetLoopLength(currentLooper, kMinLoopLengthSamples);
                            noteMode = true;
                            flangerMode = false;
                        }
                    }
                    break;
                // Decay
                case DaisyVersio::KNOB_4:
                    looper.nextFeedback = value;
                    break;
                // Speed
                case DaisyVersio::KNOB_5:
                    if (noteMode)
                    {
                        // In "note" mode, the rate knob sets the pitch, with 5
                        // octaves span.
                        float n = Map(value, 0.f, 1.f, -48, 12);
                        float rate = std::pow(2.f, n / 12);
                        looper.SetReadRate(currentLooper, rate);
                    }
                    else if (flangerMode)
                    {
                        int32_t step = static_cast<int32_t>(value * 4);
                        looper.SetReadRate(currentLooper, Map(step, 0, 3, 0.f, 1.f));
                    }
                    else
                    {
                        if (value < 0.45f)
                        {
                            looper.SetReadRate(currentLooper, Map(value, 0.f, 0.45f, kMinSpeedMult, 1.f));
                        }
                        else if (value > 0.55f)
                        {
                            looper.SetReadRate(currentLooper, Map(value, 0.55f, 1.f, 1.f, kMaxSpeedMult));
                        }
                        // Center dead zone.
                        else
                        {
                            looper.SetReadRate(currentLooper, 1.f);
                        }
                    }
                    break;
                // Freeze
                case DaisyVersio::KNOB_6:
                    looper.SetFreeze(value);
                    break;

                default:
                    break;
            }
            knobValues[currentLooper][idx] = value;
            if (!knobChanged)
            {
                knobChanged = true;
            }
        }
        else
        {
            if (knobChanged)
            {
                startTime = ms;
                knobChanged = false;
            }
            else
            {
                if (ms - startTime > 350.f)
                {
                    ClearLeds();
                }
            }
        }
    }

    inline void ProcessUi()
    {
        if (!looper.IsStartingUp())
        {
            if (looper.IsBuffering())
            {
                LedMeter(looper.GetBufferSamples(StereoLooper::LEFT) / static_cast<float>(kBufferSamples), 7);
                if (hw.tap.FallingEdge())
                {
                    // Stop buffering.
                    looper.mustStopBuffering = true;
                }
            }
            else
            {
                HandleLooperSwitch();
                HandleTriggerSwitch();

                if (hw.tap.RisingEdge())
                {
                    buttonPressed = true;
                }

                if (hw.tap.FallingEdge() && buttonPressed)
                {
                    buttonPressed = false;
                    switch (clickOp)
                    {
                        case MenuClickOp::TRIGGER:
                            if (StereoLooper::TriggerMode::GATE == looper.GetTriggerMode())
                            {
                                looper.dryLevel_ = 1.0f;
                                looper.mustRestartRead = true;
                            }
                            else
                            {
                                looper.mustRestart = true;
                            }
                            break;
                        case MenuClickOp::CLEAR:
                            looper.mustClearBuffer = true;
                            clickOp = MenuClickOp::TRIGGER;
                            break;
                        case MenuClickOp::RESET:
                            looper.mustResetLooper = true;
                            clickOp = MenuClickOp::TRIGGER;
                            break;
                        case MenuClickOp::EDIT:
                            clickOp = MenuClickOp::TRIGGER;
                            break;
                    }
                }

                if (buttonPressed)
                {
                    if (StereoLooper::BOTH == currentLooper)
                    {
                        if (StereoLooper::TriggerMode::GATE == looper.GetTriggerMode())
                        {
                            looper.dryLevel_ = 0.f;
                        }
                        // Handle button time held only when the trigger mode is not gate.
                        else
                        {
                            // Clear the buffer if the button has been held more than 2 sec.
                            if (hw.tap.TimeHeldMs() >= 2000.f)
                            {
                                clickOp = MenuClickOp::CLEAR;
                            }
                            // Reset the looper if the button has been held more than 5 sec.
                            if (hw.tap.TimeHeldMs() >= 5000.f)
                            {
                                clickOp = MenuClickOp::RESET;
                            }
                        }
                    }
                    else
                    {
                        clickOp = MenuClickOp::EDIT;
                    }
                }

                if (StereoLooper::TriggerMode::GATE == looper.GetTriggerMode())
                {
                    if (hw.Gate())
                    {
                        looper.dryLevel_ = 1.0f;
                        //looper.mustRestartRead = true;
                        gateTriggered = true;
                    }
                    else if (gateTriggered) {
                        looper.dryLevel_ = 0.f;
                        gateTriggered = false;
                    }
                }
                else if (hw.gate.Trig())
                {
                    looper.mustRestart = true;
                }

                if (StereoLooper::BOTH == currentLooper || MenuClickOp::EDIT == clickOp)
                {
                    ProcessKnob(DaisyVersio::KNOB_3); // Size
                    ProcessKnob(DaisyVersio::KNOB_1); // Start
                }
            }

            if (StereoLooper::BOTH == currentLooper || MenuClickOp::EDIT == clickOp)
            {
                ProcessKnob(DaisyVersio::KNOB_5); // Rate
            }
            ProcessKnob(DaisyVersio::KNOB_4); // Decay
            ProcessKnob(DaisyVersio::KNOB_2); // Tone
            ProcessKnob(DaisyVersio::KNOB_6); // Freeze
            ProcessKnob(DaisyVersio::KNOB_0); // Blend
        }
    }

    inline void InitUi()
    {
        colors[DaisyVersio::KNOB_0].Init(0.5f, 0.f, 1.f); // Purple
        colors[DaisyVersio::KNOB_1].Init(0.f, 1.f, 0.5f); // Green
        colors[DaisyVersio::KNOB_2].Init(0.f, 0.f, 1.f); // Blue
        colors[DaisyVersio::KNOB_3].Init(1.f, 0.8f, 0.5f); // Yellow
        colors[DaisyVersio::KNOB_4].Init(1.f, 0.f, 1.f); // Magenta
        colors[DaisyVersio::KNOB_5].Init(1.f, 0.5f, 0.f); // Orange
        colors[DaisyVersio::KNOB_6].Init(0.5f, 0.5f, 1.f); // Light blue
        colors[7].Init(1.f, 0.f, 0.f); // Red
    }
}