#pragma once

namespace chowdsp
{
/** Methods for implementing the signum function */
namespace signum
{
    /** Signum function to determine the sign of the input. */
    template <typename T>
    inline int signum (T val)
    {
        return (T (0) < val) - (val < T (0));
    }

#if defined(XSIMD_HPP)
    /** Signum function to determine the sign of the input. */
    template <typename T>
    inline xsimd::batch<T> signum (xsimd::batch<T> val)
    {
        using v_type = xsimd::batch<T>;
        const auto positive = xsimd::select (val > v_type ((T) 0), v_type ((T) 1), v_type ((T) 0));
        const auto negative = xsimd::select (val < v_type ((T) 0), v_type ((T) 1), v_type ((T) 0));
        return positive - negative;
    }
#endif

} // namespace signum
} // namespace chowdsp
