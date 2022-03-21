#pragma once

#include "daisy_versio.h"

namespace wreath
{
    using namespace daisy;

    // The minimum difference in parameter value to be registered.
    constexpr float kMinValueDelta{0.002f};
    // The minimum difference in parameter value to be considered picked up.
    constexpr float kMinPickupValueDelta{0.01f};
    // The trigger threshold value.
    constexpr float kTriggerThres{0.3f};
    // Maximum BPM supported.
    constexpr int kMaxBpm{300};

    DaisyVersio hw;

    char switch1Pos{};
    size_t switch1Begin{};
    size_t switch1End{};
    char switch2Pos{};
    size_t switch2Begin{};
    size_t switch2End{};

    static size_t begin{};
    static size_t end{};

    size_t ms{};
    size_t cv1Bpm{};

    QSPIHandle qspi;
    QSPIHandle::Config qspi_config;

    inline static int CalculateBpm()
    {
        end = ms;
        // Handle the ms reset.
        if (end < begin)
        {
            end += 10000;
        }

        return std::round((1000.f / (end - begin)) * 60);
    }

    Parameter knobs[DaisyVersio::KNOB_LAST]{};

    inline void InitHw()
    {
        hw.Init(true);
        hw.StartAdc();

        qspi_config.device = QSPIHandle::Config::Device::IS25LP064A;
        qspi_config.mode   = QSPIHandle::Config::Mode::MEMORY_MAPPED;

        qspi_config.pin_config.io0 = dsy_pin(DSY_GPIOF, 8);
        qspi_config.pin_config.io1 = dsy_pin(DSY_GPIOF, 9);
        qspi_config.pin_config.io2 = dsy_pin(DSY_GPIOF, 7);
        qspi_config.pin_config.io3 = dsy_pin(DSY_GPIOF, 6);
        qspi_config.pin_config.clk = dsy_pin(DSY_GPIOF, 10);
        qspi_config.pin_config.ncs = dsy_pin(DSY_GPIOG, 6);
        qspi.Init(qspi_config);

        for (short i = 0; i < DaisyVersio::KNOB_LAST; i++)
        {
            hw.knobs[i].SetCoeff(1.f); // No slew;
            knobs[i].Init(hw.knobs[i], 0.0f, 1.0f, Parameter::LINEAR);
        }
    }

    inline void UpdateClock()
    {
        /*
        if (ms > 10000)
        {
            ms = 0;
        }
        */
        ms++;
    }

    inline void ProcessControls()
    {
        hw.ProcessAllControls();
        hw.tap.Debounce();
        hw.UpdateLeds();

/*
        if (switch1Pos != hw.sw[0].Read())
        {
            switch1End = ms;
            if (switch1End - switch1Begin > 500.f)
            {
                switch1Pos = hw.sw[0].Read();
            }
            switch1Begin = ms;
        }

        if (switch2Pos != hw.sw[1].Read())
        {
            switch2End = ms;
            if (switch2End - switch2Begin > 500.f)
            {
                switch2Pos = hw.sw[1].Read();
            }
            switch2Begin = ms;
        }
*/
    }
}