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

    constexpr float kMaxMsHoldForTrigger{300.f};
    constexpr float kMaxGain{5.f};
    constexpr float kMaxFilterValue{1500.f};
    constexpr float kMaxRateSlew{10.f};

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
    float deltaValues[2][7]{};
    float knobValues[7]{};
    float globalValues[7]{};

    enum ColorName
    {
        COLOR_RED,
        COLOR_YELLOW,
        COLOR_PURPLE,
        COLOR_GREEN,
        COLOR_ICE,
        COLOR_ORANGE,
        COLOR_BLUE,
        COLOR_PINK,
        COLOR_WHITE,
        COLOR_LIME,
        COLOR_CREAM,
        COLOR_AQUA,
        COLOR_LAST
    };
    Color colors[ColorName::COLOR_LAST];
    ColorName channelColor[2]{ColorName::COLOR_ORANGE, ColorName::COLOR_ORANGE};

    enum class ButtonHoldMode
    {
        NO_MODE,
        GLOBAL,
        ARM,
    };
    ButtonHoldMode buttonHoldMode{ButtonHoldMode::NO_MODE};
    Looper::TriggerMode currentTriggerMode{};
    bool buttonPressed{};
    bool gateTriggered{};
    int32_t buttonHoldStartTime{};
    bool recordingArmed{};

    bool startUp{true};
    bool first{true};
    bool buffering{};

    inline void ClearLeds()
    {
        for (size_t i = 0; i < DaisyVersio::LED_LAST; i++)
        {
            hw.SetLed(i, 0, 0, 0);
        }
    }

    inline void LedMeter(float value, short colorIdx, short length = 4, float offset = 0.f)
    {
        value = fclamp(value, 0.f, 1.f);
        short idx = static_cast<short>(std::floor(value * (length)));
        //idx %= length;
        //short odx = static_cast<short>(std::floor(offset * (length - 1)));
        for (short i = 0; i <= length + 1; i++)
        {
            if (i <= idx)
            {
                float factor = ((i == idx) ? (value * length - idx) : 1.f);
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
        }
        else
        {
            currentChannel = prevChannel;
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

        currentTriggerMode = static_cast<Looper::TriggerMode>(value);
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

        if (Channel::LEFT == channel)
        {
            deltaValues[Channel::LEFT][idx] = value - channelValues[Channel::BOTH][idx];
        }
        else if (Channel::RIGHT == channel)
        {
            deltaValues[Channel::RIGHT][idx] = value - channelValues[Channel::BOTH][idx];
        }

        switch (idx)
        {
            // Blend
            case DaisyVersio::KNOB_0:
                if (Channel::GLOBAL == channel)
                {
                    looper.inputGain = value * kMaxGain;
                }
                else
                {
                    looper.dryWetMix = value;
                }
                break;
            // Start
            case DaisyVersio::KNOB_1:
                {
                    if (Channel::GLOBAL == channel)
                    {
                        looper.OffsetLoopers(Map(value, 0.f, 1.f, 0.f, looper.GetLoopLength(Channel::RIGHT)));
                    }
                    else
                    {
                        if (Channel::BOTH == channel || Channel::LEFT == channel)
                        {
                            float v = (Channel::BOTH == channel) ? fclamp(value + deltaValues[Channel::LEFT][idx], 0.f, 1.f) : value;
                            leftValue = Map(v, 0.f, 1.f, 0.f, looper.GetBufferSamples(Channel::LEFT) - 1);
                            looper.SetLoopStart(Channel::LEFT, leftValue);
                        }
                        if (Channel::BOTH == channel || Channel::RIGHT == channel)
                        {
                            float v = (Channel::BOTH == channel) ? fclamp(value + deltaValues[Channel::RIGHT][idx], 0.f, 1.f) : value;
                            rightValue = Map(v, 0.f, 1.f, 0.f, looper.GetBufferSamples(Channel::RIGHT) - 1);
                            looper.SetLoopStart(Channel::RIGHT, rightValue);
                        }
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
                    if (Channel::GLOBAL == channel)
                    {
                        //looper.SetSamplesToFade(Map(value, 0.f, 1.f, 0.f, kMaxSamplesToFade));
                        looper.SetLoopSync(value >= 0.5);
                    }
                    else
                    {
                        if (Channel::BOTH == channel || Channel::LEFT == channel)
                        {
                            float v = (Channel::BOTH == channel) ? fclamp(value + deltaValues[Channel::LEFT][idx], 0.f, 1.f) : value;

                            // Backwards, from buffer's length to 50ms.
                            if (v <= 0.35f)
                            {
                                looper.SetLoopLength(Channel::LEFT, Map(v, 0.f, 0.35f, looper.GetBufferSamples(Channel::LEFT), kMinSamplesForFlanger));
                                looper.SetDirection(Channel::LEFT, Direction::BACKWARDS);
                                channelColor[Channel::LEFT] = looper.GetLoopSync() ? ColorName::COLOR_PURPLE : ColorName::COLOR_GREEN;
                            }
                            // Backwards, from 50ms to 1ms (grains).
                            else if (v < 0.47f)
                            {
                                looper.SetLoopLength(Channel::LEFT, Map(v, 0.35f, 0.47f, kMinSamplesForFlanger, kMinSamplesForTone));
                                looper.SetDirection(Channel::LEFT, Direction::BACKWARDS);
                                channelColor[Channel::LEFT] = looper.GetLoopSync() ? ColorName::COLOR_PINK : ColorName::COLOR_LIME;
                            }
                            // Forward, from 1ms to 50ms (grains).
                            else if (v >= 0.53f && v < 0.65f)
                            {
                                looper.SetLoopLength(Channel::LEFT, Map(v, 0.53f, 0.65f, kMinSamplesForTone, kMinSamplesForFlanger));
                                looper.SetDirection(Channel::LEFT, Direction::FORWARD);
                                channelColor[Channel::LEFT] = looper.GetLoopSync() ? ColorName::COLOR_AQUA : ColorName::COLOR_YELLOW;
                            }
                            // Forward, from 50ms to buffer's length.
                            else if (v >= 0.65f)
                            {
                                looper.SetLoopLength(Channel::LEFT, Map(v, 0.65f, 1.f, kMinSamplesForFlanger, looper.GetBufferSamples(Channel::LEFT)));
                                looper.SetDirection(Channel::LEFT, Direction::FORWARD);
                                channelColor[Channel::LEFT] = looper.GetLoopSync() ? ColorName::COLOR_BLUE : ColorName::COLOR_ORANGE;
                            }
                            // Center dead zone.
                            else
                            {
                                looper.SetLoopLength(Channel::LEFT, Map(v, 0.47f, 0.53f, kMinLoopLengthSamples, kMinLoopLengthSamples));
                                looper.SetDirection(Channel::LEFT, Direction::FORWARD);
                                channelColor[Channel::LEFT] = ColorName::COLOR_WHITE;
                            }

                            // Refresh the rate parameter if the note mode changed.
                            static StereoLooper::NoteMode noteModeLeft{};
                            if (noteModeLeft != looper.noteModeLeft)
                            {
                                ProcessParameter(DaisyVersio::KNOB_5, knobValues[DaisyVersio::KNOB_5], Channel::LEFT);
                            }
                            noteModeLeft = looper.noteModeLeft;
                        }

                        if (Channel::BOTH == channel || Channel::RIGHT == channel)
                        {
                            float v = (Channel::BOTH == channel) ? fclamp(value + deltaValues[Channel::RIGHT][idx], 0.f, 1.f) : value;

                            // Backwards, from buffer's length to 50ms.
                            if (v <= 0.35f)
                            {
                                looper.SetLoopLength(Channel::RIGHT, Map(v, 0.f, 0.35f, looper.GetBufferSamples(Channel::RIGHT), kMinSamplesForFlanger));
                                looper.SetDirection(Channel::RIGHT, Direction::BACKWARDS);
                                channelColor[Channel::RIGHT] = looper.GetLoopSync() ? ColorName::COLOR_PURPLE : ColorName::COLOR_GREEN;
                            }
                            // Backwards, from 50ms to 1ms (grains).
                            else if (v < 0.47f)
                            {
                                looper.SetLoopLength(Channel::RIGHT, Map(v, 0.35f, 0.47f, kMinSamplesForFlanger, kMinSamplesForTone));
                                looper.SetDirection(Channel::RIGHT, Direction::BACKWARDS);
                                channelColor[Channel::RIGHT] = looper.GetLoopSync() ? ColorName::COLOR_PINK : ColorName::COLOR_LIME;
                            }
                            // Forward, from 1ms to 50ms (grains).
                            else if (v >= 0.53f && v < 0.65f)
                            {
                                looper.SetLoopLength(Channel::RIGHT, Map(v, 0.53f, 0.65f, kMinSamplesForTone, kMinSamplesForFlanger));
                                looper.SetDirection(Channel::RIGHT, Direction::FORWARD);
                                channelColor[Channel::RIGHT] = looper.GetLoopSync() ? ColorName::COLOR_AQUA : ColorName::COLOR_YELLOW;
                            }
                            // Forward, from 50ms to buffer's length.
                            else if (v >= 0.65f)
                            {
                                looper.SetLoopLength(Channel::RIGHT, Map(v, 0.65f, 1.f, kMinSamplesForFlanger, looper.GetBufferSamples(Channel::RIGHT)));
                                looper.SetDirection(Channel::RIGHT, Direction::FORWARD);
                                channelColor[Channel::RIGHT] = looper.GetLoopSync() ? ColorName::COLOR_BLUE : ColorName::COLOR_ORANGE;
                            }
                            // Center dead zone.
                            else
                            {
                                looper.SetLoopLength(Channel::RIGHT, Map(v, 0.47f, 0.53f, kMinLoopLengthSamples, kMinLoopLengthSamples));
                                looper.SetDirection(Channel::RIGHT, Direction::FORWARD);
                                channelColor[Channel::RIGHT] = ColorName::COLOR_WHITE;
                            }

                            // Refresh the rate parameter if the note mode changed.
                            static StereoLooper::NoteMode noteModeRight{};
                            if (noteModeRight != looper.noteModeRight)
                            {
                                ProcessParameter(DaisyVersio::KNOB_5, knobValues[DaisyVersio::KNOB_5], Channel::RIGHT);
                            }
                            noteModeRight = looper.noteModeRight;
                        }
                    }
                }
                break;
            // Decay
            case DaisyVersio::KNOB_4:
                if (Channel::GLOBAL == channel)
                {
                    looper.filterLevel = value;
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
                    looper.rateSlew = Map(value, 0.f, 1.f, 0.f, kMaxRateSlew);
                }
                else
                {
                    if (Channel::BOTH == channel || Channel::LEFT == channel)
                    {
                        float v = (Channel::BOTH == channel) ? fclamp(value + deltaValues[Channel::LEFT][idx], 0.f, 1.f) : value;

                        if (StereoLooper::NoteMode::NOTE == looper.noteModeLeft)
                        {
                            // In "note" mode, the rate knob sets the pitch, with 4
                            // octaves span.
                            leftValue = std::floor(Map(v, 0.f, 1.f, -24, 24)) - 24;
                            leftValue = std::pow(2.f, leftValue / 12);
                        }
                        else if (StereoLooper::NoteMode::FLANGER == looper.noteModeLeft)
                        {
                            // In "note" mode, the rate knob sets the pitch, with 4
                            // octaves span.
                            leftValue = Map(v, 0.f, 1.f, -24, 24);
                            leftValue = std::pow(2.f, leftValue / 12);
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
                        }
                        looper.SetReadRate(Channel::LEFT, leftValue);
                    }

                    if (Channel::BOTH == channel || Channel::RIGHT == channel)
                    {
                        float v = (Channel::BOTH == channel) ? fclamp(value + deltaValues[Channel::RIGHT][idx], 0.f, 1.f) : value;

                        if (StereoLooper::NoteMode::NOTE == looper.noteModeRight)
                        {
                            // In "note" mode, the rate knob sets the pitch, with 4
                            // octaves span.
                            rightValue = std::floor(Map(v, 0.f, 1.f, -24, 24)) - 24;
                            rightValue = std::pow(2.f, rightValue / 12);
                        }
                        else if (StereoLooper::NoteMode::FLANGER == looper.noteModeRight)
                        {
                            // In "note" mode, the rate knob sets the pitch, with 4
                            // octaves span.
                            rightValue = Map(v, 0.f, 1.f, -24, 24);
                            rightValue = std::pow(2.f, rightValue / 12);
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
                        }

                        looper.SetReadRate(Channel::RIGHT, rightValue);
                    }
                }
                break;
            // Freeze
            case DaisyVersio::KNOB_6:
                if (Channel::GLOBAL == channel)
                {
                    looper.stereoWidth = Map(value, 0.f, 1.f, 0.f, 1.f);
                }
                else
                {
                    if (Channel::BOTH == channel || Channel::LEFT == channel)
                    {
                        float leftValue = (Channel::BOTH == channel) ? fclamp(value + deltaValues[Channel::LEFT][idx], 0.f, 1.f) : value;
                        looper.SetFreeze(Channel::LEFT, leftValue);
                    }
                    if (Channel::BOTH == channel || Channel::RIGHT == channel)
                    {
                        float rightValue = (Channel::BOTH == channel) ? fclamp(value + deltaValues[Channel::RIGHT][idx], 0.f, 1.f) : value;
                        looper.SetFreeze(Channel::RIGHT, rightValue);
                    }
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

    inline void ProcessUi()
    {
        if (looper.IsStartingUp())
        {
            if (startUp)
            {
                startUp = false;

                // Init the dry/wet mix parameter.
                knobValues[DaisyVersio::KNOB_0] = knobs[DaisyVersio::KNOB_0].Process();
                ProcessParameter(DaisyVersio::KNOB_0, knobValues[DaisyVersio::KNOB_0], Channel::BOTH);

                // Init the global parameters.
                globalValues[DaisyVersio::KNOB_0] = 1.f / kMaxGain; // Input gain (unity)
                ProcessParameter(DaisyVersio::KNOB_0, globalValues[DaisyVersio::KNOB_0], Channel::GLOBAL);
                globalValues[DaisyVersio::KNOB_1] = 0.f; // Channels offset (0)
                ProcessParameter(DaisyVersio::KNOB_1, globalValues[DaisyVersio::KNOB_1], Channel::GLOBAL);
                globalValues[DaisyVersio::KNOB_2] = 0.5f; // Filter type (BP)
                ProcessParameter(DaisyVersio::KNOB_2, globalValues[DaisyVersio::KNOB_2], Channel::GLOBAL);
                globalValues[DaisyVersio::KNOB_3] = 0.f; // Loop sync (off)
                ProcessParameter(DaisyVersio::KNOB_3, globalValues[DaisyVersio::KNOB_3], Channel::GLOBAL);
                globalValues[DaisyVersio::KNOB_4] = 0.5f; // Filter level (50%)
                ProcessParameter(DaisyVersio::KNOB_4, globalValues[DaisyVersio::KNOB_4], Channel::GLOBAL);
                globalValues[DaisyVersio::KNOB_5] = 0.f; // Rate slew (0)
                ProcessParameter(DaisyVersio::KNOB_5, globalValues[DaisyVersio::KNOB_5], Channel::GLOBAL);
                globalValues[DaisyVersio::KNOB_6] = 1.f; // Stereo image (normal)
                ProcessParameter(DaisyVersio::KNOB_6, globalValues[DaisyVersio::KNOB_6], Channel::GLOBAL);
            }

            return;
        }

        // The looper is buffering.
        if (looper.IsBuffering())
        {
            buffering = true;
            LedMeter(looper.GetBufferSamples(Channel::LEFT) / static_cast<float>(kBufferSamples), ColorName::COLOR_RED);

            // Stop buffering.
            if (hw.tap.RisingEdge() || (hw.gate.Trig() && !first))
            {
                ClearLeds();
                looper.mustStopBuffering = true;
            }

            return;
        }

        // The looper is ready, do some configuration before start.
        if (looper.IsReady())
        {
            if (!buffering)
            {
                return;
            }
            buffering = false;

            currentTriggerMode = static_cast<Looper::TriggerMode>(hw.sw[DaisyVersio::SW_1].Read());
            looper.SetTriggerMode(currentTriggerMode);

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

            looper.Start();

            return;
        }

        // At this point the looper is running, do the normal UI loop.

        if (!recordingArmed)
        {
            if (Channel::GLOBAL == currentChannel)
            {
                LedMeter(1.f, looper.GetLoopSync() ? ColorName::COLOR_ICE : ColorName::COLOR_CREAM);
            }
            else if (Channel::BOTH == currentChannel)
            {
                Channel max = (looper.GetLoopLength(Channel::LEFT) >= looper.GetLoopLength(Channel::RIGHT)) ? Channel::LEFT : Channel::RIGHT;
                // Show the loop position of the longest channel.
                if (looper.IsFrozen())
                {
                    LedMeter((looper.GetLoopStart(max) + looper.GetReadPos(max)) / looper.GetBufferSamples(max), channelColor[max], 4, looper.GetLoopStart(max) / looper.GetLoopLength(max));
                }
                else
                {
                    // Delay mode.
                    if (looper.HasLoopSync())
                    {
                        LedMeter(looper.GetReadPos(max) / looper.GetLoopLength(max), channelColor[max]);
                    }
                    // Looper mode.
                    else
                    {
                        LedMeter(looper.GetWritePos(max) / looper.GetBufferSamples(max), channelColor[max]);
                    }
                }
            }
            else if (Channel::GLOBAL != currentChannel)
            {
                // Show the loop position of the current channel.
                LedMeter(looper.GetReadPos(currentChannel) / looper.GetLoopLength(currentChannel), channelColor[currentChannel]);
            }
        }

        HandleTriggerSwitch();
        HandleChannelSwitch();

        ProcessKnob(DaisyVersio::KNOB_3); // Size
        ProcessKnob(DaisyVersio::KNOB_1); // Start
        ProcessKnob(DaisyVersio::KNOB_6); // Freeze

        ProcessKnob(DaisyVersio::KNOB_0); // Blend
        ProcessKnob(DaisyVersio::KNOB_2); // Tone
        ProcessKnob(DaisyVersio::KNOB_4); // Decay
        ProcessKnob(DaisyVersio::KNOB_5); // Rate

        // Handle button press.
        if (hw.tap.RisingEdge() && !buttonPressed)
        {
            buttonPressed = true;
            if (looper.IsGateMode())
            {
                looper.Gate(true);
            }
            else
            {
                buttonHoldStartTime = ms;
            }
        }

        // Handle button release.
        if (hw.tap.FallingEdge() && buttonPressed)
        {
            buttonPressed = false;
            if (ButtonHoldMode::GLOBAL == buttonHoldMode)
            {
                GlobalMode(true);
                buttonHoldMode = ButtonHoldMode::NO_MODE;
            }
            else if (ButtonHoldMode::ARM == buttonHoldMode)
            {
                recordingArmed = true;
                LedMeter(1.f, ColorName::COLOR_RED);
                buttonHoldMode = ButtonHoldMode::NO_MODE;
            }
            else
            {
                if (recordingArmed)
                {
                    looper.mustResetLooper = true;
                    recordingArmed = false;
                }
                else if (looper.IsGateMode())
                {
                    looper.Gate(false);
                }
                else if (ms - buttonHoldStartTime <= kMaxMsHoldForTrigger)
                {
                    if (Channel::GLOBAL == currentChannel)
                    {
                        GlobalMode(false);
                    }
                    else
                    {
                        LedMeter(1.f, ColorName::COLOR_PINK);
                        looper.mustRetrigger = true;
                    }
                }
            }
        }

        // Do something while the button is pressed.
        if (buttonPressed && !looper.IsGateMode())
        {
            if (recordingArmed)
            {
                if (ms - buttonHoldStartTime > 1000.f)
                {
                    recordingArmed = false;
                    buttonPressed = false;
                }
            }
            else
            {
                if (ms - buttonHoldStartTime > kMaxMsHoldForTrigger)
                {
                    buttonHoldMode = ButtonHoldMode::GLOBAL;
                    LedMeter(1.f, looper.GetLoopSync() ? ColorName::COLOR_ICE : ColorName::COLOR_CREAM);
                }
                if (ms - buttonHoldStartTime > 1500.f)
                {
                    if (Channel::GLOBAL == currentChannel)
                    {
                        GlobalMode(false);
                    }
                    buttonHoldMode = ButtonHoldMode::ARM;
                    LedMeter(1.f, ColorName::COLOR_RED);
                }
            }
        }

        if (Channel::GLOBAL != currentChannel && ButtonHoldMode::NO_MODE == buttonHoldMode && !first)
        {
            if (looper.IsGateMode())
            {
                if (hw.Gate())
                {
                    looper.Gate(true);
                    gateTriggered = true;
                }
                else if (gateTriggered) {
                    looper.Gate(false);
                    gateTriggered = false;
                }
            }
            else if (hw.gate.Trig())
            {
                if (recordingArmed)
                {
                    GlobalMode(false);
                    looper.mustResetLooper = true;
                    recordingArmed = false;
                }
                else
                {
                    LedMeter(1.f, ColorName::COLOR_PINK);
                    looper.mustRetrigger = true;
                }
            }
        }

        first = false;
    }

    inline void InitUi()
    {
        colors[ColorName::COLOR_RED].Init(1.f, 0.f, 0.f); // Red (buffering)

        colors[ColorName::COLOR_WHITE].Init(0.9f, 0.9f, 0.9f); // (size noon)

        // Looper mode.
        colors[ColorName::COLOR_CREAM].Init(0.8f, 0.6f, 0.4f); // Ice (global mode)
        colors[ColorName::COLOR_GREEN].Init(0.f, 1.f, 0.f); // (size ccw)
        colors[ColorName::COLOR_LIME].Init(0.7f, 0.8f, 0.2f); // (size ccw)
        colors[ColorName::COLOR_YELLOW].Init(1.f, 0.7f, 0.f); // Yellow (flanger mode, looper)
        colors[ColorName::COLOR_ORANGE].Init(1.f, 0.4f, 0.f); // Orange (size cw)

        // Delay mode.

        colors[ColorName::COLOR_ICE].Init(0.5f, 0.5f, 1.f); // Ice (global mode)
        colors[ColorName::COLOR_PURPLE].Init(0.5f, 0.f, 1.f); // Purple (size ccw)
        colors[ColorName::COLOR_PINK].Init(0.5f, 0.4f, 1.f); // Purple (size ccw)
        colors[ColorName::COLOR_AQUA].Init(0.2f, 0.5f, 1.f); // Blue (size 10 o'clock)
        colors[ColorName::COLOR_BLUE].Init(0.f, 0.f, 1.f); // Blue (size 10 o'clock)
        //colors[ColorName::COLOR_PINK].Init(0.9f, 0.f, 0.4f); // Hot pink
        //colors[ColorName::COLOR_PINK].Init(1.f, 0.f, 1.f); // Magenta (trigger/gate)

    }
}