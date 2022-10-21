/*
 * Copyright (C) 2019 Stefano D'Angelo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 * NOTE: code has been modified significantly by Jatin Chowdhury
 */

#ifndef OMEGA_H_INCLUDED
#define OMEGA_H_INCLUDED

#include <algorithm>
#include "sample_type.h"

namespace chowdsp
{
/**
 * Useful approximations for evaluating the Wright Omega function.
 *
 * This approach was devloped by Stefano D'Angelo, and adapted from his
 * original implementation under the MIT license.
 * - Paper: https://www.dafx.de/paper-archive/2019/DAFx2019_paper_5.pdf
 * - Original Source: https://www.dangelo.audio/dafx2019-omega.html
 */
namespace Omega
{
    /**
     * Evaluates a polynomial of a given order, using Estrin's scheme.
     * Coefficients should be given in the form { a_n, a_n-1, ..., a_1, a_0 }
     * https://en.wikipedia.org/wiki/Estrin%27s_scheme
     */
    template <int ORDER, typename T, typename X>
    inline typename std::enable_if<(ORDER == 1), decltype (T {} * X {})>::type estrin (const T (&coeffs)[ORDER + 1], const X& x)
    {
        return coeffs[1] + coeffs[0] * x;
    }

    template <int ORDER, typename T, typename X>
    inline typename std::enable_if<(ORDER > 1), decltype (T {} * X {})>::type estrin (const T (&coeffs)[ORDER + 1], const X& x)
    {
        decltype (T {} * X {}) temp[ORDER / 2 + 1];
        for (int n = ORDER; n >= 0; n -= 2)
            temp[n / 2] = coeffs[n] + coeffs[n - 1] * x;

        temp[0] = (ORDER % 2 == 0) ? coeffs[0] : temp[0];

        return estrin<ORDER / 2> (temp, x * x); // recurse!
    }

    /** approximation for log_2(x), optimized on the range [1, 2] */
    template <typename T>
    constexpr T log2_approx (T x)
    {
        constexpr auto alpha = (T) 0.1640425613334452;
        constexpr auto beta = (T) -1.098865286222744;
        constexpr auto gamma = (T) 3.148297929334117;
        constexpr auto zeta = (T) -2.213475204444817;

        return estrin<3> ({ alpha, beta, gamma, zeta }, x);
    }

#if defined(XSIMD_HPP)
    /** approximation for log_2(x), optimized on the range [1, 2] */
    template <typename T>
    inline xsimd::batch<T> log2_approx (const xsimd::batch<T>& x)
    {
        static constexpr auto alpha = (NumericType<T>) 0.1640425613334452;
        static constexpr auto beta = (NumericType<T>) -1.098865286222744;
        static constexpr auto gamma = (NumericType<T>) 3.148297929334117;
        static constexpr auto zeta = (NumericType<T>) -2.213475204444817;

        return estrin<3> ({ alpha, beta, gamma, zeta }, x);
    }
#endif

    /** approximation for log(x) */
    template <typename T>
    constexpr T log_approx (T x);

    /** approximation for log(x) (32-bit) */
    template <>
    CHOWDSP_WDF_MAYBE_UNUSED constexpr float log_approx (float x)
    {
        union
        {
            int32_t i;
            float f;
        } v {};
        v.f = x;
        int32_t ex = v.i & 0x7f800000;
        int32_t e = (ex >> 23) - 127;
        v.i = (v.i - ex) | 0x3f800000;

        return 0.693147180559945f * ((float) e + log2_approx<float> (v.f));
    }

    /** approximation for log(x) (64-bit) */
    template <>
    CHOWDSP_WDF_MAYBE_UNUSED constexpr double log_approx (double x)
    {
        union
        {
            int64_t i;
            double d;
        } v {};
        v.d = x;
        int64_t ex = v.i & 0x7ff0000000000000;
        int64_t e = (ex >> 53) - 510;
        v.i = (v.i - ex) | 0x3ff0000000000000;

        return 0.693147180559945 * ((double) e + log2_approx<double> (v.d));
    }

#if defined(XSIMD_HPP)
    /** approximation for log(x) (SIMD 32-bit) */
    template <>
    CHOWDSP_WDF_MAYBE_UNUSED inline xsimd::batch<float> log_approx (xsimd::batch<float> x)
    {
        union
        {
            xsimd::batch<int32_t> i;
            xsimd::batch<float> f;
        } v;
        v.f = x;
        xsimd::batch<int32_t> ex = v.i & 0x7f800000;
        xsimd::batch<float> e = xsimd::to_float ((ex >> 23) - 127);
        v.i = (v.i - ex) | 0x3f800000;

        return 0.693147180559945f * (log2_approx<float> (v.f) + e);
    }

    /** approximation for log(x) (64-bit) */
    template <>
    CHOWDSP_WDF_MAYBE_UNUSED inline xsimd::batch<double> log_approx (xsimd::batch<double> x)
    {
        union
        {
            xsimd::batch<int64_t> i;
            xsimd::batch<double> d;
        } v {};
        v.d = x;
        xsimd::batch<int64_t> ex = v.i & 0x7ff0000000000000;
        xsimd::batch<double> e = xsimd::to_float ((ex >> 53) - 510);
        v.i = (v.i - ex) | 0x3ff0000000000000;

        return 0.693147180559945 * (e + log2_approx<double> (v.d));
    }
#endif

    /** approximation for 2^x, optimized on the range [0, 1] */
    template <typename T>
    constexpr T pow2_approx (T x)
    {
        constexpr auto alpha = (T) 0.07944154167983575;
        constexpr auto beta = (T) 0.2274112777602189;
        constexpr auto gamma = (T) 0.6931471805599453;
        constexpr auto zeta = (T) 1.0;

        return estrin<3> ({ alpha, beta, gamma, zeta }, x);
    }

#if defined(XSIMD_HPP)
    template <typename T>
    inline xsimd::batch<T> pow2_approx (const xsimd::batch<T>& x)
    {
        static constexpr auto alpha = (NumericType<T>) 0.07944154167983575;
        static constexpr auto beta = (NumericType<T>) 0.2274112777602189;
        static constexpr auto gamma = (NumericType<T>) 0.6931471805599453;
        static constexpr auto zeta = (NumericType<T>) 1.0;

        return estrin<3> ({ alpha, beta, gamma, zeta }, x);
    }
#endif

    /** approximation for exp(x) */
    template <typename T>
    T exp_approx (T x);

    /** approximation for exp(x) (32-bit) */
    template <>
    CHOWDSP_WDF_MAYBE_UNUSED constexpr float exp_approx (float x)
    {
        x = std::max (-126.0f, 1.442695040888963f * x);

        union
        {
            int32_t i;
            float f;
        } v {};

        auto xi = (int32_t) x;
        int32_t l = x < 0.0f ? xi - 1 : xi;
        float f = x - (float) l;
        v.i = (l + 127) << 23;

        return v.f * pow2_approx<float> (f);
    }

    /** approximation for exp(x) (64-bit) */
    template <>
    CHOWDSP_WDF_MAYBE_UNUSED constexpr double exp_approx (double x)
    {
        x = std::max (-126.0, 1.442695040888963 * x);

        union
        {
            int64_t i;
            double d;
        } v {};

        auto xi = (int64_t) x;
        int64_t l = x < 0.0 ? xi - 1 : xi;
        double d = x - (double) l;
        v.i = (l + 1023) << 52;

        return v.d * pow2_approx<double> (d);
    }

#if defined(XSIMD_HPP)
    /** approximation for exp(x) (32-bit) */
    template <>
    CHOWDSP_WDF_MAYBE_UNUSED inline xsimd::batch<float> exp_approx (xsimd::batch<float> x)
    {
        x = xsimd::max (xsimd::broadcast (-126.0f), 1.442695040888963f * x);

        union
        {
            xsimd::batch<int32_t> i;
            xsimd::batch<float> f;
        } v {};

        auto xi = xsimd::to_int (x);
        xsimd::batch<int32_t> l = xsimd::select (xi < 0, xi - 1, xi);
        xsimd::batch<float> f = x - xsimd::to_float (l);
        v.i = (l + 127) << 23;

        return v.f * pow2_approx<float> (f);
    }

    /** approximation for exp(x) (64-bit) */
    template <>
    CHOWDSP_WDF_MAYBE_UNUSED inline xsimd::batch<double> exp_approx (xsimd::batch<double> x)
    {
        x = xsimd::max (xsimd::broadcast (-126.0), 1.442695040888963 * x);

        union
        {
            xsimd::batch<int64_t> i;
            xsimd::batch<double> d;
        } v {};

        auto xi = xsimd::to_int (x);
        xsimd::batch<int64_t> l = xsimd::select (xi < 0, xi - 1, xi);
        xsimd::batch<double> d = x - xsimd::to_float (l);
        v.i = (l + 1023) << 52;

        return v.d * pow2_approx<double> (d);
    }
#endif

    /** First-order approximation of the Wright Omega functions */
    template <typename T>
    constexpr T omega1 (T x)
    {
#if defined(XSIMD_HPP)
        using xsimd::max;
#endif
        using std::max;

        return max (x, (T) 0);
    }

    /** Second-order approximation of the Wright Omega functions */
    template <typename T>
    constexpr T omega2 (T x)
    {
        constexpr auto x1 = (NumericType<T>) -3.684303659906469;
        constexpr auto x2 = (NumericType<T>) 1.972967391708859;
        constexpr auto a = (NumericType<T>) 9.451797158780131e-3;
        constexpr auto b = (NumericType<T>) 1.126446405111627e-1;
        constexpr auto c = (NumericType<T>) 4.451353886588814e-1;
        constexpr auto d = (NumericType<T>) 5.836596684310648e-1;

        return select (x < x1, (T) 0, select (x > x2, x, estrin<3> ({ a, b, c, d }, x)));
    }

    /** Third-order approximation of the Wright Omega functions */
    template <typename T>
    constexpr T omega3 (T x)
    {
        constexpr auto x1 = (NumericType<T>) -3.341459552768620;
        constexpr auto x2 = (NumericType<T>) 8.0;
        constexpr auto a = (NumericType<T>) -1.314293149877800e-3;
        constexpr auto b = (NumericType<T>) 4.775931364975583e-2;
        constexpr auto c = (NumericType<T>) 3.631952663804445e-1;
        constexpr auto d = (NumericType<T>) 6.313183464296682e-1;

        return select (x < x1, (T) 0, select (x < x2, estrin<3> ({ a, b, c, d }, x), x - log_approx<T> (x)));
    }

    /** Fourth-order approximation of the Wright Omega functions */
    template <typename T>
    constexpr T omega4 (T x)
    {
        const auto y = omega3<T> (x);
        return y - (y - exp_approx<T> (x - y)) / (y + (T) 1);
    }

} // namespace Omega
} // namespace chowdsp

#endif //OMEGA_H_INCLUDED
