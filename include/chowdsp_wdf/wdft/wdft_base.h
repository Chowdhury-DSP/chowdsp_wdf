#ifndef CHOWDSP_WDF_WDFT_BASE_H
#define CHOWDSP_WDF_WDFT_BASE_H

#include "../math/sample_type.h"

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
            if (dontPropagateImpedance)
                return; // the impedance propagation is being deferred until later...

            calcImpedance();

            if (parent != nullptr)
                parent->propagateImpedanceChange();
        }

    protected:
        BaseWDF* parent = nullptr;

    private:
        bool dontPropagateImpedance = false;

        template <typename... Elements>
        friend class ScopedDeferImpedancePropagation;
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

    /** Helper struct for common WDF member variables */
    template <typename T>
    struct WDFMembers
    {
        T R = (NumericType<T>) 1.0e-9; /* impedance */
        T G = (T) 1.0 / R; /* admittance */
        T a = (T) 0.0; /* incident wave */
        T b = (T) 0.0; /* reflected wave */
    };

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
