#include <catch2/catch2.hpp>
#include <chowdsp_wdf/chowdsp_wdf.h>

using namespace chowdsp::wdft;

TEST_CASE ("Static Basic Circuits Test")
{
    SECTION ("Voltage Divider")
    {
        ResistorT<float> r1 (10000.0f);
        ResistorT<float> r2 (10000.0f);

        auto s1 = makeSeries<float> (r1, r2);
        auto p1 = makeInverter<float> (s1);
        IdealVoltageSourceT<float, decltype (p1)> vs { p1 };

        vs.setVoltage (10.0f);
        vs.incident (p1.reflected());
        p1.incident (vs.reflected());

        auto vOut = voltage<float> (r2);
        REQUIRE (vOut == 5.0f);
    }

    SECTION ("Current Divider Test")
    {
        ResistorT<float> r1 (10000.0f);
        ResistorT<float> r2 (10000.0f);

        auto p1 = makeParallel<float> (r1, r2);
        IdealCurrentSourceT<float, decltype (p1)> is (p1);

        is.setCurrent (1.0f);
        is.incident (p1.reflected());
        p1.incident (is.reflected());

        auto iOut = current<float> (r2);
        REQUIRE (iOut == 0.5f);
    }

    SECTION ("Shockley Diode")
    {
        constexpr auto saturationCurrent = 1.0e-7;
        constexpr auto thermalVoltage = 25.85e-3;
        constexpr auto voltage = -0.35;

        ResistiveVoltageSourceT<double> Vs;
        auto I1 = makeInverter<double> (Vs);
        DiodeT<double, decltype (I1)> D1 { I1, saturationCurrent, thermalVoltage };

        Vs.setVoltage (voltage);
        D1.incident (I1.reflected());
        I1.incident (D1.reflected());

        auto expectedCurrent = saturationCurrent * (std::exp (-voltage / thermalVoltage) - 1.0);
        REQUIRE (current<double> (D1) == Approx (expectedCurrent).margin (1.0e-3));
    }

    SECTION ("Current Switch")
    {
        ResistorT<float> r1 (10000.0f);
        ResistiveCurrentSourceT<float> Is;

        auto s1 = makeSeries<float> (r1, Is);
        SwitchT<float, decltype (s1)> sw { s1 };

        // run with switch closed
        sw.setClosed (true);
        Is.setCurrent (1.0f);
        sw.incident (s1.reflected());
        s1.incident (sw.reflected());

        auto currentClosed = current<float> (r1);
        REQUIRE (currentClosed == Approx (-1.0f).margin (1.0e-3f));

        // run with switch open
        sw.setClosed (false);
        sw.incident (s1.reflected());
        s1.incident (sw.reflected());

        auto currentOpen = current<float> (r1);
        REQUIRE (currentOpen == 0.0f);
    }

    SECTION ("Y-Parameter Test")
    {
        constexpr auto y11 = 0.11;
        constexpr auto y12 = 0.22;
        constexpr auto y21 = 0.33;
        constexpr auto y22 = 0.44;
        constexpr auto V = 2.0;

        ResistorT<double> res { 10000.0 };
        YParameterT<double, decltype (res)> yParam { res, y11, y12, y21, y22 };
        IdealVoltageSourceT<double, decltype (yParam)> Vs { yParam };

        Vs.setVoltage (V);
        Vs.incident (yParam.reflected());
        yParam.incident (Vs.reflected());

        REQUIRE (-current<double> (res) == Approx (y11 * voltage<double> (res) + y12 * V).margin (1.0e-3));
        REQUIRE (current<double> (yParam) == Approx (y21 * voltage<double> (res) + y22 * V).margin (1.0e-3));
    }

    SECTION ("RC Lowpass")
    {
        constexpr double fs = 44100.0;
        constexpr double fc = 500.0;

        constexpr auto capValue = 1.0e-6;
        constexpr auto resValue = 1.0 / ((2 * M_PI) * fc * capValue);

        CapacitorT<double> c1 (capValue, fs);
        ResistorT<double> r1 (resValue);

        auto s1 = makeSeries<double> (r1, c1);
        auto p1 = makeInverter<double> (s1);
        IdealVoltageSourceT<double, decltype (p1)> vs { p1 };

        auto testFreq = [&] (double freq, double expectedMagDB) {
            c1.reset();

            double magnitude = 0.0;
            for (int n = 0; n < (int) fs; ++n)
            {
                const auto x = std::sin (2.0 * M_PI * freq * (double) n / fs);
                vs.setVoltage (x);

                vs.incident (p1.reflected());
                p1.incident (vs.reflected());

                const auto y = voltage<double> (c1);

                if (n > 1000)
                    magnitude = std::max (magnitude, std::abs (y));
            }

            const auto actualMagnitudeDB = 20.0 * std::log10 (magnitude);
            REQUIRE (actualMagnitudeDB == Approx (expectedMagDB).margin (0.1));
        };

        testFreq (2 * fc, -7.0);
        testFreq (fc, -3.0);
        testFreq (0.5 * fc, -1.0);
    }

    SECTION ("Alpha Transform")
    {
        constexpr float fs = 44100.0f;

        // 1 kHz cutoff 2nd-order highpass
        constexpr float R = 300.0f;
        constexpr float C = 1.0e-6f;
        constexpr float L = 0.022f;

        auto testFreq = [&] (float freq, float expectedMagDB, auto& vs, auto& p1, auto& l1) {
            float magnitude = 0.0f;
            for (int n = 0; n < (int) fs; ++n)
            {
                const auto x = std::sin (2.0f * (float) M_PI * freq * (float) n / fs);
                vs.setVoltage (x);

                vs.incident (p1.reflected());
                p1.incident (vs.reflected());

                const auto y = voltage<float> (l1);

                if (n > 1000)
                    magnitude = std::max (magnitude, std::abs (y));
            }

            const auto actualMagnitudeDB = 20.0f * std::log10 (magnitude);
            REQUIRE (actualMagnitudeDB == Approx (expectedMagDB).margin (0.1f));
        };

        // reference filter
        {
            CapacitorT<float> c1 (C);
            ResistorT<float> r1 (R);
            InductorT<float> l1 (L);

            auto s1 = makeSeries<float> (r1, c1);
            auto s2 = makeSeries<float> (s1, l1);
            auto p1 = makeInverter<float> (s2);
            IdealVoltageSourceT<float, decltype (p1)> vs { p1 };

            c1.prepare (fs);
            l1.prepare (fs);

            testFreq (10.0e3f, 0.0f, vs, p1, l1);
        }

        CapacitorAlphaT<float> c1 (C);
        ResistorT<float> r1 (R);
        InductorAlphaT<float> l1 (L);

        auto s1 = makeSeries<float> (r1, c1);
        auto s2 = makeSeries<float> (s1, l1);
        auto p1 = makeInverter<float> (s2);
        IdealVoltageSourceT<float, decltype (p1)> vs { p1 };

        // alpha = 1.0 filter
        {
            constexpr float alpha = 1.0f;
            c1.prepare (fs);
            c1.setAlpha (alpha);
            l1.prepare (fs);
            l1.setAlpha (alpha);

            testFreq (10.0e3f, 0.0f, vs, p1, l1);
        }

        // alpha = 0.1 filter
        {
            constexpr float alpha = 0.1f;
            c1.reset();
            c1.setAlpha (alpha);
            l1.reset();
            l1.setAlpha (alpha);

            testFreq (10.0e3f, -1.1f, vs, p1, l1);
        }
    }
}

template <typename WDFType>
struct ImpedanceChecker
{
    float value1 = 0.0f;
    float value2 = 0.0f;
    std::function<void (WDFType&, float)> changeFunc;
    std::function<float (float)> impedanceCalc;
    std::function<WDFType()> factory;
};

template <typename WDFType>
void checkImpedanceChange (const ImpedanceChecker<WDFType>& checker)
{
    auto&& component = checker.factory();
    REQUIRE (component.wdf.R == checker.impedanceCalc (checker.value1));

    checker.changeFunc (component, checker.value2);
    REQUIRE (component.wdf.R == checker.impedanceCalc (checker.value2));
}

template <typename WDFType>
void checkImpedanceProp (const ImpedanceChecker<WDFType>& checker)
{
    constexpr float otherR = 5000.0f;
    ResistorT<float> r2 { otherR };
    auto&& component = checker.factory();
    auto s1 = makeSeries<float> (component, r2);
    IdealCurrentSourceT<float, decltype (s1)> is (s1);
    is.setCurrent (1.0f);

    REQUIRE (s1.wdf.R == checker.impedanceCalc (checker.value1) + otherR);
    REQUIRE (is.reflected() == 2.0f * s1.wdf.R);

    checker.changeFunc (component, checker.value2);
    REQUIRE (s1.wdf.R == checker.impedanceCalc (checker.value2) + otherR);
    REQUIRE (is.reflected() == 2.0f * s1.wdf.R);
}

template <typename WDFType>
void doImpedanceChecks (const ImpedanceChecker<WDFType>& checker)
{
    checkImpedanceChange (checker);
    checkImpedanceProp (checker);
}

TEST_CASE ("Static Impedance Change Test")
{
    SECTION ("Impedance Change")
    {
        constexpr float fs = 44100.0f;

        // resistor
        doImpedanceChecks (ImpedanceChecker<ResistorT<float>> {
            1000.0f,
            2000.0f,
            [] (auto& r, float value) { r.setResistanceValue (value); },
            [] (float value) { return value; },
            []() { return ResistorT<float> { 1000.0f }; },
        });

        // capacitor
        doImpedanceChecks (ImpedanceChecker<CapacitorT<float>> {
            1.0e-6f,
            2.0e-6f,
            [] (auto& c, float value) { c.setCapacitanceValue (value); },
            [_fs = fs] (float value) { return 1.0f / (2.0f * value * (float) _fs); },
            [_fs = fs]() { return CapacitorT<float> { 1.0e-6f, _fs }; },
        });

        // capacitor alpha
        doImpedanceChecks (ImpedanceChecker<CapacitorAlphaT<float>> {
            1.0e-6f,
            2.0e-6f,
            [] (auto& c, float value) { c.setCapacitanceValue (value); },
            [_fs = fs] (float value) { return 1.0f / (1.5f * value * (float) _fs); },
            [_fs = fs]() { return CapacitorAlphaT<float> { 1.0e-6f, _fs, 0.5f }; },
        });

        // inductor
        doImpedanceChecks (ImpedanceChecker<InductorT<float>> {
            1.0f,
            2.0f,
            [] (auto& i, float value) { i.setInductanceValue (value); },
            [_fs = fs] (float value) { return 2.0f * value * (float) _fs; },
            [_fs = fs]() { return InductorT<float> { 1.0f, _fs }; },
        });

        // inductor alpha
        doImpedanceChecks (ImpedanceChecker<InductorAlphaT<float>> {
            1.0f,
            2.0f,
            [] (auto& i, float value) { i.setInductanceValue (value); },
            [_fs = fs] (float value) { return 1.5f * value * (float) _fs; },
            [_fs = fs]() { return InductorAlphaT<float> { 1.0f, _fs, 0.5f }; },
        });

        // resistive voltage source
        doImpedanceChecks (ImpedanceChecker<ResistiveVoltageSourceT<float>> {
            1000.0f,
            2000.0f,
            [] (auto& r, float value) { r.setResistanceValue (value); },
            [] (float value) { return value; },
            []() { return ResistiveVoltageSourceT<float> { 1000.0f }; },
        });

        // resistive current source
        doImpedanceChecks (ImpedanceChecker<ResistiveCurrentSourceT<float>> {
            1000.0f,
            2000.0f,
            [] (auto& r, float value) { r.setResistanceValue (value); },
            [] (float value) { return value; },
            []() { return ResistiveCurrentSourceT<float> { 1000.0f }; },
        });
    }
}
