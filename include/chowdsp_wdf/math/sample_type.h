#ifndef CHOWDSP_WDF_SAMPLE_TYPE_H
#define CHOWDSP_WDF_SAMPLE_TYPE_H

#include <type_traits>
#include <xsimd/xsimd.hpp> // @REMOVE_THIS

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

// casting helpers... @TODO: implement for AVX and NEON
template <typename Ret, typename Arg>
xsimd::batch<Ret> xsimd_cast (const xsimd::batch<Arg>&);

template<>
inline xsimd::batch<float> xsimd_cast (const xsimd::batch<int32_t>& x)
{
    return _mm_cvtepi32_ps (x);
}

template<>
inline xsimd::batch<double> xsimd_cast (const xsimd::batch<int64_t>& x)
{
    auto tmp = _mm_shuffle_epi32 (x, _MM_SHUFFLE(1, 3, 2, 0));
    return _mm_cvtepi32_pd (tmp);
}

template<>
inline xsimd::batch<int32_t> xsimd_cast (const xsimd::batch<float>& x)
{
    return _mm_cvtps_epi32 (x);
}

template<>
inline xsimd::batch<int64_t> xsimd_cast (const xsimd::batch<double>& x)
{
    auto tmp = _mm_cvtpd_epi32 (x);
    return _mm_shuffle_epi32 (tmp, _MM_SHUFFLE(3, 1, 2, 0));
}
#endif
} // namespace chowdsp

#endif // DOXYGEN

#endif //CHOWDSP_WDF_SAMPLE_TYPE_H
