#include <catch2/catch2.hpp>
#include <chowdsp_wdf/chowdsp_wdf.h>

TEST_CASE("Basic Circuits Test")
{
    SECTION("Voltage Divider")
    {
        using namespace chowdsp::WDF;
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

    SECTION("Current Divider Test")
    {
        using namespace chowdsp::WDF;
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
}
