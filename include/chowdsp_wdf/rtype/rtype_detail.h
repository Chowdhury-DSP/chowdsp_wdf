#ifndef CHOWDSP_WDF_RTYPE_DETAIL_H
#define CHOWDSP_WDF_RTYPE_DETAIL_H

#include <array>
#include <algorithm>
#include <initializer_list>
#include <tuple>
#include <vector>

namespace chowdsp
{
#ifndef DOXYGEN
namespace wdft
{
    /** Utility functions used internally by the R-Type adaptor */
    namespace rtype_detail
    {
        /** Divides two numbers and rounds up if there is a remainder. */
        template <typename T>
        constexpr T ceil_div (T num, T den)
        {
            return (num + den - 1) / den;
        }

        template <typename T, size_t base_size>
        constexpr typename std::enable_if<std::is_floating_point<T>::value, size_t>::type array_pad()
        {
#if defined(XSIMD_HPP)
            using v_type = xsimd::simd_type<T>;
            constexpr auto simd_size = v_type::size;
            constexpr auto num_simd_registers = ceil_div (base_size, simd_size);
            return num_simd_registers * simd_size;
#else
            return base_size;
#endif
        }

        template <typename T, size_t base_size>
        constexpr typename std::enable_if<! std::is_floating_point<T>::value, size_t>::type array_pad()
        {
            return base_size;
        }

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

        template <typename ElementType, int arraySize, int alignment = CHOWDSP_WDF_DEFAULT_SIMD_ALIGNMENT>
        struct AlignedArray
        {
            template <typename IntType>
            ElementType& operator[] (IntType index) noexcept
            {
                return array[index];
            }
            template <typename IntType>
            const ElementType& operator[] (IntType index) const noexcept
            {
                return array[index];
            }

            ElementType* data() noexcept { return array; }
            const ElementType* data() const noexcept { return array; }

            void clear() { std::fill (std::begin (array), std::end (array), ElementType {}); }
            static constexpr int size() noexcept { return arraySize; }

        private:
            alignas (alignment) ElementType array[array_pad<ElementType, (size_t) arraySize>()] {};
        };

        template <typename T, int nRows, int nCols = nRows, int alignment = CHOWDSP_WDF_DEFAULT_SIMD_ALIGNMENT>
        using Matrix = AlignedArray<T, nRows, alignment>[(size_t) nCols];

        /** Implementation for float/double. */
        template <typename T, int numPorts>
        constexpr typename std::enable_if<std::is_floating_point<T>::value, void>::type
            RtypeScatter (const Matrix<T, numPorts>& S_, const AlignedArray<T, numPorts>& a_, AlignedArray<T, numPorts>& b_)
        {
            // input matrix (S) of size dim x dim
            // input vector (a) of size 1 x dim
            // output vector (b) of size 1 x dim

#if defined(XSIMD_HPP)
            using v_type = xsimd::simd_type<T>;
            constexpr auto simd_size = (int) v_type::size;
            constexpr auto vec_size = ceil_div (numPorts, simd_size) * simd_size;

            for (int c = 0; c < vec_size; c += simd_size)
            {
                auto b_vec = a_[0] * xsimd::load_aligned (S_[0].data() + c);
                for (int r = 1; r < numPorts; ++r)
                    b_vec = xsimd::fma (xsimd::broadcast (a_[r]), xsimd::load_aligned (S_[r].data() + c), b_vec);

                xsimd::store_aligned (b_.data() + c, b_vec);
            }
#else // No SIMD
            for (int c = 0; c < numPorts; ++c)
            {
                b_[c] = S_[0][c] * a_[0];
                for (int r = 1; r < numPorts; ++r)
                    b_[c] += S_[r][c] * a_[r];
            }
#endif // SIMD options
        }

#if defined(XSIMD_HPP)
        /** Implementation for SIMD float/double. */
        template <typename T, int numPorts>
        constexpr typename std::enable_if<! std::is_floating_point<T>::value, void>::type
            RtypeScatter (const Matrix<T, numPorts>& S_, const AlignedArray<T, numPorts>& a_, AlignedArray<T, numPorts>& b_)
        {
            for (int c = 0; c < numPorts; ++c)
            {
                b_[c] = S_[0][c] * a_[0];
                for (int r = 1; r < numPorts; ++r)
                    b_[c] += S_[r][c] * a_[r];
            }
        }
#endif // XSIMD
    } // namespace rtype_detail
} // namespace wdft

namespace wdf
{
    /** Utility functions used internally by the R-Type adaptor */
    namespace rtype_detail
    {
        using wdft::rtype_detail::ceil_div;

        template <typename T>
        typename std::enable_if<std::is_floating_point<T>::value, size_t>::type array_pad (size_t base_size)
        {
#if defined(XSIMD_HPP)
            using v_type = xsimd::simd_type<T>;
            constexpr auto simd_size = v_type::size;
            const auto num_simd_registers = ceil_div (base_size, simd_size);
            return num_simd_registers * simd_size;
#else
            return base_size;
#endif
        }

        template <typename T>
        typename std::enable_if<! std::is_floating_point<T>::value, size_t>::type array_pad (size_t base_size)
        {
            return base_size;
        }

        template <typename ElementType>
        struct AlignedArray
        {
            explicit AlignedArray (size_t size) : m_size ((int) size),
                                                  vector (array_pad<ElementType> (size), ElementType {})
            {
            }

            ElementType& operator[] (int index) noexcept { return vector[index]; }
            const ElementType& operator[] (int index) const noexcept { return vector[index]; }

            ElementType* data() noexcept { return vector.data(); }
            const ElementType* data() const noexcept { return vector.data(); }

            void clear() { std::fill (std::begin (vector), std::end (vector), ElementType {}); }
            int size() const noexcept { return (int) m_size; }

        private:
            const int m_size;
#if defined(XSIMD_HPP)
            std::vector<ElementType, xsimd::default_allocator<ElementType>> vector;
#else
            std::vector<ElementType> vector;
#endif
        };

        template <typename ElementType>
        struct Matrix
        {
            Matrix (size_t nRows, size_t nCols) : vector (nCols, AlignedArray<ElementType> (nRows))
            {
            }

            AlignedArray<ElementType>& operator[] (int index) noexcept { return vector[(size_t) index]; }
            const AlignedArray<ElementType>& operator[] (int index) const noexcept { return vector[(size_t) index]; }

        private:
            std::vector<AlignedArray<ElementType>> vector;
        };

        /** Implementation for float/double. */
        template <typename T>
        constexpr typename std::enable_if<std::is_floating_point<T>::value, void>::type
            RtypeScatter (const Matrix<T>& S_, const AlignedArray<T>& a_, AlignedArray<T>& b_)
        {
            // input matrix (S) of size dim x dim
            // input vector (a) of size 1 x dim
            // output vector (b) of size 1 x dim

#if defined(XSIMD_HPP)
            using v_type = xsimd::simd_type<T>;
            constexpr auto simd_size = (int) v_type::size;
            const auto numPorts = a_.size();
            const auto vec_size = ceil_div (numPorts, simd_size) * simd_size;

            for (int c = 0; c < vec_size; c += simd_size)
            {
                auto b_vec = a_[0] * xsimd::load_aligned (S_[0].data() + c);
                for (int r = 1; r < numPorts; ++r)
                    b_vec = xsimd::fma (xsimd::broadcast (a_[r]), xsimd::load_aligned (S_[r].data() + c), b_vec);

                xsimd::store_aligned (b_.data() + c, b_vec);
            }
#else // No SIMD
            const auto numPorts = a_.size();
            for (int c = 0; c < numPorts; ++c)
            {
                b_[c] = S_[0][c] * a_[0];
                for (int r = 1; r < numPorts; ++r)
                    b_[c] += S_[r][c] * a_[r];
            }
#endif // SIMD options
        }

#if defined(XSIMD_HPP)
        /** Implementation for SIMD float/double. */
        template <typename T>
        constexpr typename std::enable_if<! std::is_floating_point<T>::value, void>::type
            RtypeScatter (const Matrix<T>& S_, const AlignedArray<T>& a_, AlignedArray<T>& b_)
        {
            const auto numPorts = a_.size();
            for (int c = 0; c < numPorts; ++c)
            {
                b_[c] = S_[0][c] * a_[0];
                for (int r = 1; r < numPorts; ++r)
                    b_[c] += S_[r][c] * a_[r];
            }
        }
#endif // XSIMD
    } // namespace rtype_detail
} // namespace wdf
#endif // DOXYGEN
} // namespace chowdsp

#endif //CHOWDSP_WDF_RTYPE_DETAIL_H
