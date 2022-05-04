#if CHOWDSP_WDF_TEST_WITH_XSIMD

#include <catch2/catch2.hpp>
#include <xsimd/xsimd.hpp>
#include "BassmanToneStack.h"

TEST_CASE ("SIMD Circuits Test")
{
    SECTION ("SIMD Signum Test")
    {
        using chowdsp::signum::signum;

        float testReg alignas (CHOWDSP_WDF_DEFAULT_SIMD_ALIGNMENT)[xsimd::batch<float>::size] { -1.0f, 0.0f, 0.5f, 1.0f };
        float signumReg alignas (CHOWDSP_WDF_DEFAULT_SIMD_ALIGNMENT)[xsimd::batch<float>::size] {};
        xsimd::store_aligned (signumReg, signum (xsimd::load_aligned (testReg)));

        for (size_t i = 0; i < xsimd::batch<float>::size; ++i)
            REQUIRE (signumReg[i] == (float) signum (testReg[i]));
    }

    SECTION ("Voltage Divider")
    {
        using v_type = xsimd::batch<float>;
        using namespace chowdsp::wdf;

        Resistor<v_type> r1 (10000.0f);
        Resistor<v_type> r2 (10000.0f);

        WDFSeries<v_type> s1 (&r1, &r2);
        PolarityInverter<v_type> p1 (&s1);
        IdealVoltageSource<v_type> vs { &p1 };

        vs.setVoltage (10.0f);
        vs.incident (p1.reflected());
        p1.incident (vs.reflected());

        REQUIRE (xsimd::all (r2.voltage() == v_type (5.0f)));
    }

    SECTION ("Current Divider Test")
    {
        using v_type = xsimd::batch<float>;
        using namespace chowdsp::wdf;

        Resistor<v_type> r1 (10000.0f);
        Resistor<v_type> r2 (10000.0f);

        WDFParallel<v_type> p1 (&r1, &r2);
        IdealCurrentSource<v_type> is (&p1);

        is.setCurrent (1.0f);
        is.incident (p1.reflected());
        p1.incident (is.reflected());

        REQUIRE (xsimd::all (r2.current() == v_type (0.5f)));
    }

    SECTION ("Shockley Diode Test")
    {
        using v_type = xsimd::batch<double>;

        static const auto saturationCurrent = (v_type) 1.0e-7;
        static const auto thermalVoltage = (v_type) 25.85e-3;
        static const auto voltage = (v_type) -0.35;

        using namespace chowdsp::wdf;
        ResistiveVoltageSource<v_type> Vs;
        PolarityInverter<v_type> I1 { &Vs };
        Diode<v_type> D1 { &I1, saturationCurrent, thermalVoltage };

        Vs.setVoltage (voltage);
        D1.incident (I1.reflected());
        I1.incident (D1.reflected());

        auto expectedCurrent = saturationCurrent * (xsimd::exp (-voltage / thermalVoltage) - (v_type) 1.0);
        REQUIRE (xsimd::all ((D1.current() < expectedCurrent + 1.0e-3 && D1.current() > expectedCurrent - 1.0e-3)));
    }

    SECTION ("SIMD DIode Clipper Test")
    {
        using namespace chowdsp::wdf;
        constexpr double fs = 44100.0;

        using FloatType = double;
        using VType = xsimd::batch<FloatType>;
        constexpr auto Cap = (FloatType) 47.0e-9;
        constexpr auto Res = (FloatType) 4700.0;

        constexpr int num = 5;
        FloatType data1[num] = { 1.0, 0.5, 0.0, -0.5, -1.0 };
        VType data2[num] = { 1.0, 0.5, 0.0, -0.5, -1.0 };

        // Normal
        {
            ResistiveVoltageSource<FloatType> Vs {};
            Resistor<FloatType> R1 { Res };
            auto C1 = std::make_unique<Capacitor<FloatType>> (Cap, (FloatType) fs);

            auto S1 = std::make_unique<WDFSeries<FloatType>> (&Vs, &R1);
            auto P1 = std::make_unique<WDFParallel<FloatType>> (S1.get(), C1.get());
            auto I1 = std::make_unique<PolarityInverter<FloatType>> (P1.get());

            DiodePair<FloatType> dp { I1.get(), (FloatType) 2.52e-9 };
            dp.setDiodeParameters ((FloatType) 2.52e-9, (FloatType) 0.02585, 1);

            for (int i = 0; i < num; ++i)
            {
                Vs.setVoltage (data1[i]);
                dp.incident (P1->reflected());
                data1[i] = C1->voltage();
                P1->incident (dp.reflected());
            }
        }

        // SIMD
        {
            ResistiveVoltageSource<VType> Vs {};
            Resistor<VType> R1 { Res };
            auto C1 = std::make_unique<Capacitor<VType>> (Cap, (VType) fs);

            auto S1 = std::make_unique<WDFSeries<VType>> (&Vs, &R1);
            auto P1 = std::make_unique<WDFParallel<VType>> (S1.get(), C1.get());
            auto I1 = std::make_unique<PolarityInverter<VType>> (P1.get());

            DiodePair<VType> dp { I1.get(), (VType) 2.52e-9 };

            for (int i = 0; i < num; ++i)
            {
                Vs.setVoltage (data2[i]);
                dp.incident (P1->reflected());
                data2[i] = C1->voltage();
                P1->incident (dp.reflected());
            }
        }

        for (int i = 0; i < num; ++i)
            REQUIRE (xsimd::all ((data2[i] < data1[i] + 1.0e-6 && data2[i] > data1[i] - 1.0e-6)));
    }

    SECTION ("Static SIMD WDF Test")
    {
        using FloatType = float;
        using Vec = xsimd::batch<FloatType>;
        constexpr double fs = 44100.0;

        constexpr int num = 5;
        Vec data1[num] = { 1.0, 0.5, 0.0, -0.5, -1.0 };
        Vec data2[num] = { 1.0, 0.5, 0.0, -0.5, -1.0 };

        // dynamic
        {
            using namespace chowdsp::wdf;
            using Resistor = Resistor<Vec>;
            using Capacitor = CapacitorAlpha<Vec>;
            using ResVs = ResistiveVoltageSource<Vec>;

            ResVs Vs { 1.0e-9f };
            Resistor r162 { 4700.0f };
            Resistor r163 { 100000.0f };

            auto c40 = std::make_unique<Capacitor> ((FloatType) 0.015e-6f, (FloatType) fs, (FloatType) 0.029f);
            auto P1 = std::make_unique<WDFParallel<Vec>> (c40.get(), &r163);
            auto S1 = std::make_unique<WDFSeries<Vec>> (&Vs, P1.get());
            auto I1 = std::make_unique<PolarityInverter<Vec>> (&r162);
            auto P2 = std::make_unique<WDFParallel<Vec>> (I1.get(), S1.get());

            Diode<Vec> d53 { P2.get(), 2.52e-9f }; // 1N4148 diode
            d53.setDiodeParameters ((FloatType) 2.52e-9, (FloatType) 0.02585, 1);

            for (int i = 0; i < num; ++i)
            {
                Vs.setVoltage (data1[i]);
                d53.incident (P2->reflected());
                data1[i] = r162.voltage();
                P2->incident (d53.reflected());
            }
        }

        // static
        {
            using namespace chowdsp::wdft;
            using Resistor = ResistorT<Vec>;
            using Capacitor = CapacitorAlphaT<Vec>;
            using ResVs = ResistiveVoltageSourceT<Vec>;

            ResVs Vs { 1.0e-9f };
            Resistor r162 { 4700.0f };
            Resistor r163 { 100000.0f };
            Capacitor c40 { (FloatType) 0.015e-6f, (FloatType) fs, (FloatType) 0.029f };

            auto P1 = makeParallel<Vec> (c40, r163);
            auto S1 = makeSeries<Vec> (Vs, P1);
            auto I1 = makeInverter<Vec> (r162);
            auto P2 = makeParallel<Vec> (I1, S1);
            DiodeT<Vec, decltype (P2)> d53 { P2, 2.52e-9f }; // 1N4148 diode

            for (int i = 0; i < num; ++i)
            {
                Vs.setVoltage (data2[i]);
                d53.incident (P2.reflected());
                data2[i] = voltage<Vec> (r162);
                P2.incident (d53.reflected());
            }
        }

        for (int i = 0; i < num; ++i)
            REQUIRE (xsimd::all ((data2[i] < data1[i] + 1.0e-6f && data2[i] > data1[i] - 1.0e-6f)));
    }

    SECTION ("SIMD R-Type Test")
    {
        constexpr double fs = 44100.0;

        using FloatType = double;
        using VType = xsimd::batch<FloatType>;

        constexpr int num = 5;
        FloatType data1[num] = { 1.0, 0.5, 0.0, -0.5, -1.0 };
        VType data2[num] = { 1.0, 0.5, 0.0, -0.5, -1.0 };

        constexpr double highPot = 0.75;
        constexpr double lowPot = 0.25;

        // Normal
        {
            Tonestack<FloatType> tonestack;
            tonestack.prepare (fs);
            tonestack.setParams ((FloatType) highPot, (FloatType) lowPot, 1.0);

            for (int i = 0; i < num; ++i)
                data1[i] = tonestack.processSample (data1[i]);
        }

        // SIMD
        {
            Tonestack<VType> tonestack;
            tonestack.prepare (fs);
            tonestack.setParams ((VType) highPot, (VType) lowPot, (VType) 1.0);

            for (int i = 0; i < num; ++i)
                data2[i] = tonestack.processSample (data2[i]);
        }

        for (int i = 0; i < num; ++i)
            REQUIRE (xsimd::all ((data2[i] < data1[i] + 1.0e-6 && data2[i] > data1[i] - 1.0e-6)));
    }
}

#endif // CHOWDSP_WDF_TEST_WITH_XSIMD
