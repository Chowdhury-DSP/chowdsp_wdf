#ifndef CHOWDSP_WDF_WDFT_SOURCES_H
#define CHOWDSP_WDF_WDFT_SOURCES_H

#include "wdft_base.h"

namespace chowdsp
{
namespace wdft
{
    /** WDF Ideal Voltage source (non-adaptable) */
    template <typename T, typename Next>
    class IdealVoltageSourceT final : public RootWDF
    {
    public:
        explicit IdealVoltageSourceT (Next& next)
        {
            next.connectToParent (this);
            calcImpedance();
        }

        void calcImpedance() override {}

        /** Sets the voltage of the voltage source, in Volts */
        void setVoltage (T newV) { Vs = newV; }

        /** Accepts an incident wave into a WDF ideal voltage source. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
        }

        /** Propogates a reflected wave from a WDF ideal voltage source. */
        inline T reflected() noexcept
        {
            wdf.b = -wdf.a + (T) 2.0 * Vs;
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        T Vs = (T) 0.0;
    };

    /** WDF Voltage source with series resistance */
    template <typename T>
    class ResistiveVoltageSourceT final : public BaseWDF
    {
    public:
        /** Creates a new resistive voltage source.
         * @param value: initial resistance value, in Ohms
         */
        explicit ResistiveVoltageSourceT (T value = NumericType<T> (1.0e-9)) : R_value (value)
        {
            calcImpedance();
        }

        /** Sets the resistance value of the series resistor, in Ohms. */
        void setResistanceValue (T newR)
        {
            if (newR == R_value)
                return;

            R_value = newR;
            propagateImpedanceChange();
        }

        /** Computes the impedance for a WDF resistive voltage souce
         * Z_Vr = Z_R
         */
        inline void calcImpedance() override
        {
            wdf.R = R_value;
            wdf.G = (T) 1.0 / wdf.R;
        }

        /** Sets the voltage of the voltage source, in Volts */
        void setVoltage (T newV) { Vs = newV; }

        /** Accepts an incident wave into a WDF resistive voltage source. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
        }

        /** Propogates a reflected wave from a WDF resistive voltage source. */
        inline T reflected() noexcept
        {
            wdf.b = Vs;
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        T Vs = (T) 0.0;
        T R_value = (T) 1.0e-9;
    };

    /** WDF Current source (non-adaptable) */
    template <typename T, typename Next>
    class IdealCurrentSourceT final : public RootWDF
    {
    public:
        explicit IdealCurrentSourceT (Next& n) : next (n)
        {
            n.connectToParent (this);
            calcImpedance();
        }

        inline void calcImpedance() override
        {
            twoR = (T) 2.0 * next.wdf.R;
            twoR_Is = twoR * Is;
        }

        /** Sets the current of the current source, in Amps */
        void setCurrent (T newI)
        {
            Is = newI;
            twoR_Is = twoR * Is;
        }

        /** Accepts an incident wave into a WDF ideal current source. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
        }

        /** Propogates a reflected wave from a WDF ideal current source. */
        inline T reflected() noexcept
        {
            wdf.b = twoR_Is + wdf.a;
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        const Next& next;

        T Is = (T) 0.0;
        T twoR;
        T twoR_Is;
    };

    /** WDF Current source with parallel resistance */
    template <typename T>
    class ResistiveCurrentSourceT final : public BaseWDF
    {
    public:
        /** Creates a new resistive current source.
         * @param value: initial resistance value, in Ohms
         */
        explicit ResistiveCurrentSourceT (T value = NumericType<T> (1.0e9)) : R_value (value)
        {
            calcImpedance();
        }

        /** Sets the resistance value of the parallel resistor, in Ohms. */
        void setResistanceValue (T newR)
        {
            if (newR == R_value)
                return;

            R_value = newR;
            propagateImpedanceChange();
        }

        /** Computes the impedance for a WDF resistive current souce
         * Z_Ir = Z_R
         */
        inline void calcImpedance() override
        {
            wdf.R = R_value;
            wdf.G = (T) 1.0 / wdf.R;
        }

        /** Sets the current of the current source, in Amps */
        void setCurrent (T newI) { Is = newI; }

        /** Accepts an incident wave into a WDF resistive current source. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
        }

        /** Propogates a reflected wave from a WDF resistive current source. */
        inline T reflected() noexcept
        {
            wdf.b = wdf.R * Is;
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        T Is = (T) 0.0;
        T R_value = (T) 1.0e9;
    };
} // namespace wdft
} // namespace chowdsp

#endif //CHOWDSP_WDF_WDFT_SOURCES_H
