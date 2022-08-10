#ifndef CHOWDSP_WDF_WDF_BASE_H
#define CHOWDSP_WDF_WDF_BASE_H

#include <string>
#include <utility>

#include "../wdft/wdft.h"

namespace chowdsp
{
namespace wdf
{
    /** Wave digital filter base class */
    template <typename T>
    class WDF : public wdft::BaseWDF
    {
    public:
        explicit WDF (std::string type) : type (std::move (type)) {}
        ~WDF() override = default;

        void connectToNode (WDF<T>* p) { wdfParent = p; }

        /** Sub-classes override this function to propagate
     * an impedance change to the upstream elements in
     * the WDF tree.
     */
        virtual inline void propagateImpedance()
        {
            calcImpedance();

            if (wdfParent != nullptr)
                wdfParent->propagateImpedance();
        }

        /** Sub-classes override this function to accept an incident wave. */
        virtual void incident (T x) noexcept = 0;

        /** Sub-classes override this function to propogate a reflected wave. */
        virtual T reflected() noexcept = 0;

        /** Probe the voltage across this circuit element. */
        inline T voltage() const noexcept
        {
            return (wdf.a + wdf.b) / (T) 2.0;
        }

        /**Probe the current through this circuit element. */
        inline T current() const noexcept
        {
            return (wdf.a - wdf.b) / ((T) 2.0 * wdf.R);
        }

        // These classes need access to a,b
        template <typename>
        friend class YParameter;

        template <typename>
        friend class WDFParallel;

        template <typename>
        friend class WDFSeries;

        wdft::WDFMembers<T> wdf;

    private:
        const std::string type;

        WDF<T>* wdfParent = nullptr;
    };

    template <typename T, typename WDFType>
    class WDFWrapper : public WDF<T>
    {
    public:
        template <typename... Args>
        explicit WDFWrapper (const std::string& name, Args&&... args) : WDF<T> (name),
                                                                        internalWDF (std::forward<Args> (args)...)
        {
            calcImpedance();
        }

        /** Computes the impedance of the WDF resistor, Z_R = R. */
        inline void calcImpedance() override
        {
            internalWDF.calcImpedance();
            this->wdf.R = internalWDF.wdf.R;
            this->wdf.G = internalWDF.wdf.G;
        }

        /** Accepts an incident wave into a WDF resistor. */
        inline void incident (T x) noexcept override
        {
            this->wdf.a = x;
            internalWDF.incident (x);
        }

        /** Propogates a reflected wave from a WDF resistor. */
        inline T reflected() noexcept override
        {
            this->wdf.b = internalWDF.reflected();
            return this->wdf.b;
        }

    protected:
        WDFType internalWDF;
    };

    template <typename T, typename WDFType>
    class WDFRootWrapper : public WDFWrapper<T, WDFType>
    {
    public:
        template <typename Next, typename... Args>
        WDFRootWrapper (const std::string& name, Next& next, Args&&... args) : WDFWrapper<T, WDFType> (name, std::forward<Args> (args)...)
        {
            next.connectToNode (this);
            calcImpedance();
        }

        inline void propagateImpedance() override
        {
            this->calcImpedance();
        }

        inline void calcImpedance() override
        {
            this->internalWDF.calcImpedance();
        }
    };
} // namespace wdf
} // namespace chowdsp

#endif //CHOWDSP_WDF_WDF_BASE_H
