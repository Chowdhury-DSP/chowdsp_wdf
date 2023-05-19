#include <catch2/catch2.hpp>
#include <chowdsp_wdf/chowdsp_wdf.h>

using namespace chowdsp::wdft;

TEST_CASE ("Combined Component Test")
{
    SECTION ("Resistor/Capacitor Series")
    {
        static constexpr auto r_val = 2000.0f;
        static constexpr auto c_val = 2.0e-6f;

        ResistorT<float> r1 { r_val };
        CapacitorT<float> c1 { c_val };
        WDFSeriesT<float, decltype (r1), decltype (c1)> s1 { r1, c1 };

        ResistorCapacitorSeriesT<float> rc1 { r_val, c_val };

        float inputs[] = { 0.0f, 1.0f, -1.0f, 2.0f, -3.0f };
        for (auto& a : inputs)
        {
            s1.incident (a);
            const auto ref = s1.reflected();

            rc1.incident (a);
            const auto actual = rc1.reflected();

            REQUIRE (ref == Approx { actual }.margin (1.0e-4f));
        }
    }

    SECTION ("Resistor/Capacitor Parallel")
    {
        static constexpr auto r_val = 2000.0f;
        static constexpr auto c_val = 2.0e-6f;

        ResistorT<float> r1 { r_val };
        CapacitorT<float> c1 { c_val };
        WDFParallelT<float, decltype (r1), decltype (c1)> p1 { r1, c1 };

        ResistorCapacitorParallelT<float> rc1 { r_val, c_val };

        float inputs[] = { 0.0f, 1.0f, -1.0f, 2.0f, -3.0f };
        for (auto& a : inputs)
        {
            p1.incident (a);
            const auto ref = p1.reflected();

            rc1.incident (a);
            const auto actual = rc1.reflected();

            REQUIRE (ref == Approx { actual }.margin (1.0e-4f));
        }
    }

    SECTION ("Resistor/Capacitor/Voltage Source Series")
    {
        static constexpr auto r_val = 2000.0f;
        static constexpr auto c_val = 2.0e-6f;
        static constexpr auto source_v = 1.5f;

        ResistiveVoltageSourceT<float> rv1 { r_val };
        rv1.setVoltage (source_v);
        CapacitorT<float> c1 { c_val };
        WDFSeriesT<float, decltype (rv1), decltype (c1)> s1 { rv1, c1 };

        ResistiveCapacitiveVoltageSourceT<float> rc1 { r_val, c_val };
        rc1.setVoltage (source_v);
        rc1.reset();

        float inputs[] = { 0.0f, 1.0f, -1.0f, 2.0f, -3.0f };
        for (auto& a : inputs)
        {
            s1.incident (a);
            const auto ref = s1.reflected();

            rc1.incident (a);
            const auto actual = rc1.reflected();

            REQUIRE (ref == Approx { actual }.margin (1.0e-4f));
        }
    }
}
