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

    short currentMovement{};
    short currentLooper{0};
    UiEventQueue eventQueue;

    void UpdateControls()
    {
        ProcessControls();

        if (!looper.IsStartingUp())
        {
            // Handle CV1 as trigger input for resetting the read position to
            // the loop start point.
            looper.hasCvRestart = isCv1Connected;
            if (isCv1Connected)
            {
                looper.mustRestart = cv1Trigger;
            }

            // Handle CV2 as loop start point when frozen.
            if (looper.IsFrozen() && isCv2Connected)
            {
                //looper.SetLoopStart(fmap(cv2.Value(), 0, looper.GetBufferSamples(0)));
            }
        }
    }

    inline static void ProcessEvent(const UiEventQueue::Event &e)
    {
        switch (e.type)
        {
        case UiEventQueue::Event::EventType::buttonPressed:
            break;

        case UiEventQueue::Event::EventType::buttonReleased:
            break;

        case UiEventQueue::Event::EventType::encoderTurned:
            break;

        case UiEventQueue::Event::EventType::encoderActivityChanged:
            break;

        default:
            break;
        }
    }

    inline void GenerateUiEvents()
    {
        if (!looper.IsBuffering())
        {
        }
    }

    inline void ProcessUi()
    {
        UpdateControls();

        while (!eventQueue.IsQueueEmpty())
        {
            UiEventQueue::Event e = eventQueue.GetAndRemoveNextEvent();
            if (e.type != UiEventQueue::Event::EventType::invalid)
            {
                ProcessEvent(e);
            }
        }
    }
}