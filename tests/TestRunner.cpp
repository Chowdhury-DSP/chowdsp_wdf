#if CHOWDSP_WDF_TEST_WITH_XSIMD
#include <xsimd/xsimd.hpp>
#endif

#define CATCH_CONFIG_RUNNER
#include "catch2/catch2.hpp"

int main(int argc, char **argv)
{
    int result = Catch::Session().run(argc, argv);
    return result;
}

