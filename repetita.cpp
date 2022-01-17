#include "repetita.h"
#include "ui.h"
#include <cstring>

using namespace wreath;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    UpdateClock();
    UpdateControls();
    GenerateUiEvents();

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
    InitHw(0.05f, 0.f);

    looper.Init(hw.AudioSampleRate());

    hw.StartAudio(AudioCallback);

    while (1)
    {
        ProcessUi();
    }
}