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
            if (all (newR == R_value))
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

    /** WDF Voltage source with series capacitance */
    template <typename T>
    class CapacitiveVoltageSourceT final : public BaseWDF
    {
    public:
        /** Creates a new resistive voltage source.
         * @param value: initial resistance value, in Ohms
         */
        explicit CapacitiveVoltageSourceT (T value = NumericType<T> (1.0e-6), T fs = (T) 48000)
            : C_value (value),
              fs (fs)

        {
            calcImpedance();
        }

        /** Prepares the capacitor to operate at a new sample rate */
        void prepare (T sampleRate)
        {
            fs = sampleRate;
            propagateImpedanceChange();

            reset();
        }

        void reset()
        {
            z = (T) 0;
            v_1 = (T) 0;
        }

        /** Sets the capacitance value of the series resistor, in Farads. */
        void setCapacitanceValue (T newC)
        {
            if (newC == C_value)
                return;

            C_value = newC;
            propagateImpedanceChange();
        }

        /** Computes the impedance of the WDF capacitor,
         *             1
         * Z_C = --------------
         *        2 * f_s * C
         */
        inline void calcImpedance() override
        {
            wdf.R = (T) 1.0 / ((T) 2.0 * C_value * fs);
            wdf.G = (T) 1.0 / wdf.R;
        }

        /** Sets the voltage of the voltage source, in Volts */
        void setVoltage (T newV)
        {
            v_0 = newV;
        }

        /** Accepts an incident wave into a WDF resistive voltage source. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
            z = wdf.a;
        }

        /** Propogates a reflected wave from a WDF resistive voltage source. */
        inline T reflected() noexcept
        {
            wdf.b = z + v_0 - v_1;
            v_1 = v_0;
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        T C_value = (T) 1.0e-6;

        T z {};
        T v_0 {};
        T v_1 {};

        T fs;
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
            if (all (newR == R_value))
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

    /** WDF Resistor and Capacitor and Voltage source in Series */
    template <typename T>
    class ResistiveCapacitiveVoltageSourceT final : public BaseWDF
    {
    public:
        /** Creates a new WDF Resistor/Capacitor Series.
         * @param cap_value: Resistance value in Ohms
         * @param res_value: Capacitance value in Farads
         * @param fs: WDF sample rate
         */
        explicit ResistiveCapacitiveVoltageSourceT (T res_value, T cap_value, T fs = (T) 48000.0)
            : R_value (res_value),
              C_value (cap_value),
              tt ((T) 1 / fs)
        {
            calcImpedance();
            reset();
        }

        /** Prepares the capacitor to operate at a new sample rate */
        void prepare (T sampleRate)
        {
            tt = (T) 1 / sampleRate;
            propagateImpedanceChange();

            reset();
        }

        /** Resets the capacitor state */
        void reset()
        {
            z = 0.0f;
        }

        /** Sets the resistance value of the WDF resistor, in Ohms. */
        void setResistanceValue (T newR)
        {
            if (all (newR == R_value))
                return;

            R_value = newR;
            propagateImpedanceChange();
        }

        /** Sets the capacitance value of the WDF capacitor, in Farads. */
        void setCapacitanceValue (T newC)
        {
            if (all (newC == C_value))
                return;

            C_value = newC;
            propagateImpedanceChange();
        }

        /** Sets the voltage of the voltage source, in Volts */
        void setVoltage (T newV) { Vs = newV; }

        /** Computes the impedance of the WDF resistor/capacitor combination */
        inline void calcImpedance() override
        {
            wdf.R = tt / ((T) 2.0 * C_value) + R_value;
            wdf.G = (T) 1.0 / wdf.R;
            T_over_2RC = tt / ((T) 2 * C_value * R_value);
        }

        /** Accepts an incident wave into the WDF. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
            z -= T_over_2RC * (wdf.a - wdf.b);
        }

        /** Propogates a reflected wave from the WDF. */
        inline T reflected() noexcept
        {
            wdf.b = -(z + Vs);
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        T Vs = (T) 0.0;
        T R_value = (T) 1.0e3;
        T C_value = (T) 1.0e-6;

        T T_over_2RC = (T) 0.0;

        T z = (T) 0.0;

        T tt;
    };
} // namespace wdft
} // namespace chowdsp

#endif //CHOWDSP_WDF_WDFT_SOURCES_H
