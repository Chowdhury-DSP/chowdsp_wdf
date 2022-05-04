#include <catch2/catch2.hpp>

#include "BassmanToneStack.h"
#include "BaxandallEQ.h"

constexpr double fs = 48000.0f;

void bassmanFreqTest (float lowPot, float highPot, float sineFreq, float expGainDB, float maxErr)
{
    Tonestack<double> tonestack;
    tonestack.prepare (fs);
    tonestack.setParams ((double) highPot, (double) lowPot, 1.0);

    double magnitude = 0.0;
    for (int n = 0; n < (int) fs; ++n)
    {
        const auto x = std::sin (2.0 * M_PI * (double) n * sineFreq / fs);
        const auto y = tonestack.processSample (x);

        if (n > 1000)
            magnitude = std::max (magnitude, std::abs (y));
    }

    auto actualGainDB = 20.0 * std::log10 (magnitude);
    REQUIRE (actualGainDB == Approx (expGainDB).margin (maxErr));
}

void baxandallFreqTest (float bassParam, float trebleParam, float sineFreq, float expGainDB, float maxErr)
{
    BaxandallWDF baxandall;
    baxandall.prepare (fs);
    baxandall.setParams (bassParam, trebleParam);

    float magnitude = 0.0f;
    for (int n = 0; n < (int) fs; ++n)
    {
        const auto x = std::sin (2.0f * (float) M_PI * (float) n * sineFreq / (float) fs);
        const auto y = baxandall.processSample (x);

        if (n > 1000)
            magnitude = std::max (magnitude, std::abs (y));
    }

    auto actualGainDB = 20.0f * std::log10 (magnitude);
    REQUIRE (actualGainDB == Approx (expGainDB).margin (maxErr));
}

TEST_CASE ("RType Test")
{
    SECTION ("Bassman Bass Test")
    {
        bassmanFreqTest (0.5f, 0.001f, 60.0f, -9.0f, 0.5f);
    }

    SECTION ("Bassman Treble Test")
    {
        bassmanFreqTest (0.999f, 0.999f, 15000.0f, 5.0f, 0.5f);
    }

    SECTION ("Baxandall Bass Test")
    {
        baxandallFreqTest (0.0001f, 0.1f, 20.0f, -3.0f, 0.5f);
    }

    SECTION ("Baxandall Treble Test")
    {
        baxandallFreqTest (0.1f, 0.0001f, 20000.0f, -8.0f, 0.5f);
    }
}
