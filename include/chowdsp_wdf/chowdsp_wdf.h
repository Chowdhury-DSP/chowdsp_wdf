#pragma once

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244 4464 4514 4324)
#endif

// define maybe_unused if we can
#if __cplusplus >= 201703L
#define CHOWDSP_WDF_MAYBE_UNUSED [[maybe_unused]]
#else
#define CHOWDSP_WDF_MAYBE_UNUSED
#endif

// Define a default SIMD alignment
#if defined(XSIMD_HPP)
constexpr auto CHOWDSP_WDF_DEFAULT_SIMD_ALIGNMENT = (int) xsimd::default_arch::alignment();
#else
constexpr int CHOWDSP_WDF_DEFAULT_SIMD_ALIGNMENT = 16;
#endif

#include "wdft/wdft.h"
#include "wdf/wdf.h"
#include "rtype/rtype.h"

#include "util/defer_impedance.h"

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
