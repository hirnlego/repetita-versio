#include "repetita.h"
#include "ui.h"
#include <cstring>

using namespace wreath;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    UpdateClock();
    ProcessControls();
    ProcessUi();

    for (size_t i = 0; i < size; i++)
    {
        float leftIn{IN_L[i]};
        float rightIn{IN_R[i]};

        float leftOut{};
        float rightOut{};
        looper.Process(leftIn, rightIn, leftOut, rightOut);

        OUT_L[i] = leftOut;
        OUT_R[i] = rightOut;
    }
}

int main(void)
{
    InitHw();

    StereoLooper::Conf conf
    {
        StereoLooper::Mode::MONO,
        StereoLooper::TriggerMode::LOOP,
        Movement::NORMAL,
        Direction::FORWARD,
        1.0f
    };

    looper.Init(hw.AudioSampleRate(), conf);

    InitUi();

    hw.StartAudio(AudioCallback);

    while (1)
    {
        //ProcessUi();
    }
}