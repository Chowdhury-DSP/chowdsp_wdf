#include <unordered_map>

#include <catch2/catch2.hpp>
#include <chowdsp_wdf/chowdsp_wdf.h>

/** Reference values generated from scipy.special */
std::unordered_map<double, double> WO_vals {
    { -10.0, 4.539786874921544e-05 },
    { -9.5, 7.484622772024869e-05 },
    { -9.0, 0.00012339457692560975 },
    { -8.5, 0.00020342698226408345 },
    { -8.0, 0.000335350149321062 },
    { -7.5, 0.0005527787213627528 },
    { -7.0, 0.0009110515723789146 },
    { -6.5, 0.0015011839473879653 },
    { -6.0, 0.002472630709097278 },
    { -5.5, 0.004070171383753891 },
    { -5.0, 0.0066930004977309955 },
    { -4.5, 0.010987603420879434 },
    { -4.0, 0.017989102828531025 },
    { -3.5, 0.029324711813756815 },
    { -3.0, 0.04747849102486547 },
    { -2.5, 0.07607221340790257 },
    { -2.0, 0.1200282389876412 },
    { -1.5, 0.1853749184489398 },
    { -1.0, 0.27846454276107374 },
    { -0.5, 0.4046738485459385 },
    { 0.0, 0.5671432904097838 },
    { 0.5, 0.7662486081617502 },
    { 1.0, 1.0 },
    { 1.5, 1.2649597201255005 },
    { 2.0, 1.5571455989976113 },
    { 2.5, 1.8726470404165942 },
    { 3.0, 2.207940031569323 },
    { 3.5, 2.559994780412122 },
    { 4.0, 2.926271062443501 },
    { 4.5, 3.3046649181693253 },
    { 5.0, 3.6934413589606496 },
    { 5.5, 4.091169202271799 },
    { 6.0, 4.4966641730061605 },
    { 6.5, 4.908941634486258 },
    { 7.0, 5.327178301371093 },
    { 7.5, 5.750681611147114 },
    { 8.0, 6.178865346308128 },
    { 8.5, 6.611230244734983 },
    { 9.0, 7.047348546597604 },
    { 9.5, 7.486851633496902 },
    { 10.0, 7.9294200950196965 },
};

template <typename T>
using FuncType = std::function<T (T)>;

template <typename T>
struct FunctionTest
{
    T low;
    T high;
    FuncType<T> testFunc;
    FuncType<T> refFunc;
    T tol;
};

template <typename T>
void checkFunctionAccuracy (const FunctionTest<T>& funcTest, size_t N = 20)
{
    auto step = (funcTest.high - funcTest.low) / (T) N;
    for (T x = funcTest.low; x < funcTest.high; x += step)
    {
        auto expected = funcTest.refFunc (x);
        auto actual = funcTest.testFunc (x);

        REQUIRE (actual == Approx (expected).margin (funcTest.tol));
    }
}

template <typename T, typename Func>
void checkWrightOmega (Func&& omega, T tol)
{
    for (auto vals : WO_vals)
    {
        auto expected = (T) vals.second;
        auto actual = omega ((T) vals.first);

        REQUIRE (actual == Approx (expected).margin (tol));
    }
}

TEST_CASE ("Omega Test")
{
    SECTION ("Log2 Test")
    {
        checkFunctionAccuracy (FunctionTest<float> {
            1.0f,
            2.0f,
            [] (auto x) { return chowdsp::Omega::log2_approx<float> (x); },
            [] (auto x) { return std::log2 (x); },
            0.008f });

        checkFunctionAccuracy (FunctionTest<double> {
            1.0,
            2.0,
            [] (auto x) { return chowdsp::Omega::log2_approx<double> (x); },
            [] (auto x) { return std::log2 (x); },
            0.008 });
    }

    SECTION ("Log Test")
    {
        checkFunctionAccuracy (FunctionTest<float> {
            8.0f,
            12.0f,
            [] (auto x) { return chowdsp::Omega::log_approx<float> (x); },
            [] (auto x) { return std::log (x); },
            0.005f });

        checkFunctionAccuracy (FunctionTest<double> {
            8.0,
            12.0,
            [] (auto x) { return chowdsp::Omega::log_approx<double> (x); },
            [] (auto x) { return std::log (x); },
            0.005 });
    }

    SECTION ("Pow2 Test")
    {
        checkFunctionAccuracy (FunctionTest<float> {
            0.0f,
            1.0f,
            [] (auto x) { return chowdsp::Omega::pow2_approx<float> (x); },
            [] (auto x) { return std::pow (2.0f, x); },
            0.001f });

        checkFunctionAccuracy (FunctionTest<double> {
            0.0,
            1.0,
            [] (auto x) { return chowdsp::Omega::pow2_approx<double> (x); },
            [] (auto x) { return std::pow (2.0, x); },
            0.001 });
    }

    SECTION ("Exp Test")
    {
        checkFunctionAccuracy (FunctionTest<float> {
            -4.0f,
            2.0f,
            [] (auto x) { return chowdsp::Omega::exp_approx<float> (x); },
            [] (auto x) { return std::exp (x); },
            0.005f });

        checkFunctionAccuracy (FunctionTest<double> {
            -4.0,
            2.0,
            [] (auto x) { return chowdsp::Omega::exp_approx<double> (x); },
            [] (auto x) { return std::exp (x); },
            0.005 });
    }

    SECTION ("Omega1 Test")
    {
        checkWrightOmega ([] (float x) { return chowdsp::Omega::omega1 (x); }, 2.1f);
        checkWrightOmega ([] (double x) { return chowdsp::Omega::omega1 (x); }, 2.1);
    }

    SECTION ("Omega2 Test")
    {
        checkWrightOmega ([] (float x) { return chowdsp::Omega::omega2 (x); }, 2.1f);
        checkWrightOmega ([] (double x) { return chowdsp::Omega::omega2 (x); }, 2.1);
    }

    SECTION ("Omega3 Test")
    {
        checkWrightOmega ([] (float x) { return chowdsp::Omega::omega3 (x); }, 0.3f);
        checkWrightOmega ([] (double x) { return chowdsp::Omega::omega3 (x); }, 0.3);
    }

    SECTION ("Omega4 Test")
    {
        checkWrightOmega ([] (float x) { return chowdsp::Omega::omega4 (x); }, 0.05f);
        checkWrightOmega ([] (double x) { return chowdsp::Omega::omega4 (x); }, 0.05);
    }
}
