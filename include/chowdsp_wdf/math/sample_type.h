#ifndef CHOWDSP_WDF_SAMPLE_TYPE_H
#define CHOWDSP_WDF_SAMPLE_TYPE_H

#include <type_traits>

#ifndef DOXYGEN

namespace chowdsp
{
/** Useful structs for determining the internal data type of SIMD types */
namespace SampleTypeHelpers
{
    template <typename T, bool = std::is_floating_point<T>::value>
    struct ElementType
    {
        using Type = T;
    };

    template <typename T>
    struct ElementType<T, false>
    {
        using Type = typename T::value_type;
    };
} // namespace SampleTypeHelpers

/** Type alias for a SIMD numeric type */
template <typename T>
using NumericType = typename SampleTypeHelpers::ElementType<T>::Type;

/** Returns true if all the elements in a SIMD vector are equal */
inline bool all (bool x)
{
    return x;
}

/** Ternary select operation */
template <typename T>
inline T select (bool b, const T& t, const T& f)
{
    return b ? t : f;
}


#if defined(XSIMD_HPP)
/** Returns true if all the elements in a SIMD vector are equal */
template <typename T>
inline bool all (xsimd::batch_bool<T> x)
{
    return xsimd::all (x);
}

/** Ternary select operation */
template <typename T>
inline xsimd::batch<T> select (const xsimd::batch_bool<T>& b, const xsimd::batch<T>& t, const xsimd::batch<T>& f)
{
    return xsimd::select (b, t, f);
}

template <typename Ret, typename Arg, typename Arch>
xsimd::batch<Ret, Arch> xsimd_cast (const xsimd::batch<Arg, Arch>&);

template <typename Ret, typename Arch>
inline typename std::enable_if<std::is_base_of<xsimd::sse2, Arch>::value, typename xsimd::batch<float, Arch>>::type
    xsimd_cast (const xsimd::batch<int32_t, Arch>& x)
{
    return _mm_cvtepi32_ps (x);
}

template <typename Ret, typename Arch>
inline typename std::enable_if<std::is_base_of<xsimd::sse2, Arch>::value, typename xsimd::batch<double, Arch>>::type
    xsimd_cast (const xsimd::batch<int64_t, Arch>& x)
{
    auto tmp = _mm_shuffle_epi32 (x, _MM_SHUFFLE(1, 3, 2, 0));
    return _mm_cvtepi32_pd (tmp);
}

template <typename Ret, typename Arch>
inline typename std::enable_if<std::is_base_of<xsimd::sse2, Arch>::value, typename xsimd::batch<int32_t, Arch>>::type
    xsimd_cast (const xsimd::batch<float, Arch>& x)
{
    return _mm_cvtps_epi32 (x);
}

template <typename Ret, typename Arch>
inline typename std::enable_if<std::is_base_of<xsimd::sse2, Arch>::value, typename xsimd::batch<int64_t, Arch>>::type
    xsimd_cast (const xsimd::batch<double, Arch>& x)
{
    auto tmp = _mm_cvtpd_epi32 (x);
    return _mm_shuffle_epi32 (tmp, _MM_SHUFFLE(3, 1, 2, 0));
}

template <typename Ret, typename Arch>
inline typename std::enable_if<std::is_base_of<xsimd::avx, Arch>::value, typename xsimd::batch<float, Arch>>::type
    xsimd_cast (const xsimd::batch<int32_t, Arch>& x)
{
    return _mm256_cvtepi32_ps (x);
}

template <typename Ret, typename Arch>
inline typename std::enable_if<std::is_base_of<xsimd::avx2, Arch>::value, typename xsimd::batch<double, Arch>>::type
    xsimd_cast (const xsimd::batch<int64_t, Arch>& x)
{
    auto tmp = _mm256_shuffle_epi32 (x, _MM_SHUFFLE(2, 0, 2, 0));
    tmp = _mm256_permute4x64_epi64 (tmp, _MM_SHUFFLE(2, 1, 2, 1));
    auto tmp2 = _mm256_castsi256_si128 (tmp);
    return _mm256_cvtepi32_pd (tmp2);
}

template <typename Ret, typename Arch>
inline typename std::enable_if<std::is_base_of<xsimd::avx, Arch>::value, typename xsimd::batch<int32_t, Arch>>::type
    xsimd_cast (const xsimd::batch<float, Arch>& x)
{
    return _mm256_cvtps_epi32 (x);
}

template <typename Ret, typename Arch>
inline typename std::enable_if<std::is_base_of<xsimd::avx2, Arch>::value, typename xsimd::batch<int64_t, Arch>>::type
    xsimd_cast (const xsimd::batch<double, Arch>& x)
{
    auto tmp = _mm256_cvtpd_epi32 (x);
    return _mm256_cvtepi32_epi64 (tmp);
}

template <typename Ret, typename Arch>
inline typename std::enable_if<std::is_base_of<xsimd::neon, Arch>::value, typename xsimd::batch<float, Arch>>::type
    xsimd_cast (const xsimd::batch<int32_t, Arch>& x)
{
    return vcvtaq_s32_f32 (x);
}

template <typename Ret, typename Arch>
inline typename std::enable_if<std::is_base_of<xsimd::neon, Arch>::value, typename xsimd::batch<int32_t, Arch>>::type
    xsimd_cast (const xsimd::batch<float, Arch>& x)
{
    return vcvtq_f32_s32 (x);
}

template <typename Ret, typename Arch>
inline typename std::enable_if<std::is_base_of<xsimd::neon, Arch>::value, typename xsimd::batch<double, Arch>>::type
    xsimd_cast (const xsimd::batch<int64_t, Arch>& x)
{
    return vcvtaq_s64_f64 (x);
}

template <typename Ret, typename Arch>
inline typename std::enable_if<std::is_base_of<xsimd::neon, Arch>::value, typename xsimd::batch<int64_t, Arch>>::type
    xsimd_cast (const xsimd::batch<double, Arch>& x)
{
    return vcvtq_f64_s64 (x);
}
#endif
} // namespace chowdsp

#endif // DOXYGEN

#endif //CHOWDSP_WDF_SAMPLE_TYPE_H
