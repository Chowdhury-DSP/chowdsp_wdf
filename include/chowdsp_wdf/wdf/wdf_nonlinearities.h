#ifndef CHOWDSP_WDF_WDF_NONLINEARITIES_H
#define CHOWDSP_WDF_WDF_NONLINEARITIES_H

#include "wdf_base.h"

namespace chowdsp
{
namespace wdf
{
    /** WDF Switch (non-adaptable) */
    template <typename T>
    class Switch final : public WDFWrapper<T, wdft::SwitchT<T, WDF<T>>>
    {
    public:
        explicit Switch (WDF<T>* next) : WDFWrapper<T, wdft::SwitchT<T, WDF<T>>> ("Switch", *next)
        {
            next->connectToNode (this);
        }

        /** Sets the state of the switch. */
        void setClosed (bool shouldClose)
        {
            this->internalWDF.setClosed (shouldClose);
        }
    };

    /** WDF Open circuit (non-adaptable) */
    template <typename T>
    class Open final : public WDF<T>
    {
    public:
        Open() : WDF<T> ("Open")
        {
            this->R = (T) 1.0e15;
            this->G = (T) 1.0 / this->R;
        }

        inline void calcImpedance() override {}

        /** Accepts an incident wave into a WDF open. */
        inline void incident (T x) noexcept override
        {
            this->a = x;
        }

        /** Propogates a reflected wave from a WDF open. */
        inline T reflected() noexcept override
        {
            this->b = this->a;
            return this->b;
        }
    };

    /** WDF Short circuit (non-adaptable) */
    template <typename T>
    class Short final : public WDF<T>
    {
    public:
        Short() : WDF<T> ("Short")
        {
            this->R = (T) 1.0e-15;
            this->G = (T) 1.0 / this->R;
        }

        inline void calcImpedance() override {}

        /** Accepts an incident wave into a WDF short. */
        inline void incident (T x) noexcept override
        {
            this->a = x;
        }

        /** Propogates a reflected wave from a WDF short. */
        inline T reflected() noexcept override
        {
            this->b = -this->a;
            return this->b;
        }
    };

    /**
     * WDF diode pair (non-adaptable)
     * See Werner et al., "An Improved and Generalized Diode Clipper Model for Wave Digital Filters"
     * https://www.researchgate.net/publication/299514713_An_Improved_and_Generalized_Diode_Clipper_Model_for_Wave_Digital_Filters
     */
    template <typename T, wdft::DiodeQuality Q = wdft::DiodeQuality::Best>
    class DiodePair final : public WDFRootWrapper<T, wdft::DiodePairT<T, WDF<T>, Q>>
    {
    public:
        /**
         * Creates a new WDF diode pair, with the given diode specifications.
         * @param next: the next element in the WDF connection tree
         * @param Is: reverse saturation current
         * @param Vt: thermal voltage
         * @param nDiodes: the number of series diodes
         */
        DiodePair (WDF<T>* next, T Is, T Vt = (NumericType<T>) 25.85e-3, T nDiodes = (T) 1) : WDFRootWrapper<T, wdft::DiodePairT<T, WDF<T>, Q>> ("DiodePair", *next, *next, Is, Vt, nDiodes)
        {
            next->connectToNode (this);
        }

        /** Sets diode specific parameters */
        void setDiodeParameters (T newIs, T newVt, T nDiodes)
        {
            this->internalWDF.setDiodeParameters (newIs, newVt, nDiodes);
        }
    };

    /**
     * WDF diode (non-adaptable)
     * See Werner et al., "An Improved and Generalized Diode Clipper Model for Wave Digital Filters"
     * https://www.researchgate.net/publication/299514713_An_Improved_and_Generalized_Diode_Clipper_Model_for_Wave_Digital_Filters
     */
    template <typename T>
    class Diode final : public WDFRootWrapper<T, wdft::DiodeT<T, WDF<T>>>
    {
    public:
        /**
         * Creates a new WDF diode, with the given diode specifications.
         * @param next: the next element in the WDF connection tree
         * @param Is: reverse saturation current
         * @param Vt: thermal voltage
         * @param nDiodes: the number of series diodes
         */
        Diode (WDF<T>* next, T Is, T Vt = (NumericType<T>) 25.85e-3, T nDiodes = 1) : WDFRootWrapper<T, wdft::DiodeT<T, WDF<T>>> ("Diode", *next, *next, Is, Vt, nDiodes)
        {
            next->connectToNode (this);
        }

        /** Sets diode specific parameters */
        void setDiodeParameters (T newIs, T newVt, T nDiodes)
        {
            this->internalWDF.setDiodeParameters (newIs, newVt, nDiodes);
        }
    };
} // namespace wdf
} // namespace chowdsp

#endif //CHOWDSP_WDF_WDF_NONLINEARITIES_H
