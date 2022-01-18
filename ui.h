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
    bool knobChanged[7]{};
    enum class MenuClickOp
    {
        TRIGGER,
        CLEAR,
        RESET,
    };
    MenuClickOp clickOp{MenuClickOp::TRIGGER};

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
            if (looper.IsBuffering())
            {
                // Stop buffering.
                looper.mustStopBuffering = true;
            }
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
        knobChanged[idx] = std::abs(knobValues[idx] - val) > kMinValueDelta;
        if (knobChanged[idx])
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
                    //looper.SetLoopLength(currentLooper, fmap(val, 0, looper.GetBufferSamples(StereoLooper::LEFT)));
                    break;
                // Decay
                case DaisyVersio::KNOB_4:
                    looper.nextFeedback = val;
                    break;
                // Speed
                case DaisyVersio::KNOB_5:
                    if (val < 0.5f)
                    {
                        //looper.SetReadRate(currentLooper, fmap(val * 2, kMinSpeedMult, 1.f));
                    }
                    else
                    {
                        //looper.SetReadRate(currentLooper, fmap((val * 2) - 1, 1.f, kMaxSpeedMult));
                    }
                    break;
                // Thaw
                case DaisyVersio::KNOB_6:
                    looper.SetFreeze(val);
                    break;

                default:
                    break;
            }

            knobChanged[idx] = val;
        }
    }

    inline void GenerateUiEvents()
    {
        if (!looper.IsStartingUp())
        {
            if (hw.tap.RisingEdge())
            {
                eventQueue.AddButtonPressed(0, 1);
                TrigLed(0, true);
            }
            if (hw.tap.FallingEdge())
            {
                eventQueue.AddButtonReleased(0);
                TrigLed(0, false);
            }

            if (hw.tap.TimeHeldMs() >= 1000.f)
            {
                clickOp = MenuClickOp::CLEAR;
            }
            if (hw.tap.TimeHeldMs() >= 5000.f)
            {
                clickOp = MenuClickOp::RESET;
            }

            if (hw.gate.Trig())
            {
                looper.Restart();
                //TrigLed(0, true);
            }

            ProcessPot(DaisyVersio::KNOB_0);
            ProcessPot(DaisyVersio::KNOB_1);
            ProcessPot(DaisyVersio::KNOB_2);
            ProcessPot(DaisyVersio::KNOB_3);
            ProcessPot(DaisyVersio::KNOB_4);
            ProcessPot(DaisyVersio::KNOB_5);
            ProcessPot(DaisyVersio::KNOB_6);
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