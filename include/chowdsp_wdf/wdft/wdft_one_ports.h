#ifndef CHOWDSP_WDF_WDFT_ONE_PORTS_H
#define CHOWDSP_WDF_WDFT_ONE_PORTS_H

#include "wdft_base.h"

namespace chowdsp
{
namespace wdft
{
    /** WDF Resistor Node */
    template <typename T>
    class ResistorT final : public BaseWDF
    {
    public:
        /** Creates a new WDF Resistor with a given resistance.
         * @param value: resistance in Ohms
         */
        explicit ResistorT (T value) : R_value (value)
        {
            calcImpedance();
        }

        /** Sets the resistance value of the WDF resistor, in Ohms. */
        void setResistanceValue (T newR)
        {
            if (all (newR == R_value))
                return;

            R_value = newR;
            propagateImpedanceChange();
        }

        /** Computes the impedance of the WDF resistor, Z_R = R. */
        inline void calcImpedance() override
        {
            wdf.R = R_value;
            wdf.G = (T) 1.0 / wdf.R;
        }

        /** Accepts an incident wave into a WDF resistor. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
        }

        /** Propogates a reflected wave from a WDF resistor. */
        inline T reflected() noexcept
        {
            wdf.b = 0.0;
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        T R_value = (T) 1.0e-9;
    };

    /** WDF Capacitor Node */
    template <typename T>
    class CapacitorT final : public BaseWDF
    {
    public:
        /** Creates a new WDF Capacitor.
         * @param value: Capacitance value in Farads
         * @param fs: WDF sample rate
         */
        explicit CapacitorT (T value, T fs = (T) 48000.0) : C_value (value),
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

        /** Resets the capacitor state */
        void reset()
        {
            z = (T) 0.0;
        }

        /** Sets the capacitance value of the WDF capacitor, in Farads. */
        void setCapacitanceValue (T newC)
        {
            if (all (newC == C_value))
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

        /** Accepts an incident wave into a WDF capacitor. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
            z = wdf.a;
        }

        /** Propogates a reflected wave from a WDF capacitor. */
        inline T reflected() noexcept
        {
            wdf.b = z;
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        T C_value = (T) 1.0e-6;
        T z = (T) 0.0;

        T fs;
    };

    /** WDF Capacitor Node with alpha transform parameter */
    template <typename T>
    class CapacitorAlphaT final : public BaseWDF
    {
    public:
        /** Creates a new WDF Capacitor.
         * @param value: Capacitance value in Farads
         * @param fs: WDF sample rate
         * @param alpha: alpha value to be used for the alpha transform,
         *             use 0 for Backwards Euler, use 1 for Bilinear Transform.
         */
        explicit CapacitorAlphaT (T value, T fs = (T) 48000.0, T alpha = (T) 1.0) : C_value (value),
                                                                                    fs (fs),
                                                                                    alpha (alpha),
                                                                                    b_coef (((T) 1.0 - alpha) / (T) 2.0),
                                                                                    a_coef (((T) 1.0 + alpha) / (T) 2.0)
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

        /** Resets the capacitor state */
        void reset()
        {
            z = (T) 0.0;
        }

        /** Sets a new alpha value to use for the alpha transform */
        void setAlpha (T newAlpha)
        {
            alpha = newAlpha;
            b_coef = ((T) 1.0 - alpha) / (T) 2.0;
            a_coef = ((T) 1.0 + alpha) / (T) 2.0;

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

        /** Computes the impedance of the WDF capacitor,
         *                 1
         * Z_C = ---------------------
         *       (1 + alpha) * f_s * C
         */
        inline void calcImpedance() override
        {
            wdf.R = (T) 1.0 / (((T) 1.0 + alpha) * C_value * fs);
            wdf.G = (T) 1.0 / wdf.R;
        }

        /** Accepts an incident wave into a WDF capacitor. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
            z = wdf.a;
        }

        /** Propogates a reflected wave from a WDF capacitor. */
        inline T reflected() noexcept
        {
            wdf.b = b_coef * wdf.b + a_coef * z;
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        T C_value = (T) 1.0e-6;
        T z = (T) 0.0;

        T fs;

        T alpha;
        T b_coef;
        T a_coef;
    };

    /** WDF Inductor Node */
    template <typename T>
    class InductorT final : public BaseWDF
    {
    public:
        /** Creates a new WDF Inductor.
         * @param value: Inductance value in Farads
         * @param fs: WDF sample rate
         */
        explicit InductorT (T value, T fs = (T) 48000.0) : L_value (value),
                                                           fs (fs)
        {
            calcImpedance();
        }

        /** Prepares the inductor to operate at a new sample rate */
        void prepare (T sampleRate)
        {
            fs = sampleRate;
            propagateImpedanceChange();

            reset();
        }

        /** Resets the inductor state */
        void reset()
        {
            z = (T) 0.0;
        }

        /** Sets the inductance value of the WDF inductor, in Henries. */
        void setInductanceValue (T newL)
        {
            if (all (newL == L_value))
                return;

            L_value = newL;
            propagateImpedanceChange();
        }

        /** Computes the impedance of the WDF inductor,
         * Z_L = 2 * f_s * L
         */
        inline void calcImpedance() override
        {
            wdf.R = (T) 2.0 * L_value * fs;
            wdf.G = (T) 1.0 / wdf.R;
        }

        /** Accepts an incident wave into a WDF inductor. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
            z = wdf.a;
        }

        /** Propogates a reflected wave from a WDF inductor. */
        inline T reflected() noexcept
        {
            wdf.b = -z;
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        T L_value = (T) 1.0e-6;
        T z = (T) 0.0;

        T fs;
    };

    /** WDF Inductor Node with alpha transform parameter */
    template <typename T>
    class InductorAlphaT final : public BaseWDF
    {
    public:
        /** Creates a new WDF Inductor.
         * @param value: Inductance value in Farads
         * @param fs: WDF sample rate
         * @param alpha: alpha value to be used for the alpha transform,
         *               use 0 for Backwards Euler, use 1 for Bilinear Transform.
         */
        explicit InductorAlphaT (T value, T fs = (T) 48000.0, T alpha = (T) 1.0) : L_value (value),
                                                                                   fs (fs),
                                                                                   alpha (alpha),
                                                                                   b_coef (((T) 1.0 - alpha) / (T) 2.0),
                                                                                   a_coef (((T) 1.0 + alpha) / (T) 2.0)
        {
            calcImpedance();
        }

        /** Prepares the inductor to operate at a new sample rate */
        void prepare (T sampleRate)
        {
            fs = sampleRate;
            propagateImpedanceChange();

            reset();
        }

        /** Resets the inductor state */
        void reset()
        {
            z = (T) 0.0;
        }

        /** Sets a new alpha value to use for the alpha transform */
        void setAlpha (T newAlpha)
        {
            alpha = newAlpha;
            b_coef = ((T) 1.0 - alpha) / (T) 2.0;
            a_coef = ((T) 1.0 + alpha) / (T) 2.0;

            propagateImpedanceChange();
        }

        /** Sets the inductance value of the WDF inductor, in Henries. */
        void setInductanceValue (T newL)
        {
            if (all (newL == L_value))
                return;

            L_value = newL;
            propagateImpedanceChange();
        }

        /** Computes the impedance of the WDF inductor,
         * Z_L = (1 + alpha) * f_s * L
         */
        inline void calcImpedance() override
        {
            wdf.R = ((T) 1.0 + alpha) * L_value * fs;
            wdf.G = (T) 1.0 / wdf.R;
        }

        /** Accepts an incident wave into a WDF inductor. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
            z = wdf.a;
        }

        /** Propogates a reflected wave from a WDF inductor. */
        inline T reflected() noexcept
        {
            wdf.b = b_coef * wdf.b - a_coef * z;
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        T L_value = (T) 1.0e-6;
        T z = (T) 0.0;

        T fs;

        T alpha;
        T b_coef;
        T a_coef;
    };

    /** WDF Resistor and Capacitor in Series */
    template <typename T>
    class ResistorCapacitorSeriesT final : public BaseWDF
    {
    public:
        /** Creates a new WDF Resistor/Capacitor Series.
         * @param cap_value: Resistance value in Ohms
         * @param res_value: Capacitance value in Farads
         * @param fs: WDF sample rate
         */
        explicit ResistorCapacitorSeriesT (T res_value, T cap_value, T fs = (T) 48000.0)
            : R_value (res_value),
              C_value (cap_value),
              tt ((T) 1 / fs)
        {
            calcImpedance();
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
            z = (T) 0.0;
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

        /** Computes the impedance of the WDF resistor/capacitor combination */
        inline void calcImpedance() override
        {
            wdf.R = tt / ((T) 2.0 * C_value) + R_value;
            wdf.G = (T) 1.0 / wdf.R;
            T_over_T_plus_2RC = tt / ((T) 2 * C_value * R_value + tt);
        }

        /** Accepts an incident wave into the WDF. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
            z = wdf.a;
        }

        /** Propogates a reflected wave from the WDF. */
        inline T reflected() noexcept
        {
            wdf.b += T_over_T_plus_2RC * (z - wdf.b);
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        T R_value = (T) 1.0e3;
        T C_value = (T) 1.0e-6;

        T T_over_T_plus_2RC = (T) 0.0;

        T z = (T) 0.0;

        T tt;
    };

    /** WDF Resistor and Capacitor in parallel */
    template <typename T>
    class ResistorCapacitorParallelT final : public BaseWDF
    {
    public:
        /** Creates a new WDF Resistor/Capacitor Parallel.
         * @param res_value: Resistance value in Ohms
         * @param cap_value: Capacitance value in Farads
         * @param fs: WDF sample rate
         */
        explicit ResistorCapacitorParallelT (T res_value, T cap_value, T fs = (T) 48000.0)
            : R_value (res_value),
              C_value (cap_value),
              tt ((T) 1 / fs)
        {
            calcImpedance();
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
            z = (T) 0.0;
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

        /** Computes the impedance of the WDF resistor/capacitor combination */
        inline void calcImpedance() override
        {
            const auto twoRC = (T) 2.0 * C_value * R_value;
            wdf.R = R_value * tt / (twoRC + tt);
            wdf.G = (T) 1.0 / wdf.R;
            twoRC_over_twoRC_plus_T = twoRC / (twoRC + tt);
        }

        /** Accepts an incident wave into the WDF. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
            z = wdf.a;
        }

        /** Propogates a reflected wave from the WDF. */
        inline T reflected() noexcept
        {
            wdf.b = twoRC_over_twoRC_plus_T * (wdf.b + z) - wdf.b;
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        T R_value = (T) 1.0e3;
        T C_value = (T) 1.0e-6;

        T twoRC_over_twoRC_plus_T = (T) 0.0;

        T z = (T) 0.0;

        T tt;
    };
} // namespace wdft
} // namespace chowdsp

#endif //CHOWDSP_WDF_WDFT_ONE_PORTS_H
