#pragma once

// define NODISCARD if we can
#if __cplusplus >= 201703L
#define CHOWDSP_WDF_MAYBE_UNUSED [[maybe_unused]]
#else
#define CHOWDSP_WDF_MAYBE_UNUSED
#endif

// Define a default SIMD alignment
#if defined(XSIMD_HPP)
constexpr int WDF_DEFAULT_SIMD_ALIGNMENT = xsimd::default_arch::alignment();
#else
constexpr int WDF_DEFAULT_SIMD_ALIGNMENT = 16;
#endif

#include "wdft/wdft.h"
#include "wdf/wdf.h"
#include "rtype/rtype.h"
