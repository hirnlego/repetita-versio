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
        SETTINGS,
    };
    Channel prevChannel{Channel::BOTH};
    Channel currentChannel{Channel::BOTH};

    float channelValues[3][7]{};
    float deltaValues[2][7]{};
    float knobValues[7]{};

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

    enum TriggerMode
    {
        LOOP,
        REC,
        ONESHOT,
    };
    enum class ButtonHoldMode
    {
        NO_MODE,
        SETTINGS,
        ARM,
    };
    ButtonHoldMode buttonHoldMode{ButtonHoldMode::NO_MODE};
    TriggerMode currentTriggerMode{};
    bool buttonPressed{};
    bool gateTriggered{};
    int32_t buttonHoldStartTime{};
    bool recordingArmed{};
    bool recordingLeftTriggered{};
    bool recordingRightTriggered{};

    bool startUp{true};
    bool first{true};
    bool buffering{};

    struct Settings
    {
        float inputGain;
        float filterType;
        float loopSync;
        float filterLevel;
        float rateSlew;
        float stereoWidth;
        float degradation;
    };

    Settings defaultSettings{1.f / kMaxGain, 0.5f, 0.f, 0.5f, 0.f, 1.f, 0.f};
    Settings localSettings{};

    PersistentStorage<Settings> storage(hw.seed.qspi);

    bool mustUpdateStorage{};

    bool operator!=(const Settings& lhs, const Settings& rhs)
    {
        return lhs.inputGain != rhs.inputGain || lhs.filterType != rhs.filterType || lhs.loopSync != rhs.loopSync || lhs.filterLevel != rhs.filterLevel || lhs.rateSlew != rhs.rateSlew || lhs.stereoWidth != rhs.stereoWidth || lhs.degradation != rhs.degradation;
    }

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
        // idx %= length;
        // short odx = static_cast<short>(std::floor(offset * (length - 1)));
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
        if (!looper.IsBuffering() && Channel::SETTINGS != currentChannel)
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

    void SettingsMode(bool active)
    {
        if (active)
        {
            prevChannel = currentChannel;
            currentChannel = Channel::SETTINGS;
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
        if (Channel::SETTINGS == currentChannel)
        {
            SettingsMode(false);
        }

        currentChannel = prevChannel = static_cast<Channel>(value);
    }

    // 0: center, 1: left, 2: right
    static void HandleTriggerSwitch(bool init = false)
    {
        short value = hw.sw[DaisyVersio::SW_1].Read();
        if (value == currentTriggerMode && !init)
        {
            return;
        }

        currentTriggerMode = static_cast<TriggerMode>(value);

        switch (currentTriggerMode)
        {
        case TriggerMode::REC:
            looper.mustStopWriting = true;
            looper.SetLooping(true);
            break;
        case TriggerMode::LOOP:
            recordingLeftTriggered = false;
            recordingRightTriggered = false;
            looper.mustStartWritingLeft = true;
            looper.mustStartWritingRight = true;
            looper.mustStartReading = true;
            looper.SetLooping(true);
            break;
        case TriggerMode::ONESHOT:
            looper.mustStopReading = true;
            looper.SetLooping(false);
            break;
        }
    }

    void HandleTriggerRecording()
    {
        if (recordingLeftTriggered)
        {
            if (Channel::BOTH == currentChannel || Channel::LEFT == currentChannel)
            {
                looper.mustStopWritingLeft = true;
            }
            recordingLeftTriggered = false;
        }
        else
        {
            if (Channel::BOTH == currentChannel || Channel::LEFT == currentChannel)
            {
                looper.mustStartWritingLeft = true;
            }
            recordingLeftTriggered = true;
        }
        if (recordingRightTriggered)
        {
            if (Channel::BOTH == currentChannel || Channel::RIGHT == currentChannel)
            {
                looper.mustStopWritingRight = true;
            }
            recordingRightTriggered = false;
        }
        else
        {
            if (Channel::BOTH == currentChannel || Channel::RIGHT == currentChannel)
            {
                looper.mustStartWritingRight = true;
            }
            recordingRightTriggered = true;
        }
    }

    inline void ProcessParameter(short idx, float value, Channel channel)
    {
        // Keep track of parameters values only after startup.
        if (!looper.IsStartingUp())
        {
            if (Channel::SETTINGS != channel)
            {
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
            if (Channel::SETTINGS == channel)
            {
                localSettings.inputGain = value;
                looper.inputGain = value * kMaxGain;
                mustUpdateStorage = true;
            }
            else
            {
                looper.dryWetMix = value;
            }
            break;
        // Start
        case DaisyVersio::KNOB_1:
        {
            if (Channel::SETTINGS == channel)
            {
                localSettings.stereoWidth = looper.stereoWidth = value;
                mustUpdateStorage = true;
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
            if (Channel::SETTINGS == channel)
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
                localSettings.filterType = value;
                mustUpdateStorage = true;
            }
            else
            {
                looper.SetFilterValue(Map(value, 0.f, 1.f, 0.f, kMaxFilterValue));
            }
            break;
        // Size
        case DaisyVersio::KNOB_3:
        {
            if (Channel::SETTINGS == channel)
            {
                localSettings.loopSync = value;
                looper.SetLoopSync(value >= 0.5);
                mustUpdateStorage = true;
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
            if (Channel::SETTINGS == channel)
            {
                localSettings.filterLevel = looper.filterLevel = value;
                mustUpdateStorage = true;
            }
            else
            {
                looper.feedback = value;
            }
            break;
        // Rate
        case DaisyVersio::KNOB_5:
            if (Channel::SETTINGS == channel)
            {
                localSettings.rateSlew = value;
                looper.rateSlew = Map(value, 0.f, 1.f, 0.f, kMaxRateSlew);
                mustUpdateStorage = true;
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
            if (Channel::SETTINGS == channel)
            {
                localSettings.degradation = value;
                looper.SetDegradation(value);
                mustUpdateStorage = true;
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
                // TODO: Restore defaults when holding the button during startup.
                // storage.RestoreDefaults();

                startUp = false;

                // Init the dry/wet mix parameter.
                knobValues[DaisyVersio::KNOB_0] = knobs[DaisyVersio::KNOB_0].Process();
                ProcessParameter(DaisyVersio::KNOB_0, knobValues[DaisyVersio::KNOB_0], Channel::BOTH);

                // Init the settings.
                Settings &storedSettings = storage.GetSettings();
                ProcessParameter(DaisyVersio::KNOB_0, storedSettings.inputGain, Channel::SETTINGS);
                ProcessParameter(DaisyVersio::KNOB_1, storedSettings.stereoWidth, Channel::SETTINGS);
                ProcessParameter(DaisyVersio::KNOB_2, storedSettings.filterType, Channel::SETTINGS);
                ProcessParameter(DaisyVersio::KNOB_3, storedSettings.loopSync, Channel::SETTINGS);
                ProcessParameter(DaisyVersio::KNOB_4, storedSettings.filterLevel, Channel::SETTINGS);
                ProcessParameter(DaisyVersio::KNOB_5, storedSettings.rateSlew, Channel::SETTINGS);
                ProcessParameter(DaisyVersio::KNOB_6, storedSettings.degradation, Channel::SETTINGS);
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

        // The looper is ready, do some configuration before starting.
        if (looper.IsReady())
        {
            if (!buffering)
            {
                return;
            }
            buffering = false;

            HandleTriggerSwitch(true);

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
            if (Channel::SETTINGS == currentChannel)
            {
                LedMeter(1.f, looper.GetLoopSync() ? ColorName::COLOR_ICE : ColorName::COLOR_CREAM);
            }
            else if (Channel::BOTH == currentChannel)
            {
                Channel max = (looper.GetLoopLength(Channel::LEFT) >= looper.GetLoopLength(Channel::RIGHT)) ? Channel::LEFT : Channel::RIGHT;
                // Show the loop position of the longest channel.
                ColorName color = recordingLeftTriggered || recordingRightTriggered ? ColorName::COLOR_RED : channelColor[max];
                // Delay mode.
                if (looper.HasLoopSync())
                {
                    LedMeter(looper.GetReadPos(max) / looper.GetLoopLength(max), color);
                }
                // Looper mode.
                else
                {
                    LedMeter(looper.GetWritePos(max) / looper.GetBufferSamples(max), color);
                }
            }
            else if (Channel::SETTINGS != currentChannel)
            {
                // Show the loop position of the current channel.
                ColorName color = recordingLeftTriggered || recordingRightTriggered ? ColorName::COLOR_RED : channelColor[currentChannel];
                LedMeter(looper.GetReadPos(currentChannel) / looper.GetLoopLength(currentChannel), color);
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
            buttonHoldStartTime = System::GetNow();
        }

        // Handle button release.
        if (hw.tap.FallingEdge() && buttonPressed)
        {
            buttonPressed = false;
            if (ButtonHoldMode::SETTINGS == buttonHoldMode)
            {
                SettingsMode(true);
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
                else if (System::GetNow() - buttonHoldStartTime <= kMaxMsHoldForTrigger)
                {
                    if (Channel::SETTINGS == currentChannel)
                    {
                        SettingsMode(false);
                    }
                    else
                    {
                        LedMeter(1.f, ColorName::COLOR_PINK);
                        if (TriggerMode::ONESHOT == currentTriggerMode)
                        {
                            looper.mustRestart = true;
                        }
                        else if (TriggerMode::REC == currentTriggerMode)
                        {
                            HandleTriggerRecording();
                        }
                        else
                        {
                            looper.mustRetrigger = true;
                        }
                    }
                }
            }
        }

        // Do something while the button is pressed.
        if (buttonPressed && !recordingLeftTriggered && !recordingRightTriggered)
        {
            if (recordingArmed)
            {
                if (System::GetNow() - buttonHoldStartTime > 1000.f)
                {
                    recordingArmed = false;
                    buttonPressed = false;
                }
            }
            else
            {
                if (System::GetNow() - buttonHoldStartTime > kMaxMsHoldForTrigger)
                {
                    buttonHoldMode = ButtonHoldMode::SETTINGS;
                    LedMeter(1.f, looper.GetLoopSync() ? ColorName::COLOR_ICE : ColorName::COLOR_CREAM);
                }
                if (System::GetNow() - buttonHoldStartTime > 1500.f)
                {
                    if (Channel::SETTINGS == currentChannel)
                    {
                        SettingsMode(false);
                    }
                    buttonHoldMode = ButtonHoldMode::ARM;
                    LedMeter(1.f, ColorName::COLOR_RED);
                }
            }
        }

        if (Channel::SETTINGS != currentChannel && ButtonHoldMode::NO_MODE == buttonHoldMode && !first)
        {
            if (hw.gate.Trig())
            {
                if (recordingArmed)
                {
                    SettingsMode(false);
                    looper.mustResetLooper = true;
                    recordingArmed = false;
                }
                else if (TriggerMode::REC == currentTriggerMode)
                {
                    HandleTriggerRecording();
                }
                else
                {
                    LedMeter(1.f, ColorName::COLOR_PINK);
                    if (TriggerMode::ONESHOT == currentTriggerMode)
                    {
                        looper.mustRestart = true;
                    }
                    else
                    {
                        looper.mustRetrigger = true;
                    }
                }
            }
        }

        first = false;
    }

    inline void InitUi()
    {
        storage.Init(defaultSettings);

        colors[ColorName::COLOR_RED].Init(1.f, 0.f, 0.f); // Red (buffering)

        colors[ColorName::COLOR_WHITE].Init(0.9f, 0.9f, 0.9f); // (size noon)

        // Looper mode.
        colors[ColorName::COLOR_CREAM].Init(0.8f, 0.6f, 0.4f); // Ice (settings mode)
        colors[ColorName::COLOR_GREEN].Init(0.f, 1.f, 0.f);    // (size ccw)
        colors[ColorName::COLOR_LIME].Init(0.7f, 0.8f, 0.2f);  // (size ccw)
        colors[ColorName::COLOR_YELLOW].Init(1.f, 0.7f, 0.f);  // Yellow (flanger mode, looper)
        colors[ColorName::COLOR_ORANGE].Init(1.f, 0.4f, 0.f);  // Orange (size cw)

        // Delay mode.

        colors[ColorName::COLOR_ICE].Init(0.5f, 0.5f, 1.f);   // Ice (settings mode)
        colors[ColorName::COLOR_PURPLE].Init(0.5f, 0.f, 1.f); // Purple (size ccw)
        colors[ColorName::COLOR_PINK].Init(0.5f, 0.4f, 1.f);  // Purple (size ccw)
        colors[ColorName::COLOR_AQUA].Init(0.2f, 0.5f, 1.f);  // Blue (size 10 o'clock)
        colors[ColorName::COLOR_BLUE].Init(0.f, 0.f, 1.f);    // Blue (size 10 o'clock)
        // colors[ColorName::COLOR_PINK].Init(0.9f, 0.f, 0.4f); // Hot pink
        // colors[ColorName::COLOR_PINK].Init(1.f, 0.f, 1.f); // Magenta (trigger)
    }

    void ProcessStorage()
    {
        if (mustUpdateStorage)
        {
            if (!looper.IsStartingUp())
            {
                Settings &storedSettings = storage.GetSettings();
                storedSettings = localSettings;
                storage.Save();
            }
            mustUpdateStorage = false;
        }
    }
}