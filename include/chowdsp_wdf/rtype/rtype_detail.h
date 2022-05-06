#ifndef CHOWDSP_WDF_RTYPE_DETAIL_H
#define CHOWDSP_WDF_RTYPE_DETAIL_H

#include <array>
#include <initializer_list>
#include <tuple>

namespace chowdsp
{
namespace wdft
{
#ifndef DOXYGEN
    /** Utility functions used internally by the R-Type adaptor */
    namespace rtype_detail
    {
        /** Functions to do a function for each element in the tuple */
        template <typename Fn, typename Tuple, size_t... Ix>
        constexpr void forEachInTuple (Fn&& fn, Tuple&& tuple, std::index_sequence<Ix...>) noexcept (noexcept (std::initializer_list<int> { (fn (std::get<Ix> (tuple), Ix), 0)... }))
        {
            (void) std::initializer_list<int> { ((void) fn (std::get<Ix> (tuple), Ix), 0)... };
        }

        template <typename T>
        using TupleIndexSequence = std::make_index_sequence<std::tuple_size<std::remove_cv_t<std::remove_reference_t<T>>>::value>;

        template <typename Fn, typename Tuple>
        constexpr void forEachInTuple (Fn&& fn, Tuple&& tuple) noexcept (noexcept (forEachInTuple (std::forward<Fn> (fn), std::forward<Tuple> (tuple), TupleIndexSequence<Tuple> {})))
        {
            forEachInTuple (std::forward<Fn> (fn), std::forward<Tuple> (tuple), TupleIndexSequence<Tuple> {});
        }

        template <typename T, int N>
        using Array = T[(size_t) N];

        template <typename ElementType, int arraySize, int alignment = CHOWDSP_WDF_DEFAULT_SIMD_ALIGNMENT>
        struct AlignedArray
        {
            ElementType& operator[] (int index) noexcept { return array[index]; }
            const ElementType& operator[] (int index) const noexcept { return array[index]; }

            ElementType* data() noexcept { return array; }
            const ElementType* data() const noexcept { return array; }

            static constexpr int size() { return arraySize; }
            alignas (alignment) Array<ElementType, arraySize> array;
        };

        template <typename T, int nRows, int nCols = nRows, int alignment = CHOWDSP_WDF_DEFAULT_SIMD_ALIGNMENT>
        using Matrix = AlignedArray<T, nRows, alignment>[(size_t) nCols];

        /** Implementation for float/double. */
        template <typename T, int numPorts>
        constexpr typename std::enable_if<std::is_floating_point<T>::value, void>::type
            RtypeScatter (const Matrix<T, numPorts>& S_, const Array<T, numPorts>& a_, Array<T, numPorts>& b_)
        {
            // input matrix (S) of size dim x dim
            // input vector (a) of size 1 x dim
            // output vector (b) of size 1 x dim

#if defined(XSIMD_HPP)
            using v_type = xsimd::simd_type<T>;
            constexpr auto simd_size = (int) v_type::size;
            constexpr auto vec_size = numPorts - numPorts % simd_size;

            for (int c = 0; c < numPorts; ++c)
            {
                v_type bc {};
                for (int r = 0; r < vec_size; r += simd_size)
                    bc = xsimd::fma (xsimd::load_aligned (S_[c].data() + r), xsimd::load_aligned (a_ + r), bc);
                b_[c] = xsimd::hadd (bc);

                // remainder of ops that can't be vectorized
                for (int r = vec_size; r < numPorts; ++r)
                    b_[c] += S_[c][r] * a_[r];
            }
#else // No SIMD
            for (int c = 0; c < numPorts; ++c)
            {
                b_[c] = (T) 0;
                for (int r = 0; r < numPorts; ++r)
                    b_[c] += S_[c][r] * a_[r];
            }
#endif // SIMD options
        }

#if defined(XSIMD_HPP)
        /** Implementation for SIMD float/double. */
        template <typename T, int numPorts>
        constexpr typename std::enable_if<! std::is_floating_point<T>::value, void>::type
            RtypeScatter (const Matrix<T, numPorts>& S_, const Array<T, numPorts>& a_, Array<T, numPorts>& b_)
        {
            for (int c = 0; c < numPorts; ++c)
            {
                b_[c] = (T) 0;
                for (int r = 0; r < numPorts; ++r)
                    b_[c] += S_[c][r] * a_[r];
            }
        }
#endif // XSIMD
    } // namespace rtype_detail
#endif // DOXYGEN

} // namespace wdft
} // namespace chowdsp

#endif //CHOWDSP_WDF_RTYPE_DETAIL_H
