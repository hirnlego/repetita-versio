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

    float knobUpdates[7]{};
    float knobValues[3][7]{};
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
    short currentLooper{StereoLooper::BOTH};
    bool noteMode{};
    bool flangerMode{};
    bool gateTriggered{};
    bool knobChanged{};
    bool mustTurnOffLeds{};
    float knobChangeStartTime{};
    float buttonHoldStartTime{};

    enum KnobStatus
    {
        MOVING,
        STILL,
        LEDSON,
        LEDSOFF,
    };
    KnobStatus knobStatus{KnobStatus::STILL};

    UiEventQueue eventQueue;

    // 0: center, 1: left, 2: right
    static void HandleLooperSwitch()
    {
        short value = hw.sw[DaisyVersio::SW_0].Read() - 1;
        value = value < 0 ? 2 : value;
        if (value == currentLooper)
        {
            return;
        }

        currentLooper = value;
        switch (value)
        {
        case 0:
        case 1:
            looper.SetMode(StereoLooper::Mode::DUAL);
            break;
        case 2:
            looper.SetMode(StereoLooper::Mode::MONO);
            break;
        default:
            break;
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

    inline void ProcessParameter(short idx, float value)
    {
        switch (idx)
        {
            // Blend
            case DaisyVersio::KNOB_0:
                looper.nextMix = value;
                break;
            // Start
            case DaisyVersio::KNOB_1:
                {
                    if (looper.IsDualMode())
                    {
                        looper.SetLoopStart(currentLooper, Map(value, 0.f, 1.f, 0.f, looper.GetLoopEnd(currentLooper)));
                    }
                    else
                    {
                        looper.SetLoopStart(StereoLooper::LEFT, Map(knobValues[StereoLooper::LEFT][idx] + value, 0.f, 1.f, 0.f, looper.GetLoopEnd(StereoLooper::LEFT)));
                        looper.SetLoopStart(StereoLooper::RIGHT, Map(knobValues[StereoLooper::RIGHT][idx] + value, 0.f, 1.f, 0.f, looper.GetLoopEnd(StereoLooper::RIGHT)));
                    }
                }
                break;
            // Tone
            case DaisyVersio::KNOB_2:
                looper.nextFilterValue = Map(value, 0.f, 1.f, 0.f, 1000.f);
                break;
            // Flip
            case DaisyVersio::KNOB_3:
                {
                    if (looper.IsDualMode())
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
                    else
                    {
                        looper.SetLoopLength(StereoLooper::LEFT, knobValues[StereoLooper::LEFT][idx] * value);
                        looper.SetLoopLength(StereoLooper::RIGHT, knobValues[StereoLooper::RIGHT][idx] * value);
                    }
                }
                break;
            // Decay
            case DaisyVersio::KNOB_4:
                looper.nextFeedback = value;
                break;
            // Speed
            case DaisyVersio::KNOB_5:
                if (looper.IsDualMode())
                {
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
                        // TODO
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
                }
                else
                {
                    looper.SetReadRate(StereoLooper::LEFT, knobValues[StereoLooper::LEFT][idx] + value);
                    looper.SetReadRate(StereoLooper::RIGHT, knobValues[StereoLooper::RIGHT][idx] + value);
                }
                /*
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
                */
                break;
            // Freeze
            case DaisyVersio::KNOB_6:
                looper.SetFreeze(value);
                break;

            default:
                break;
        }
        knobValues[currentLooper][idx] = value;
    }

    bool mustPickUp{};

    inline void ProcessKnob(int idx, bool hasPickup)
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

        // Process the parameter only if the knob is actually moving.
        if (std::abs(knobUpdates[idx] - value) > kMinValueDelta)
        {
            if (hasPickup)
            {
                ClearLeds();
                float v = std::abs(knobValues[currentLooper][idx] - value);
                short l = value > knobValues[currentLooper][idx] ? 1 - std::round(v) : 2 + std::round(v);
                mustPickUp = v > kMinPickupValueDelta;
                if (mustPickUp)
                {
                    hw.SetLed(l, 1.f, 1.f, 1.f);
                }
                else
                {
                    ProcessParameter(idx, value);
                }
            }
            else
            {
                ProcessParameter(idx, value);
            }

            knobUpdates[idx] = value;
        }
    }

    short buttonWorkflow{};

    inline void ProcessUi()
    {
        if (looper.IsStartingUp())
        {
            // Init the parameters that, at this moment, can be safely set to
            // their respective knobs values (those not affected by the initial
            // buffering).
            knobUpdates[DaisyVersio::KNOB_0] = knobs[DaisyVersio::KNOB_0].Process(); // Blend
            knobValues[StereoLooper::LEFT][DaisyVersio::KNOB_0] = knobUpdates[DaisyVersio::KNOB_0];
            knobValues[StereoLooper::RIGHT][DaisyVersio::KNOB_0] = knobUpdates[DaisyVersio::KNOB_0];
            ProcessParameter(DaisyVersio::KNOB_0, knobUpdates[DaisyVersio::KNOB_0]);
            knobUpdates[DaisyVersio::KNOB_2] = knobs[DaisyVersio::KNOB_2].Process(); // Tone
            knobValues[StereoLooper::LEFT][DaisyVersio::KNOB_2] = knobUpdates[DaisyVersio::KNOB_2];
            knobValues[StereoLooper::RIGHT][DaisyVersio::KNOB_2] = knobUpdates[DaisyVersio::KNOB_2];
            ProcessParameter(DaisyVersio::KNOB_2, knobUpdates[DaisyVersio::KNOB_2]);
            knobUpdates[DaisyVersio::KNOB_4] = knobs[DaisyVersio::KNOB_4].Process(); // Decay
            knobValues[StereoLooper::LEFT][DaisyVersio::KNOB_4] = knobUpdates[DaisyVersio::KNOB_4];
            knobValues[StereoLooper::RIGHT][DaisyVersio::KNOB_4] = knobUpdates[DaisyVersio::KNOB_4];
            ProcessParameter(DaisyVersio::KNOB_4, knobUpdates[DaisyVersio::KNOB_4]);
            knobUpdates[DaisyVersio::KNOB_5] = knobs[DaisyVersio::KNOB_5].Process(); // Rate
            knobValues[StereoLooper::LEFT][DaisyVersio::KNOB_5] = knobUpdates[DaisyVersio::KNOB_5];
            knobValues[StereoLooper::RIGHT][DaisyVersio::KNOB_5] = knobUpdates[DaisyVersio::KNOB_5];
            ProcessParameter(DaisyVersio::KNOB_5, knobUpdates[DaisyVersio::KNOB_5]);

            // The others, just init the variables.
            knobUpdates[DaisyVersio::KNOB_1] = knobs[DaisyVersio::KNOB_1].Process(); // Start
            knobValues[StereoLooper::LEFT][DaisyVersio::KNOB_1] = knobUpdates[DaisyVersio::KNOB_1];
            knobValues[StereoLooper::RIGHT][DaisyVersio::KNOB_1] = knobUpdates[DaisyVersio::KNOB_1];
            knobUpdates[DaisyVersio::KNOB_3] = knobs[DaisyVersio::KNOB_3].Process(); // Size
            knobValues[StereoLooper::LEFT][DaisyVersio::KNOB_3] = knobUpdates[DaisyVersio::KNOB_3];
            knobValues[StereoLooper::RIGHT][DaisyVersio::KNOB_3] = knobUpdates[DaisyVersio::KNOB_3];
            knobUpdates[DaisyVersio::KNOB_6] = knobs[DaisyVersio::KNOB_6].Process(); // Freeze
            knobValues[StereoLooper::LEFT][DaisyVersio::KNOB_6] = knobUpdates[DaisyVersio::KNOB_6];
            knobValues[StereoLooper::RIGHT][DaisyVersio::KNOB_6] = knobUpdates[DaisyVersio::KNOB_6];
        }
        else {
            if (looper.IsBuffering())
            {
                LedMeter(looper.GetBufferSamples(StereoLooper::LEFT) / static_cast<float>(kBufferSamples), 7);
                if (hw.tap.RisingEdge() && !buttonWorkflow)
                {
                    // Stop buffering.
                    looper.mustStopBuffering = true;
                    ClearLeds();
                    //buttonWorkflow++;
                }
            }
            else
            {
                HandleLooperSwitch();
                HandleTriggerSwitch();

                // Handle button press.
                if (hw.tap.RisingEdge() && !buttonWorkflow)
                {
                    if (StereoLooper::BOTH == currentLooper)
                    {
                        buttonHoldStartTime = ms;
                    }
                    buttonWorkflow = 1;
                }

                // Handle button release.
                else if (hw.tap.FallingEdge() && buttonWorkflow)
                {
                    buttonWorkflow = 0;
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
                        case MenuClickOp::EDIT:
                            clickOp = MenuClickOp::TRIGGER;
                            break;
                    }
                }

                // Do something while the button is pressed.
                if (buttonWorkflow)
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
                            size_t time = ms - buttonHoldStartTime;
                            // Clear the buffer if the button has been held more than 2 sec.
                            if (1 == buttonWorkflow && time >= 2000.f)
                            {
                                LedMeter(1.f, 2);
                                buttonWorkflow++;
                            }
                            if (2 == buttonWorkflow && time >= 2350.f)
                            {
                                ClearLeds();
                                looper.mustClearBuffer = true;
                                buttonWorkflow++;
                            }
                            // Reset the looper if the button has been held more than 5 sec.
                            if (3 == buttonWorkflow && time >= 5000.f)
                            {
                                LedMeter(1.f, 7);
                                buttonWorkflow++;
                            }
                            if (4 == buttonWorkflow && time >= 5350.f)
                            {
                                ClearLeds();
                                looper.mustResetLooper = true;
                                buttonWorkflow = 0;
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
                    //ProcessKnob(DaisyVersio::KNOB_1, true); // Start
                    //ProcessKnob(DaisyVersio::KNOB_6, false); // Freeze
                }

                ProcessKnob(DaisyVersio::KNOB_3, true); // Size
            }

            ProcessKnob(DaisyVersio::KNOB_0, false); // Blend
            ProcessKnob(DaisyVersio::KNOB_2, false); // Tone
            ProcessKnob(DaisyVersio::KNOB_4, false); // Decay
            ProcessKnob(DaisyVersio::KNOB_5, true); // Rate
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