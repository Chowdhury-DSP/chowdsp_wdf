#ifndef CHOWDSP_WDF_WDFT_BASE_H
#define CHOWDSP_WDF_WDFT_BASE_H

#include <type_traits>

namespace chowdsp
{
namespace wdft
{
    /** Base WDF class for propagating impedance changes between elements */
    class BaseWDF
    {
    public:
        virtual ~BaseWDF() = default;

        void connectToParent (BaseWDF* p) { parent = p; }

        virtual void calcImpedance() = 0;

        inline virtual void propagateImpedanceChange()
        {
            calcImpedance();

            if (parent != nullptr)
                parent->propagateImpedanceChange();
        }

    protected:
        BaseWDF* parent = nullptr;
    };

    /** Base class for propagating impedance changes into root WDF elements */
    class RootWDF : public BaseWDF
    {
    public:
        inline void propagateImpedanceChange() override { calcImpedance(); }

    private:
        // don't try to connect root nodes!
        void connectToParent (BaseWDF*) {}
    };

#ifndef DOXYGEN
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

    /** Helper struct for common WDF member variables */
    template <typename T>
    struct WDFMembers
    {
#if defined(XSIMD_HPP)
        using NumericType = typename SampleTypeHelpers::ElementType<T>::Type;
#else
        using NumericType = T;
#endif
        T R = (NumericType) 1.0e-9; /* impedance */
        T G = (T) 1.0 / R; /* admittance */
        T a = (T) 0.0; /* incident wave */
        T b = (T) 0.0; /* reflected wave */
    };

    /** Type alias for a SIMD numeric type */
    template <typename T>
    using NumericType = typename WDFMembers<T>::NumericType;

    /** Returns true if all the elements in a SIMD vector are equal */
    inline bool all (bool x)
    {
        return x;
    }

#if defined(XSIMD_HPP)
    /** Returns true if all the elements in a SIMD vector are equal */
    template <typename T>
    inline bool all (xsimd::batch_bool<T> x)
    {
        return xsimd::all (x);
    }
#endif
#endif // DOXYGEN

    /** Probe the voltage across this circuit element. */
    template <typename T, typename WDFType>
    inline T voltage (const WDFType& wdf) noexcept
    {
        return (wdf.wdf.a + wdf.wdf.b) * (T) 0.5;
    }

    /**Probe the current through this circuit element. */
    template <typename T, typename WDFType>
    inline T current (const WDFType& wdf) noexcept
    {
        return (wdf.wdf.a - wdf.wdf.b) * ((T) 0.5 * wdf.wdf.G);
    }
} // namespace wdft
} // namespace chowdsp

#endif // CHOWDSP_WDF_WDFT_BASE_H
