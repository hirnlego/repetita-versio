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

    char currentLooper{StereoLooper::BOTH};

    UiEventQueue eventQueue;

    float knobValues[7]{};
    enum class MenuClickOp
    {
        TRIGGER,
        CLEAR,
        RESET,
    };
    MenuClickOp clickOp{MenuClickOp::TRIGGER};

    StereoLooper::TriggerMode currentTriggerMode{};

    // 0: center, 1: left, 2: right
    static void HandleFlowSwitch()
    {
        if (hw.sw[DaisyVersio::SW_1].Read() == currentTriggerMode)
        {
            return;
        }

        switch (hw.sw[DaisyVersio::SW_1].Read())
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

    // 0: center, 1: left, 2: right
    static void SwitchToChannel(char pos)
    {
        switch (pos)
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

    inline void TrigLed(int idx, bool active)
    {
        if (active)
        {
            hw.leds[idx].Set(1, 1, 1);
        }
        else
        {
            hw.leds[idx].Set(0, 0, 0);
        }
        hw.UpdateLeds();
    }

    inline static void ProcessEvent(const UiEventQueue::Event &e)
    {
        switch (e.type)
        {
        case UiEventQueue::Event::EventType::buttonPressed:
            break;

        case UiEventQueue::Event::EventType::buttonReleased:

            /*
            else if (clickOp == MenuClickOp::TRIGGER)
            {
                looper.Restart();
            }
            else if (clickOp == MenuClickOp::CLEAR)
            {
                // Unfreeze if frozen.
                if (looper.IsFrozen())
                {
                    looper.ToggleFreeze();
                }
                // Clear the buffer.
                looper.mustClearBuffer = true;
                clickOp = MenuClickOp::TRIGGER;
            }
            else if (clickOp == MenuClickOp::RESET)
            {
                // Reset the looper.
                looper.mustResetLooper = true;
                clickOp = MenuClickOp::TRIGGER;
            }
            */
            break;

        default:
            break;
        }
    }

    inline void UpdateLeds()
    {
        if (looper.IsBuffering())
        {
            hw.leds[hw.LED_0].SetRed(1);
        }
        else
        {
            hw.leds[hw.LED_0].SetRed(0);
        }
        if (looper.IsFrozen())
        {
            hw.leds[hw.LED_0].SetBlue(1);
        }
        else
        {
            hw.leds[hw.LED_0].SetBlue(0);
        }
        hw.UpdateLeds();
    }

    inline void ProcessPot(int idx)
    {
        float val = hw.GetKnobValue(idx);
        if (std::abs(knobValues[idx] - val) > kMinValueDelta)
        {
            switch (idx)
            {
                // Blend
                case DaisyVersio::KNOB_0:
                    looper.nextMix = val;
                    break;
                // Start
                case DaisyVersio::KNOB_1:
                    looper.SetLoopStart(currentLooper, fmap(val, 0, looper.GetBufferSamples(StereoLooper::LEFT)));
                    break;
                // Tone
                case DaisyVersio::KNOB_2:
                    looper.nextFilterValue = fmap(val, 0, 1000.f);
                    break;
                // Flip
                case DaisyVersio::KNOB_3:
                    //samples *= (currentLoopLength >= kMinSamplesForTone) ? std::floor(currentLoopLength * 0.1f) : kMinLoopLengthSamples;
                    //currentLoopLength += samples;
                    looper.SetLoopLength(currentLooper, fmap(val, kMinLoopLengthSamples, looper.GetBufferSamples(StereoLooper::LEFT)));
                    break;
                // Decay
                case DaisyVersio::KNOB_4:
                    looper.nextFeedback = val;
                    break;
                // Speed
                case DaisyVersio::KNOB_5:
                    if (val < 0.5f)
                    {
                        looper.SetReadRate(currentLooper, fmap(val * 2, kMinSpeedMult, 1.f));
                    }
                    else
                    {
                        looper.SetReadRate(currentLooper, fmap((val * 2) - 1, 1.f, kMaxSpeedMult));
                    }
                    break;
                // Thaw
                case DaisyVersio::KNOB_6:
                    looper.SetFreeze(val);
                    break;

                default:
                    break;
            }

            knobValues[idx] = val;
        }
    }

    inline void GenerateUiEvents()
    {
        if (!looper.IsStartingUp())
        {
            if (hw.tap.RisingEdge())
            {
                TrigLed(0, true);
                if (looper.IsBuffering())
                {
                    // Stop buffering.
                    looper.mustStopBuffering = true;
                }
                else
                {
                    switch (looper.GetTriggerMode())
                    {
                        case StereoLooper::TriggerMode::GATE:
                            looper.mustStart = true;
                            break;
                        case StereoLooper::TriggerMode::TRIGGER:
                            looper.mustRestart = true;
                            break;
                    }
                }
            }
            if (hw.tap.FallingEdge())
            {
                TrigLed(0, false);
                if (!looper.IsBuffering())
                {
                    if (StereoLooper::TriggerMode::GATE == looper.GetTriggerMode())
                    {
                        looper.mustStop = true;
                    }
                }
            }

            if (!looper.IsBuffering())
            {
                if (hw.tap.TimeHeldMs() >= 1000.f)
                {
                    clickOp = MenuClickOp::CLEAR;
                }
                if (hw.tap.TimeHeldMs() >= 5000.f)
                {
                    clickOp = MenuClickOp::RESET;
                }

                if (hw.Gate())
                {
                    looper.mustRestart = true;
                    TrigLed(3, true);
                }
                else
                {
                    TrigLed(3, false);
                }

                HandleFlowSwitch();

                ProcessPot(DaisyVersio::KNOB_5); // Rate
                ProcessPot(DaisyVersio::KNOB_3); // Flip
                ProcessPot(DaisyVersio::KNOB_1); // Start
                ProcessPot(DaisyVersio::KNOB_2); // Tone
                ProcessPot(DaisyVersio::KNOB_4); // Decay
                ProcessPot(DaisyVersio::KNOB_6); // Thaw
                ProcessPot(DaisyVersio::KNOB_0); // Blend
            }
        }
    }

    inline void ProcessUi()
    {
        while (!eventQueue.IsQueueEmpty())
        {
            UiEventQueue::Event e = eventQueue.GetAndRemoveNextEvent();
            if (e.type != UiEventQueue::Event::EventType::invalid)
            {
                ProcessEvent(e);
            }
        }

        UpdateLeds();
    }
}