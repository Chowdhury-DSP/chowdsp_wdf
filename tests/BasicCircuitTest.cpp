#include <catch2/catch2.hpp>
#include <chowdsp_wdf/chowdsp_wdf.h>

using namespace chowdsp::wdf;

TEST_CASE ("Basic Circuits Test")
{
    SECTION ("Voltage Divider")
    {
        Resistor<float> r1 (10000.0f);
        Resistor<float> r2 (10000.0f);

        WDFSeries<float> s1 (&r1, &r2);
        PolarityInverter<float> p1 (&s1);
        IdealVoltageSource<float> vs { &p1 };

        vs.setVoltage (10.0f);
        vs.incident (p1.reflected());
        p1.incident (vs.reflected());

        auto vOut = r2.voltage();
        REQUIRE (vOut == 5.0f);
    }

    SECTION ("Current Divider Test")
    {
        Resistor<float> r1 (10000.0f);
        Resistor<float> r2 (10000.0f);

        WDFParallel<float> p1 (&r1, &r2);
        IdealCurrentSource<float> is (&p1);

        is.setCurrent (1.0f);
        is.incident (p1.reflected());
        p1.incident (is.reflected());

        auto iOut = r2.current();
        REQUIRE (iOut == 0.5f);
    }

    SECTION ("Shockley Diode")
    {
        static constexpr auto saturationCurrent = 1.0e-7;
        static constexpr auto thermalVoltage = 25.85e-3;
        static constexpr auto voltage = -0.35;

        ResistiveVoltageSource<double> Vs;
        PolarityInverter<double> I1 { &Vs };
        Diode<double> D1 { &I1, saturationCurrent, thermalVoltage };

        Vs.setVoltage (voltage);
        D1.incident (I1.reflected());
        I1.incident (D1.reflected());

        auto expectedCurrent = saturationCurrent * (std::exp (-voltage / thermalVoltage) - 1.0);
        REQUIRE (D1.current() == Approx (expectedCurrent).margin (1.0e-3));
    }

    SECTION ("Current Switch")
    {
        Resistor<float> r1 (10000.0f);
        ResistiveCurrentSource<float> Is;

        WDFSeries<float> s1 (&r1, &Is);
        Switch<float> sw { &s1 };

        // run with switch closed
        sw.setClosed (true);
        Is.setCurrent (1.0f);
        sw.incident (s1.reflected());
        s1.incident (sw.reflected());

        auto currentClosed = r1.current();
        REQUIRE (currentClosed == Approx (-1.0f).margin (1.0e-3f));

        // run with switch open
        sw.setClosed (false);
        sw.incident (s1.reflected());
        s1.incident (sw.reflected());

        auto currentOpen = r1.current();
        REQUIRE (currentOpen == 0.0f);
    }

    SECTION ("Y-Parameter Test")
    {
        static constexpr auto y11 = 0.11;
        static constexpr auto y12 = 0.22;
        static constexpr auto y21 = 0.33;
        static constexpr auto y22 = 0.44;
        static constexpr auto voltage = 2.0;

        Resistor<double> res { 10000.0 };
        YParameter<double> yParam { &res, y11, y12, y21, y22 };
        IdealVoltageSource<double> Vs { &yParam };

        Vs.setVoltage (voltage);
        Vs.incident (yParam.reflected());
        yParam.incident (Vs.reflected());

        REQUIRE (-res.current() == Approx (y11 * res.voltage() + y12 * voltage).margin (1.0e-3));
        REQUIRE (yParam.current() == Approx (y21 * res.voltage() + y22 * voltage).margin (1.0e-3));
    }

    SECTION ("RC Lowpass")
    {
        static constexpr double fs = 44100.0;
        static constexpr double fc = 500.0;

        static constexpr auto capValue = 1.0e-6;
        static constexpr auto resValue = 1.0 / ((2 * M_PI) * fc * capValue);

        Capacitor<double> c1 (capValue, fs);
        Resistor<double> r1 (resValue);

        WDFSeries<double> s1 (&r1, &c1);
        PolarityInverter<double> p1 (&s1);
        IdealVoltageSource<double> vs { &p1 };

        auto testFreq = [&] (double freq, double expectedMagDB) {
            c1.reset();

            double magnitude = 0.0;
            for (int n = 0; n < (int) fs; ++n)
            {
                const auto x = std::sin (2.0 * M_PI * freq * (double) n / fs);
                vs.setVoltage (x);

                vs.incident (p1.reflected());
                p1.incident (vs.reflected());

                const auto y = c1.voltage();

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
        static constexpr float fs = 44100.0f;

        // 1 kHz cutoff 2nd-order highpass
        static constexpr float R = 300.0f;
        static constexpr float C = 1.0e-6f;
        static constexpr float L = 0.022f;

        auto testFreq = [&] (float freq, float expectedMagDB, auto& vs, auto& p1, auto& l1) {
            float magnitude = 0.0f;
            for (int n = 0; n < (int) fs; ++n)
            {
                const auto x = std::sin (2.0f * (float) M_PI * freq * (float) n / fs);
                vs.setVoltage (x);

                vs.incident (p1.reflected());
                p1.incident (vs.reflected());

                const auto y = l1.voltage();

                if (n > 1000)
                    magnitude = std::max (magnitude, std::abs (y));
            }

            const auto actualMagnitudeDB = 20.0f * std::log10 (magnitude);
            REQUIRE (actualMagnitudeDB == Approx (expectedMagDB).margin (0.1f));
        };

        // reference filter
        {
            Capacitor<float> c1 (C);
            Resistor<float> r1 (R);
            Inductor<float> l1 (L);

            WDFSeries<float> s1 (&r1, &c1);
            WDFSeries<float> s2 (&s1, &l1);
            PolarityInverter<float> p1 (&s2);
            IdealVoltageSource<float> vs { &p1 };

            c1.prepare (fs);
            l1.prepare (fs);

            testFreq (10.0e3f, 0.0f, vs, p1, l1);
        }

        CapacitorAlpha<float> c1 (C);
        Resistor<float> r1 (R);
        InductorAlpha<float> l1 (L);

        WDFSeries<float> s1 (&r1, &c1);
        WDFSeries<float> s2 (&s1, &l1);
        PolarityInverter<float> p1 (&s2);
        IdealVoltageSource<float> vs { &p1 };

        // alpha = 1.0 filter
        {
            static constexpr float alpha = 1.0f;
            c1.prepare (fs);
            c1.setAlpha (alpha);
            l1.prepare (fs);
            l1.setAlpha (alpha);

            testFreq (10.0e3f, 0.0f, vs, p1, l1);
        }

        // alpha = 0.1 filter
        {
            static constexpr float alpha = 0.1f;
            c1.reset();
            c1.setAlpha (alpha);
            l1.reset();
            l1.setAlpha (alpha);

            testFreq (10.0e3f, -1.1f, vs, p1, l1);
        }
    }

    SECTION ("Impedance Change")
    {
        static constexpr float fs = 44100.0f;

        auto checkImpedanceChange = [=] (auto component, float value1, float value2, auto changeFunc, auto impedanceCalc) {
            REQUIRE (component.wdf.R == impedanceCalc (value1));

            changeFunc (component, value2);
            REQUIRE (component.wdf.R == impedanceCalc (value2));
        };

        auto checkImpedanceProp = [=] (auto component, float value1, float value2, auto changeFunc, auto impedanceCalc) {
            static constexpr float otherR = 5000.0f;
            Resistor<float> r2 { otherR };
            WDFSeries<float> s1 (&component, &r2);
            IdealCurrentSource<float> is (&s1);
            is.setCurrent (1.0f);

            REQUIRE (s1.wdf.R == impedanceCalc (value1) + otherR);
            REQUIRE (is.reflected() == 2.0f * s1.wdf.R);

            changeFunc (component, value2);
            REQUIRE (s1.wdf.R == impedanceCalc (value2) + otherR);
            REQUIRE (is.reflected() == 2.0f * s1.wdf.R);
        };

        auto doImpedanceChecks = [=] (auto... params) {
            checkImpedanceChange (params...);
            checkImpedanceProp (params...);
        };

        // resistor
        doImpedanceChecks (
            Resistor<float> { 1000.0f }, 1000.0f, 2000.0f, [=] (auto& r, float value) { r.setResistanceValue (value); }, [=] (float value) { return value; });

        // capacitor
        doImpedanceChecks (
            Capacitor<float> { 1.0e-6f, fs }, 1.0e-6f, 2.0e-6f, [=] (auto& c, float value) { c.setCapacitanceValue (value); }, [=] (float value) { return 1.0f / (2.0f * value * (float) fs); });

        // capacitor alpha
        doImpedanceChecks (
            CapacitorAlpha<float> { 1.0e-6f, fs, 0.5f }, 1.0e-6f, 2.0e-6f, [=] (auto& c, float value) { c.setCapacitanceValue (value); }, [=] (float value) { return 1.0f / (1.5f * value * (float) fs); });

        // inductor
        doImpedanceChecks (
            Inductor<float> { 1.0f, fs }, 1.0f, 2.0f, [=] (auto& i, float value) { i.setInductanceValue (value); }, [=] (float value) { return 2.0f * value * (float) fs; });

        // inductor alpha
        doImpedanceChecks (
            InductorAlpha<float> { 1.0f, fs, 0.5f }, 1.0f, 2.0f, [=] (auto& i, float value) { i.setInductanceValue (value); }, [=] (float value) { return 1.5f * value * (float) fs; });

        // resistive voltage source
        doImpedanceChecks (
            ResistiveVoltageSource<float> { 1000.0f }, 1000.0f, 2000.0f, [=] (auto& r, float value) { r.setResistanceValue (value); }, [=] (float value) { return value; });

        // resistive current source
        doImpedanceChecks (
            ResistiveCurrentSource<float> { 1000.0f }, 1000.0f, 2000.0f, [=] (auto& r, float value) { r.setResistanceValue (value); }, [=] (float value) { return value; });
    }
}
