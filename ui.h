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
    constexpr float kMaxGain{5.f};
    constexpr float kMaxFilterValue{2000.f};

    enum Channel
    {
        LEFT,
        RIGHT,
        BOTH,
        GLOBAL,
    };
    Channel prevChannel{Channel::BOTH};
    Channel currentChannel{Channel::BOTH};

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

    inline void AudioMeter(float leftIn, float rightIn, float leftOut, float rightOut)
    {
        if (!looper.IsBuffering() && Channel::GLOBAL != currentChannel)
        {
            static float inLeft{};
            static float inRight{};
            static float outLeft{};
            static float outRight{};
            fonepole(inLeft, std::abs(leftIn), 0.01f);
            fonepole(inRight, std::abs(rightIn), 0.01f);
            fonepole(outLeft, std::abs(leftOut), 0.01f);
            fonepole(outRight, std::abs(rightOut), 0.01f);
            hw.leds[DaisyVersio::LED_0].Set(inLeft, inLeft, inLeft);
            hw.leds[DaisyVersio::LED_1].Set(inRight, inRight, inRight);
            hw.leds[DaisyVersio::LED_2].Set(outLeft, outLeft, outLeft);
            hw.leds[DaisyVersio::LED_3].Set(outRight, outRight, outRight);
        }
    }

    float Map(float value, float aMin, float aMax, float bMin, float bMax)
    {
        float k = std::abs(bMax - bMin) / std::abs(aMax - aMin) * (bMax > bMin ? 1 : -1);

        return bMin + k * (value - aMin);
    }

    void GlobalMode(bool active)
    {
        if (active)
        {
            prevChannel = currentChannel;
            currentChannel = Channel::GLOBAL;
            LedMeter(1.f, 7);
        }
        else
        {
            currentChannel = prevChannel;
            ClearLeds();
        }
    }

    // 0: center, 1: left, 2: right
    static void HandleChannelSwitch()
    {
        short value = hw.sw[DaisyVersio::SW_0].Read() - 1;
        value = value < 0 ? 2 : value;
        if (value == prevChannel)
        {
            return;
        }

        // Quit edit mode when changing.
        if (Channel::GLOBAL == currentChannel)
        {
            GlobalMode(false);
        }

        currentChannel = prevChannel = static_cast<Channel>(value);
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

    inline void ProcessParameter(short idx, float value, Channel channel)
    {
        // Keep track of parameters values only after startup.
        if (!looper.IsStartingUp())
        {
            if (Channel::GLOBAL == channel)
            {
                globalValues[idx] = value;
            }
            else {
                channelValues[channel][idx] = value;
            }
        }

        float leftValue{};
        float rightValue{};

        static float deltaLeft{};
        static float deltaRight{};
        if (Channel::LEFT == channel)
        {
            deltaLeft = value - channelValues[Channel::BOTH][idx];
        }
        else if (Channel::RIGHT == channel)
        {
            deltaRight = value - channelValues[Channel::BOTH][idx];
        }

        switch (idx)
        {
            // Blend
            case DaisyVersio::KNOB_0:
                if (Channel::GLOBAL == channel)
                {
                    looper.gain = value * kMaxGain;
                }
                else
                {
                    looper.mix = value;
                }
                break;
            // Start
            case DaisyVersio::KNOB_1:
                {
                    if (Channel::BOTH == channel || Channel::LEFT == channel)
                    {
                        float v = (Channel::BOTH == channel) ? fclamp(value + deltaLeft, kMinSpeedMult, kMaxSpeedMult) : value;
                        leftValue = Map(v, 0.f, 1.f, 0.f, looper.GetBufferSamples(Channel::LEFT));
                        looper.SetLoopStart(Channel::LEFT, leftValue);
                    }
                    if (Channel::BOTH == channel || Channel::RIGHT == channel)
                    {
                        float v = (Channel::BOTH == channel) ? fclamp(value + deltaRight, kMinSpeedMult, kMaxSpeedMult) : value;
                        rightValue = Map(v, 0.f, 1.f, 0.f, looper.GetBufferSamples(Channel::RIGHT));
                        looper.SetLoopStart(Channel::RIGHT, rightValue);
                    }
                }
                break;
            // Tone
            case DaisyVersio::KNOB_2:
                if (Channel::GLOBAL == channel)
                {
                    if (value < 0.33f)
                    {
                        looper.filterType = StereoLooper::FilterType::LP;
                    }
                    else if (value >= 0.33f && value <= 0.66f)
                    {
                        looper.filterType = StereoLooper::FilterType::BP;
                    }
                    else
                    {
                        looper.filterType = StereoLooper::FilterType::HP;
                    }
                }
                else
                {
                    looper.SetFilterValue(Map(value, 0.f, 1.f, 0.f, kMaxFilterValue));
                }
                break;
            // Size
            case DaisyVersio::KNOB_3:
                {
                    if (Channel::BOTH == channel || Channel::LEFT == channel)
                    {
                        float v = (Channel::BOTH == channel) ? value + deltaLeft : value;

                        // Backwards, from buffer's length to 50ms.
                        if (v <= 0.35f)
                        {
                            looper.SetLoopLength(Channel::LEFT, Map(v, 0.f, 0.35f, looper.GetBufferSamples(Channel::LEFT), kMinSamplesForTone));
                            looper.SetDirection(Channel::LEFT, Direction::BACKWARDS);
                        }
                        // Backwards, from 50ms to 1ms (grains).
                        else if (v < 0.5f)
                        {
                            looper.SetLoopLength(Channel::LEFT, Map(v, 0.35f, 0.5f, kMinSamplesForTone, kMinLoopLengthSamples));
                            looper.SetDirection(Channel::LEFT, Direction::BACKWARDS);
                        }
                        // Forward, from 1ms to 50ms (grains).
                        else if (v >= 0.5f && v < 0.65f)
                        {
                            looper.SetLoopLength(Channel::LEFT, Map(v, 0.5f, 0.65f, kMinLoopLengthSamples, kMinSamplesForTone));
                            looper.SetDirection(Channel::LEFT, Direction::FORWARD);
                        }
                        // Forward, from 50ms to buffer's length.
                        else if (v >= 0.65f)
                        {
                            looper.SetLoopLength(Channel::LEFT, Map(v, 0.65f, 1.f, kMinSamplesForTone, looper.GetBufferSamples(Channel::LEFT)));
                            looper.SetDirection(Channel::LEFT, Direction::FORWARD);
                        }

                        // Refresh the rate parameter if the note mode changed.
                        static bool noteModeLeft{};
                        if (noteModeLeft != looper.noteModeLeft)
                        {
                            ProcessParameter(DaisyVersio::KNOB_5, knobValues[DaisyVersio::KNOB_5], Channel::LEFT);
                        }
                        noteModeLeft = looper.noteModeLeft;
                    }

                    if (Channel::BOTH == channel || Channel::RIGHT == channel)
                    {
                        float v = (Channel::BOTH == channel) ? value + deltaRight : value;

                        // Backwards, from buffer's length to 50ms.
                        if (v <= 0.35f)
                        {
                            looper.SetLoopLength(Channel::RIGHT, Map(v, 0.f, 0.35f, looper.GetBufferSamples(Channel::RIGHT), kMinSamplesForTone));
                            looper.SetDirection(Channel::RIGHT, Direction::BACKWARDS);
                        }
                        // Backwards, from 50ms to 1ms (grains).
                        else if (v < 0.5f)
                        {
                            looper.SetLoopLength(Channel::RIGHT, Map(v, 0.35f, 0.5f, kMinSamplesForTone, kMinLoopLengthSamples));
                            looper.SetDirection(Channel::RIGHT, Direction::BACKWARDS);
                        }
                        // Forward, from 1ms to 50ms (grains).
                        else if (v >= 0.5f && v < 0.65f)
                        {
                            looper.SetLoopLength(Channel::RIGHT, Map(v, 0.5f, 0.65f, kMinLoopLengthSamples, kMinSamplesForTone));
                            looper.SetDirection(Channel::RIGHT, Direction::FORWARD);
                        }
                        // Forward, from 50ms to buffer's length.
                        else if (v >= 0.65f)
                        {
                            looper.SetLoopLength(Channel::RIGHT, Map(v, 0.65f, 1.f, kMinSamplesForTone, looper.GetBufferSamples(Channel::RIGHT)));
                            looper.SetDirection(Channel::RIGHT, Direction::FORWARD);
                        }

                        // Refresh the rate parameter if the note mode changed.
                        static bool noteModeRight{};
                        if (noteModeRight != looper.noteModeRight)
                        {
                            ProcessParameter(DaisyVersio::KNOB_5, knobValues[DaisyVersio::KNOB_5], Channel::RIGHT);
                        }
                        noteModeRight = looper.noteModeRight;
                    }
                }
                break;
            // Decay
            case DaisyVersio::KNOB_4:
                if (Channel::GLOBAL == channel)
                {
                    // Full range would cause flipping the L and R channels at 0.
                    looper.stereoImage = Map(value, 0.f, 1.f, 0.5f, 1.f);
                }
                else
                {
                    looper.feedback = value;
                }
                break;
            // Rate
            case DaisyVersio::KNOB_5:
                if (Channel::GLOBAL == channel)
                {
                    looper.rateSlew = Map(value, 0.f, 1.f, 1.f, 0.0001f);
                }
                else
                {
                    if (Channel::BOTH == channel || Channel::LEFT == channel)
                    {
                        float v = (Channel::BOTH == channel) ? fclamp(value + deltaLeft, kMinSpeedMult, kMaxSpeedMult) : value;

                        if (looper.noteModeLeft)
                        {
                            // In "note" mode, the rate knob sets the pitch, with 5
                            // octaves span.
                            leftValue = Map(v, 0.f, 1.f, -24, 24);
                            float rate = std::pow(2.f, leftValue / 12);
                            looper.SetReadRate(Channel::LEFT, rate);
                        }
                        else
                        {
                            if (v < 0.45f)
                            {
                                leftValue = Map(v, 0.f, 0.45f, kMinSpeedMult, 1.f);
                            }
                            else if (v > 0.55f)
                            {
                                leftValue = Map(v, 0.55f, 1.f, 1.f, kMaxSpeedMult);
                            }
                            // Center dead zone.
                            else
                            {
                                leftValue = Map(v, 0.45f, 0.55f, 1.f, 1.f);
                            }
                            looper.SetReadRate(Channel::LEFT, leftValue);
                        }
                    }

                    if (Channel::BOTH == channel || Channel::RIGHT == channel)
                    {
                        float v = (Channel::BOTH == channel) ? fclamp(value + deltaRight, kMinSpeedMult, kMaxSpeedMult) : value;

                        if (looper.noteModeRight)
                        {
                            // In "note" mode, the rate knob sets the pitch, with 5
                            // octaves span.
                            rightValue = Map(v, 0.f, 1.f, -24, 24);
                            float rate = std::pow(2.f, rightValue / 12);
                            looper.SetReadRate(Channel::RIGHT, rate);
                        }
                        else
                        {
                            if (v < 0.45f)
                            {
                                rightValue = Map(v, 0.f, 0.45f, kMinSpeedMult, 1.f);
                            }
                            else if (v > 0.55f)
                            {
                                rightValue = Map(v, 0.55f, 1.f, 1.f, kMaxSpeedMult);
                            }
                            // Center dead zone.
                            else
                            {
                                rightValue = Map(v, 0.45f, 0.55f, 1.f, 1.f);
                            }
                            looper.SetReadRate(Channel::RIGHT, rightValue);
                        }
                    }
                }
                break;
            // Freeze
            case DaisyVersio::KNOB_6:
                if (Channel::GLOBAL == channel)
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

        // Process the parameter only if it actually changed.
        if (std::abs(knobValues[idx] - value) > kMinValueDelta)
        {
            ProcessParameter(idx, value, currentChannel);

            knobValues[idx] = value;
        }
    }

    bool startUp{true};
    bool first{true};

    inline void ProcessUi()
    {
        HandleTriggerSwitch();

        if (!looper.IsStartingUp())
        {
            if (looper.IsBuffering())
            {
                LedMeter(looper.GetBufferSamples(Channel::LEFT) / static_cast<float>(kBufferSamples), 7);

                // Stop buffering.
                if (hw.tap.RisingEdge())
                {
                    ClearLeds();
                    looper.mustStopBuffering = true;
                }
            }
            else
            {
                if (first)
                {
                    // Turn off buffering leds.
                    ClearLeds();

                    // Init all the parameters with the relative knobs position.
                    for (size_t i = 0; i < DaisyVersio::KNOB_LAST; i++)
                    {
                        knobValues[i] = knobs[i].Process();
                        if (knobValues[i] < kMinValueDelta)
                        {
                            knobValues[i] = 0.f;
                        }
                        else if (knobValues[i] > 1 - kMinValueDelta)
                        {
                            knobValues[i] = 1.f;
                        }
                        for (short j = 2; j >= 0; j--)
                        {
                            channelValues[j][i] = knobValues[i];
                            ProcessParameter(i, knobValues[i], static_cast<Channel>(j));
                        }
                    }

                    // Init the global parameters.
                    globalValues[DaisyVersio::KNOB_0] = 1 / kMaxGain; // Gain (1x)
                    ProcessParameter(DaisyVersio::KNOB_0, globalValues[DaisyVersio::KNOB_0], Channel::GLOBAL);
                    globalValues[DaisyVersio::KNOB_2] = 0.5f; // Filter type (BP)
                    ProcessParameter(DaisyVersio::KNOB_2, globalValues[DaisyVersio::KNOB_2], Channel::GLOBAL);
                    globalValues[DaisyVersio::KNOB_4] = 1.f; // Stereo image (full)
                    ProcessParameter(DaisyVersio::KNOB_4, globalValues[DaisyVersio::KNOB_4], Channel::GLOBAL);
                }

                HandleChannelSwitch();

                // Handle button press.
                if (hw.tap.RisingEdge() && !buttonPressed)
                {
                    buttonPressed = true;
                    if (looper.IsGateMode())
                    {
                        looper.dryLevel = 1.f;
                        //looper.mustRestartRead = true;
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
                    if (looper.IsGateMode())
                    {
                        looper.dryLevel = 0.f;
                    }
                    else if (ms - buttonHoldStartTime <= kMaxMsHoldForTrigger)
                    {
                        if (Channel::GLOBAL == currentChannel)
                        {
                            GlobalMode(false);
                        }
                        else
                        {
                            looper.mustRestart = true;
                        }
                    }
                }

                // Do something while the button is pressed.
                if (buttonPressed && !looper.IsGateMode())
                {
                    if (Channel::GLOBAL != currentChannel && ms - buttonHoldStartTime > kMaxMsHoldForTrigger)
                    {
                        GlobalMode(true);
                        buttonPressed = false;
                    }
                    else if (Channel::GLOBAL == currentChannel && ms - buttonHoldStartTime > 1000.f)
                    {
                        GlobalMode(false);
                        buttonPressed = false;
                        looper.mustResetLooper = true;
                    }
                }

                if (looper.IsGateMode())
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
                else if (hw.gate.Trig() && !first)
                {
                    looper.mustRestart = true;
                }

                ProcessKnob(DaisyVersio::KNOB_3); // Size
                ProcessKnob(DaisyVersio::KNOB_1); // Start
                ProcessKnob(DaisyVersio::KNOB_6); // Freeze

                first = false;
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

        looper.SetDirection(Channel::BOTH, Direction::FORWARD);
    }
}