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

    constexpr float kMaxMsHoldForTrigger{250.f};

    short currentChannel{StereoLooper::BOTH};
    float channelValues[3][7]{};
    float knobValues[7]{};
    float globalValues[7]{};
    Color colors[7];

    enum class ButtonOp
    {
        NORMAL,
        CLEAR,
        RESET,
        EDIT,
    };
    ButtonOp buttonOp{ButtonOp::NORMAL};
    StereoLooper::TriggerMode currentTriggerMode{};
    bool buttonPressed{};
    bool gateTriggered{};
    bool editMode{};
    float knobChangeStartTime{};
    float buttonHoldStartTime{};
    bool mustPickUpLeft{};
    bool mustPickUpRight{};

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

    void EditMode(bool active)
    {
        editMode = active;
        active ? LedMeter(1.f, 7) : ClearLeds();
    }

    // 0: center, 1: left, 2: right
    static void HandleChannelSwitch()
    {
        short value = hw.sw[DaisyVersio::SW_0].Read() - 1;
        value = value < 0 ? 2 : value;
        if (value == currentChannel)
        {
            return;
        }
        currentChannel = value;

        // Quit edit mode when changing.
        if (editMode)
        {
            EditMode(false);
        }
    }

    // 0: center, 1: left, 2: right
    static void HandleTriggerSwitch()
    {
        short value = hw.sw[DaisyVersio::SW_1].Read();
        if (value == currentTriggerMode)
        {
            return;
        }

        currentTriggerMode = static_cast<StereoLooper::TriggerMode>(value);
        looper.SetTriggerMode(currentTriggerMode);
    }

    inline void ProcessParameter(short idx, float value, short channel, bool global)
    {
        if (!looper.IsStartingUp())
        {
            if (editMode || global)
            {
                globalValues[idx] = value;
            }
            else {
                channelValues[channel][idx] = value;
            }
        }

        switch (idx)
        {
            // Blend
            case DaisyVersio::KNOB_0:
                if (editMode)
                {
                    looper.nextGain = value * 5;
                }
                else
                {
                    looper.nextMix = value;
                }
                break;
            // Start
            case DaisyVersio::KNOB_1:
                {
                    int32_t bufferSamples = looper.IsDualMode() ? looper.GetBufferSamples(channel) : looper.GetBufferSamples(StereoLooper::LEFT);
                    looper.SetLoopStart(channel, Map(value, 0.f, 1.f, 0.f, bufferSamples));
                }
                break;
            // Tone
            case DaisyVersio::KNOB_2:
                looper.nextFilterValue = Map(value, 0.f, 1.f, 0.f, 1000.f);
                break;
            // Size
            case DaisyVersio::KNOB_3:
                {
                    int32_t bufferSamples = looper.IsDualMode() ? looper.GetBufferSamples(channel) : looper.GetBufferSamples(StereoLooper::LEFT);
                    if (value <= 0.35f)
                    {
                        looper.SetLoopLength(channel, Map(value, 0.f, 0.35f, bufferSamples, kMinSamplesForTone));
                        looper.SetDirection(channel, Direction::BACKWARDS);
                    }
                    else if (value <= 0.45f)
                    {
                        looper.SetLoopLength(channel, Map(value, 0.35f, 0.45f, kMinSamplesForTone, kMinLoopLengthSamples));
                        looper.SetDirection(channel, Direction::BACKWARDS);
                    }
                    else if (value >= 0.55f && value < 0.65f)
                    {
                        looper.SetLoopLength(channel, Map(value, 0.55f, 0.65f, kMinLoopLengthSamples, kMinSamplesForTone));
                        looper.SetDirection(channel, Direction::FORWARD);
                    }
                    else if (value >= 0.65f)
                    {
                        looper.SetLoopLength(channel, Map(value, 0.65f, 1.f, kMinSamplesForTone, bufferSamples));
                        looper.SetDirection(channel, Direction::FORWARD);
                    }
                    // Center dead zone.
                    else
                    {
                        looper.SetLoopLength(channel, kMinLoopLengthSamples);
                        looper.SetDirection(channel, Direction::FORWARD);
                    }

                    // Refresh the rate parameter if the note mode changes.
                    static bool noteModeLeft{};
                    if (noteModeLeft != looper.noteModeLeft)
                    {
                        ProcessParameter(DaisyVersio::KNOB_5, knobValues[DaisyVersio::KNOB_5], StereoLooper::LEFT, false);
                    }
                    noteModeLeft = looper.noteModeLeft;
                    // Refresh the rate parameter if the note mode changes.
                    static bool noteModeRight{};
                    if (noteModeRight != looper.noteModeRight)
                    {
                        ProcessParameter(DaisyVersio::KNOB_5, knobValues[DaisyVersio::KNOB_5], StereoLooper::RIGHT, false);
                    }
                    noteModeRight = looper.noteModeRight;
                }
                break;
            // Decay
            case DaisyVersio::KNOB_4:
                looper.nextFeedback = value;
                break;
            // Rate
            case DaisyVersio::KNOB_5:
                if (looper.noteModeLeft && StereoLooper::LEFT == channel)
                {
                    // In "note" mode, the rate knob sets the pitch, with 5
                    // octaves span.
                    float n = Map(value, 0.f, 1.f, -48, 12);
                    float rate = std::pow(2.f, n / 12);
                    looper.SetReadRate(StereoLooper::LEFT, rate);
                }
                else if (looper.noteModeRight && StereoLooper::RIGHT == channel)
                {
                    // In "note" mode, the rate knob sets the pitch, with 5
                    // octaves span.
                    float n = Map(value, 0.f, 1.f, -48, 12);
                    float rate = std::pow(2.f, n / 12);
                    looper.SetReadRate(StereoLooper::RIGHT, rate);
                }
                else
                {
                    if (value < 0.45f)
                    {
                        looper.SetReadRate(channel, Map(value, 0.f, 0.45f, kMinSpeedMult, 1.f));
                    }
                    else if (value > 0.55f)
                    {
                        looper.SetReadRate(channel, Map(value, 0.55f, 1.f, 1.f, kMaxSpeedMult));
                    }
                    // Center dead zone.
                    else
                    {
                        looper.SetReadRate(channel, 1.f);
                    }
                }
                break;
            // Freeze
            case DaisyVersio::KNOB_6:
                if (editMode)
                {
                }
                else
                {
                    looper.SetFreeze(value);
                }
                break;

            default:
                break;
        }
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

        // Process the parameter only if the there's actually a change.
        if (std::abs(knobValues[idx] - value) > kMinValueDelta)
        {
            // In mono mode, update the parameter right away.
            if (StereoLooper::BOTH == currentChannel && !editMode)
            {
                ProcessParameter(idx, value, StereoLooper::BOTH, false);
            }
            // In dual mode, handle the parameter change picking up from the
            // previous value.
            else
            {
                editMode ? LedMeter(1.f, 7) : ClearLeds();

                short led{};

                // Handle parameter pick-up for the left channel.
                if (StereoLooper::LEFT == currentChannel && editMode)
                {
                    float lv = std::abs(channelValues[StereoLooper::LEFT][idx] - value);
                    led = value > channelValues[StereoLooper::LEFT][idx] ? 1 - std::round(lv) : 2 + std::round(lv);
                    mustPickUpLeft = lv > kMinPickupValueDelta;
                    if (!mustPickUpLeft)
                    {
                        ProcessParameter(idx, value, StereoLooper::LEFT, editMode);
                    }
                }
                // Handle parameter pick-up for the right channel.
                if (StereoLooper::RIGHT == currentChannel && editMode)
                {
                    float rv = std::abs(channelValues[StereoLooper::RIGHT][idx] - value);
                    led = value > channelValues[StereoLooper::RIGHT][idx] ? 1 - std::round(rv) : 2 + std::round(rv);
                    mustPickUpRight = rv > kMinPickupValueDelta;
                    if (!mustPickUpRight)
                    {
                        ProcessParameter(idx, value, StereoLooper::RIGHT, editMode);
                    }
                }
                // Show where and how far the previous values is.
                if (mustPickUpLeft || mustPickUpRight)
                {
                    ClearLeds();
                    hw.SetLed(led, 1.f, 1.f, 1.f);
                }
            }

            knobValues[idx] = value;
        }
    }

    inline void ProcessUi()
    {
        if (looper.IsStartingUp())
        {
            // Init the parameters that, at this moment, can be safely set to
            // their respective knobs values (those not affected by the initial
            // buffering).
            knobValues[DaisyVersio::KNOB_0] = knobs[DaisyVersio::KNOB_0].Process(); // Blend
            channelValues[StereoLooper::LEFT][DaisyVersio::KNOB_0] = knobValues[DaisyVersio::KNOB_0];
            channelValues[StereoLooper::RIGHT][DaisyVersio::KNOB_0] = knobValues[DaisyVersio::KNOB_0];
            ProcessParameter(DaisyVersio::KNOB_0, knobValues[DaisyVersio::KNOB_0], currentChannel, false);
            knobValues[DaisyVersio::KNOB_2] = knobs[DaisyVersio::KNOB_2].Process(); // Tone
            channelValues[StereoLooper::LEFT][DaisyVersio::KNOB_2] = knobValues[DaisyVersio::KNOB_2];
            channelValues[StereoLooper::RIGHT][DaisyVersio::KNOB_2] = knobValues[DaisyVersio::KNOB_2];
            ProcessParameter(DaisyVersio::KNOB_2, knobValues[DaisyVersio::KNOB_2], currentChannel, false);
            knobValues[DaisyVersio::KNOB_4] = knobs[DaisyVersio::KNOB_4].Process(); // Decay
            channelValues[StereoLooper::LEFT][DaisyVersio::KNOB_4] = knobValues[DaisyVersio::KNOB_4];
            channelValues[StereoLooper::RIGHT][DaisyVersio::KNOB_4] = knobValues[DaisyVersio::KNOB_4];
            ProcessParameter(DaisyVersio::KNOB_4, knobValues[DaisyVersio::KNOB_4], currentChannel, false);
            knobValues[DaisyVersio::KNOB_5] = knobs[DaisyVersio::KNOB_5].Process(); // Rate
            channelValues[StereoLooper::LEFT][DaisyVersio::KNOB_5] = knobValues[DaisyVersio::KNOB_5];
            channelValues[StereoLooper::RIGHT][DaisyVersio::KNOB_5] = knobValues[DaisyVersio::KNOB_5];
            ProcessParameter(DaisyVersio::KNOB_5, knobValues[DaisyVersio::KNOB_5], currentChannel, false);

            // The others, just init the variables.
            knobValues[DaisyVersio::KNOB_1] = knobs[DaisyVersio::KNOB_1].Process(); // Start
            channelValues[StereoLooper::LEFT][DaisyVersio::KNOB_1] = knobValues[DaisyVersio::KNOB_1];
            channelValues[StereoLooper::RIGHT][DaisyVersio::KNOB_1] = knobValues[DaisyVersio::KNOB_1];
            knobValues[DaisyVersio::KNOB_3] = knobs[DaisyVersio::KNOB_3].Process(); // Size
            channelValues[StereoLooper::LEFT][DaisyVersio::KNOB_3] = knobValues[DaisyVersio::KNOB_3];
            channelValues[StereoLooper::RIGHT][DaisyVersio::KNOB_3] = knobValues[DaisyVersio::KNOB_3];
            knobValues[DaisyVersio::KNOB_6] = knobs[DaisyVersio::KNOB_6].Process(); // Freeze
            channelValues[StereoLooper::LEFT][DaisyVersio::KNOB_6] = knobValues[DaisyVersio::KNOB_6];
            channelValues[StereoLooper::RIGHT][DaisyVersio::KNOB_6] = knobValues[DaisyVersio::KNOB_6];

            // Init the global parameters.
            globalValues[DaisyVersio::KNOB_0] = 1.f; // Gain
            ProcessParameter(DaisyVersio::KNOB_0, globalValues[DaisyVersio::KNOB_0], currentChannel, true);
        }
        else {
            HandleTriggerSwitch();

            if (looper.IsBuffering())
            {
                LedMeter(looper.GetBufferSamples(StereoLooper::LEFT) / static_cast<float>(kBufferSamples), 7);
                if (hw.tap.RisingEdge())
                {
                    // Stop buffering.
                    ClearLeds();
                    looper.mustStopBuffering = true;
                }
            }
            else
            {
                HandleChannelSwitch();

                // Handle button press.
                if (hw.tap.RisingEdge() && !buttonPressed)
                {
                    buttonPressed = true;
                    if (StereoLooper::TriggerMode::GATE == looper.GetTriggerMode())
                    {
                        looper.dryLevel = 1.f;
                        looper.mustRestartRead = true;
                    }
                    else
                    {
                        buttonHoldStartTime = ms;
                    }
                }

                // Handle button release.
                else if (hw.tap.FallingEdge() && buttonPressed)
                {
                    buttonPressed = false;
                    if (StereoLooper::TriggerMode::GATE == looper.GetTriggerMode())
                    {
                        looper.dryLevel = 0.f;
                    }
                    else if (ms - buttonHoldStartTime <= kMaxMsHoldForTrigger)
                    {
                        if (editMode)
                        {
                            EditMode(false);
                        }
                        else
                        {
                            looper.mustRestart = true;
                        }
                    }
                }

                // Do something while the button is pressed.
                if (buttonPressed)
                {
                    if (!editMode && ms - buttonHoldStartTime > kMaxMsHoldForTrigger)
                    {
                        EditMode(true);
                        buttonPressed = false;
                    }
                    else if (editMode && ms - buttonHoldStartTime > 1000.f)
                    {
                        ClearLeds();
                        looper.mustResetLooper = true;
                    }
                }

                if (StereoLooper::TriggerMode::GATE == looper.GetTriggerMode())
                {
                    if (hw.Gate())
                    {
                        looper.dryLevel = 1.0f;
                        //looper.mustRestartRead = true;
                        gateTriggered = true;
                    }
                    else if (gateTriggered) {
                        looper.dryLevel = 0.f;
                        gateTriggered = false;
                    }
                }
                else if (hw.gate.Trig())
                {
                    looper.mustRestart = true;
                }

                ProcessKnob(DaisyVersio::KNOB_3); // Size
                ProcessKnob(DaisyVersio::KNOB_1); // Start
                ProcessKnob(DaisyVersio::KNOB_6); // Freeze
            }

            ProcessKnob(DaisyVersio::KNOB_0); // Blend
            ProcessKnob(DaisyVersio::KNOB_2); // Tone
            ProcessKnob(DaisyVersio::KNOB_4); // Decay
            ProcessKnob(DaisyVersio::KNOB_5); // Rate
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