#include <benchmark/benchmark.h>

#if CHOWDSP_WDF_TEST_WITH_XSIMD
#include <xsimd/xsimd.hpp>
#endif
#include <chowdsp_wdf/chowdsp_wdf.h>

#include <random>
#include <vector>

template <typename T>
inline auto makeRandomVector (int num)
{
    std::random_device rnd_device;
    std::mt19937 mersenne_engine { rnd_device() }; // Generates random integers
    std::normal_distribution<float> dist { (T) -10, (T) 10 };

    std::vector<T> vec ((size_t) num);
    std::generate (vec.begin(), vec.end(), [&dist, &mersenne_engine]() { return dist (mersenne_engine); });

    return std::move (vec);
}

#define SCALAR_BENCH(name, testVec, func) \
  static void name (benchmark::State& state) \
  { \
      for (auto _ : state) \
      { \
          for (int i = 0; i < N; ++i) \
              (testVec)[i] = func ((testVec) [i]); \
      } \
  }                                       \
  BENCHMARK(name)->MinTime (3);

#define SIMD_BENCH(name, testVec, func, SIMDType) \
  static void name (benchmark::State& state) \
  { \
      SIMDType y;                                            \
      for (auto _ : state) \
      { \
          for (int i = 0; i < N; ++i) \
              y = func ((SIMDType) (testVec) [i]); \
      } \
      (testVec)[0] = y.get (0);\
  } \
  BENCHMARK(name)->MinTime (3);

constexpr int N = 1000;
static auto testFloatVec = makeRandomVector<float> (N);
static auto testDoubleVec = makeRandomVector<double> (N);

SCALAR_BENCH (floatWrightOmega3, testFloatVec, chowdsp::Omega::omega3)
SCALAR_BENCH (floatWrightOmega4, testFloatVec, chowdsp::Omega::omega4)
SCALAR_BENCH (doubleWrightOmega3, testDoubleVec, chowdsp::Omega::omega3)
SCALAR_BENCH (doubleWrightOmega4, testDoubleVec, chowdsp::Omega::omega4)

#if CHOWDSP_WDF_TEST_WITH_XSIMD
SIMD_BENCH (floatSIMDWrightOmega3, testFloatVec, chowdsp::Omega::omega3, xsimd::batch<float>)
SIMD_BENCH (floatSIMDWrightOmega4, testFloatVec, chowdsp::Omega::omega4, xsimd::batch<float>)
SIMD_BENCH (doubleSIMDWrightOmega3, testDoubleVec, chowdsp::Omega::omega3, xsimd::batch<double>)
SIMD_BENCH (doubleSIMDWrightOmega4, testDoubleVec, chowdsp::Omega::omega4, xsimd::batch<double>)
#endif

BENCHMARK_MAIN();
