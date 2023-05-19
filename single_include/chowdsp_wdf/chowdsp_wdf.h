#pragma once

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244 4464 4514 4324)
#endif

// define maybe_unused if we can
#if __cplusplus >= 201703L
#define CHOWDSP_WDF_MAYBE_UNUSED [[maybe_unused]]
#else
#define CHOWDSP_WDF_MAYBE_UNUSED
#endif

// Define a default SIMD alignment
#if defined(XSIMD_HPP)
constexpr auto CHOWDSP_WDF_DEFAULT_SIMD_ALIGNMENT = (int) xsimd::default_arch::alignment();
#else
constexpr int CHOWDSP_WDF_DEFAULT_SIMD_ALIGNMENT = 16;
#endif

// #include "wdft/wdft.h"
#ifndef CHOWDSP_WDF_T_INCLUDED
#define CHOWDSP_WDF_T_INCLUDED

namespace chowdsp
{
/** API for constructing Wave Digital Filters with a fixed compile-time architecture */
namespace wdft
{
}

} // namespace chowdsp

// #include "wdft_one_ports.h"
#ifndef CHOWDSP_WDF_WDFT_ONE_PORTS_H
#define CHOWDSP_WDF_WDFT_ONE_PORTS_H

// #include "wdft_base.h"
#ifndef CHOWDSP_WDF_WDFT_BASE_H
#define CHOWDSP_WDF_WDFT_BASE_H

// #include "../math/sample_type.h"
#ifndef CHOWDSP_WDF_SAMPLE_TYPE_H
#define CHOWDSP_WDF_SAMPLE_TYPE_H

#include <type_traits>

#ifndef DOXYGEN

namespace chowdsp
{
#if ! (JUCE_MODULE_AVAILABLE_chowdsp_dsp)
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
#endif

/** Type alias for a SIMD numeric type */
template <typename T>
using NumericType = typename SampleTypeHelpers::ElementType<T>::Type;

/** Returns true if all the elements in a SIMD vector are equal */
inline bool all (bool x)
{
    return x;
}

/** Ternary select operation */
template <typename T>
inline T select (bool b, const T& t, const T& f)
{
    return b ? t : f;
}
} // namespace chowdsp

#endif // DOXYGEN

#endif //CHOWDSP_WDF_SAMPLE_TYPE_H


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
            wdf.a = (T) 0;
            wdf.b = (T) 0;
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
            z -= T_over_T_plus_2RC * (wdf.a + z);
        }

        /** Propogates a reflected wave from the WDF. */
        inline T reflected() noexcept
        {
            wdf.b = -z;
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
            wdf.a = (T) 0;
            wdf.b = (T) 0;
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
            z = wdf.b + wdf.a - z;
        }

        /** Propogates a reflected wave from the WDF. */
        inline T reflected() noexcept
        {
            wdf.b = twoRC_over_twoRC_plus_T * z;
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

// #include "wdft_sources.h"
#ifndef CHOWDSP_WDF_WDFT_SOURCES_H
#define CHOWDSP_WDF_WDFT_SOURCES_H

// #include "wdft_base.h"


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

// #include "wdft_adaptors.h"
#ifndef CHOWDSP_WDF_WDFT_ADAPTORS_H
#define CHOWDSP_WDF_WDFT_ADAPTORS_H

// #include "wdft_base.h"


namespace chowdsp
{
namespace wdft
{
    /** WDF 3-port parallel adaptor */
    template <typename T, typename Port1Type, typename Port2Type>
    class WDFParallelT final : public BaseWDF
    {
    public:
        /** Creates a new WDF parallel adaptor from two connected ports. */
        WDFParallelT (Port1Type& p1, Port2Type& p2) : port1 (p1),
                                                      port2 (p2)
        {
            port1.connectToParent (this);
            port2.connectToParent (this);
            calcImpedance();
        }

        /** Computes the impedance for a WDF parallel adaptor.
         *  1     1     1
         * --- = --- + ---
         * Z_p   Z_1   Z_2
         */
        inline void calcImpedance() override
        {
            wdf.G = port1.wdf.G + port2.wdf.G;
            wdf.R = (T) 1.0 / wdf.G;
            port1Reflect = port1.wdf.G / wdf.G;
        }

        /** Accepts an incident wave into a WDF parallel adaptor. */
        inline void incident (T x) noexcept
        {
            const auto b2 = wdf.b - port2.wdf.b + x;
            port1.incident (b2 + bDiff);
            port2.incident (b2);

            wdf.a = x;
        }

        /** Propogates a reflected wave from a WDF parallel adaptor. */
        inline T reflected() noexcept
        {
            port1.reflected();
            port2.reflected();

            bDiff = port2.wdf.b - port1.wdf.b;
            wdf.b = port2.wdf.b - port1Reflect * bDiff;

            return wdf.b;
        }

        Port1Type& port1;
        Port2Type& port2;

        WDFMembers<T> wdf;

    private:
        T port1Reflect = (T) 1.0;
        T bDiff = (T) 0.0;
    };

    /** WDF 3-port series adaptor */
    template <typename T, typename Port1Type, typename Port2Type>
    class WDFSeriesT final : public BaseWDF
    {
    public:
        /** Creates a new WDF series adaptor from two connected ports. */
        WDFSeriesT (Port1Type& p1, Port2Type& p2) : port1 (p1),
                                                    port2 (p2)
        {
            port1.connectToParent (this);
            port2.connectToParent (this);
            calcImpedance();
        }

        /** Computes the impedance for a WDF parallel adaptor.
         * Z_s = Z_1 + Z_2
         */
        inline void calcImpedance() override
        {
            wdf.R = port1.wdf.R + port2.wdf.R;
            wdf.G = (T) 1.0 / wdf.R;
            port1Reflect = port1.wdf.R / wdf.R;
        }

        /** Accepts an incident wave into a WDF series adaptor. */
        inline void incident (T x) noexcept
        {
            const auto b1 = port1.wdf.b - port1Reflect * (x + port1.wdf.b + port2.wdf.b);
            port1.incident (b1);
            port2.incident (-(x + b1));

            wdf.a = x;
        }

        /** Propogates a reflected wave from a WDF series adaptor. */
        inline T reflected() noexcept
        {
            wdf.b = -(port1.reflected() + port2.reflected());
            return wdf.b;
        }

        Port1Type& port1;
        Port2Type& port2;

        WDFMembers<T> wdf;

    private:
        T port1Reflect = (T) 1.0;
    };

    /** WDF Voltage Polarity Inverter */
    template <typename T, typename PortType>
    class PolarityInverterT final : public BaseWDF
    {
    public:
        /** Creates a new WDF polarity inverter */
        explicit PolarityInverterT (PortType& p) : port1 (p)
        {
            port1.connectToParent (this);
            calcImpedance();
        }

        /** Calculates the impedance of the WDF inverter
         * (same impedance as the connected node).
         */
        inline void calcImpedance() override
        {
            wdf.R = port1.wdf.R;
            wdf.G = (T) 1.0 / wdf.R;
        }

        /** Accepts an incident wave into a WDF inverter. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
            port1.incident (-x);
        }

        /** Propogates a reflected wave from a WDF inverter. */
        inline T reflected() noexcept
        {
            wdf.b = -port1.reflected();
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        PortType& port1;
    };

    /** WDF y-parameter 2-port (short circuit admittance) */
    template <typename T, typename PortType>
    class YParameterT final : public BaseWDF
    {
    public:
        /** Creates a new WDF Y-Parameter, with the given coefficients */
        YParameterT (PortType& port1, T y11, T y12, T y21, T y22) : port1 (port1)
        {
            y[0][0] = y11;
            y[0][1] = y12;
            y[1][0] = y21;
            y[1][1] = y22;

            port1.connectToParent (this);
            calcImpedance();
        }

        /** Calculates the impedance of the WDF Y-Parameter */
        inline void calcImpedance() override
        {
            denominator = y[1][1] + port1.wdf.R * y[0][0] * y[1][1] - port1.wdf.R * y[0][1] * y[1][0];
            wdf.R = (port1.wdf.R * y[0][0] + (T) 1.0) / denominator;
            wdf.G = (T) 1.0 / wdf.R;

            T rSq = port1.wdf.R * port1.wdf.R;
            T num1A = -y[1][1] * rSq * y[0][0] * y[0][0];
            T num2A = y[0][1] * y[1][0] * rSq * y[0][0];

            A = (num1A + num2A + y[1][1]) / (denominator * (port1.wdf.R * y[0][0] + (T) 1.0));
            B = -port1.wdf.R * y[0][1] / (port1.wdf.R * y[0][0] + (T) 1.0);
            C = -y[1][0] / denominator;
        }

        /** Accepts an incident wave into a WDF Y-Parameter. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
            port1.incident (A * port1.wdf.b + B * x);
        }

        /** Propogates a reflected wave from a WDF Y-Parameter. */
        inline T reflected() noexcept
        {
            wdf.b = C * port1.reflected();
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        PortType& port1;
        T y[2][2] = { { (T) 0.0, (T) 0.0 }, { (T) 0.0, (T) 0.0 } };

        T denominator = (T) 1.0;
        T A = (T) 1.0;
        T B = (T) 1.0;
        T C = (T) 1.0;
    };

    // useful "factory" functions so you don't have to declare all the template parameters

    /** Factory method for creating a parallel adaptor between two elements. */
    template <typename T, typename P1Type, typename P2Type>
    CHOWDSP_WDF_MAYBE_UNUSED WDFParallelT<T, P1Type, P2Type> makeParallel (P1Type& p1, P2Type& p2)
    {
        return WDFParallelT<T, P1Type, P2Type> (p1, p2);
    }

    /** Factory method for creating a series adaptor between two elements. */
    template <typename T, typename P1Type, typename P2Type>
    CHOWDSP_WDF_MAYBE_UNUSED WDFSeriesT<T, P1Type, P2Type> makeSeries (P1Type& p1, P2Type& p2)
    {
        return WDFSeriesT<T, P1Type, P2Type> (p1, p2);
    }

    /** Factory method for creating a polarity inverter. */
    template <typename T, typename PType>
    CHOWDSP_WDF_MAYBE_UNUSED PolarityInverterT<T, PType> makeInverter (PType& p1)
    {
        return PolarityInverterT<T, PType> (p1);
    }
} // namespace wdft
} // namespace chowdsp

#endif //CHOWDSP_WDF_WDFT_ADAPTORS_H

// #include "wdft_nonlinearities.h"
#ifndef CHOWDSP_WDF_WDFT_NONLINEARITIES_H
#define CHOWDSP_WDF_WDFT_NONLINEARITIES_H

#include <cmath>

// #include "wdft_base.h"


// #include "../math/signum.h"


namespace chowdsp
{
/** Methods for implementing the signum function */
namespace signum
{
    /** Signum function to determine the sign of the input. */
    template <typename T>
    inline int signum (T val)
    {
        return (T (0) < val) - (val < T (0));
    }

#if defined(XSIMD_HPP)
    /** Signum function to determine the sign of the input. */
    template <typename T>
    inline xsimd::batch<T> signum (xsimd::batch<T> val)
    {
        using v_type = xsimd::batch<T>;
        const auto positive = xsimd::select (val > v_type ((T) 0), v_type ((T) 1), v_type ((T) 0));
        const auto negative = xsimd::select (val < v_type ((T) 0), v_type ((T) 1), v_type ((T) 0));
        return positive - negative;
    }
#endif

} // namespace signum
} // namespace chowdsp

// #include "../math/omega.h"
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
// #include "sample_type.h"


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


namespace chowdsp
{
namespace wdft
{
    /** Enum to determine which diode approximation eqn. to use */
    enum DiodeQuality
    {
        Good, // see reference eqn (18)
        Best, // see reference eqn (39)
    };

    /**
     * WDF diode pair (non-adaptable)
     * See Werner et al., "An Improved and Generalized Diode Clipper Model for Wave Digital Filters"
     * https://www.researchgate.net/publication/299514713_An_Improved_and_Generalized_Diode_Clipper_Model_for_Wave_Digital_Filters
     */
    template <typename T, typename Next, DiodeQuality Quality = DiodeQuality::Best>
    class DiodePairT final : public RootWDF
    {
    public:
        /**
         * Creates a new WDF diode pair, with the given diode specifications.
         * @param n: the next element in the WDF connection tree
         * @param Is: reverse saturation current
         * @param Vt: thermal voltage
         * @param nDiodes: the number of series diodes
         */
        DiodePairT (Next& n, T Is, T Vt = NumericType<T> (25.85e-3), T nDiodes = 1) : next (n)
        {
            n.connectToParent (this);
            setDiodeParameters (Is, Vt, nDiodes);
        }

        /** Sets diode specific parameters */
        void setDiodeParameters (T newIs, T newVt, T nDiodes)
        {
            Is = newIs;
            Vt = nDiodes * newVt;
            twoVt = (T) 2 * Vt;
            oneOverVt = (T) 1 / Vt;
            calcImpedance();
        }

        inline void calcImpedance() override
        {
#if defined(XSIMD_HPP)
            using xsimd::log;
#endif
            using std::log;

            R_Is = next.wdf.R * Is;
            R_Is_overVt = R_Is * oneOverVt;
            logR_Is_overVt = log (R_Is_overVt);
        }

        /** Accepts an incident wave into a WDF diode pair. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
        }

        /** Propogates a reflected wave from a WDF diode pair. */
        inline T reflected() noexcept
        {
            reflectedInternal();
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        /** Implementation for float/double (Good). */
        template <typename C = T, DiodeQuality Q = Quality>
        inline typename std::enable_if<Q == Good, void>::type
            reflectedInternal() noexcept
        {
            // See eqn (18) from reference paper
            T lambda = (T) signum::signum (wdf.a);
            wdf.b = wdf.a + (T) 2 * lambda * (R_Is - Vt * Omega::omega4 (logR_Is_overVt + lambda * wdf.a * oneOverVt + R_Is_overVt));
        }

        /** Implementation for float/double (Best). */
        template <typename C = T, DiodeQuality Q = Quality>
        inline typename std::enable_if<Q == Best, void>::type
            reflectedInternal() noexcept
        {
            // See eqn (39) from reference paper
            T lambda = (T) signum::signum (wdf.a);
            T lambda_a_over_vt = lambda * wdf.a * oneOverVt;
            wdf.b = wdf.a - twoVt * lambda * (Omega::omega4 (logR_Is_overVt + lambda_a_over_vt) - Omega::omega4 (logR_Is_overVt - lambda_a_over_vt));
        }

        T Is; // reverse saturation current
        T Vt; // thermal voltage

        // pre-computed vars
        T twoVt;
        T oneOverVt;
        T R_Is;
        T R_Is_overVt;
        T logR_Is_overVt;

        const Next& next;
    };

    /**
     * WDF diode (non-adaptable)
     * See Werner et al., "An Improved and Generalized Diode Clipper Model for Wave Digital Filters"
     * https://www.researchgate.net/publication/299514713_An_Improved_and_Generalized_Diode_Clipper_Model_for_Wave_Digital_Filters
     */
    template <typename T, typename Next, DiodeQuality Quality = DiodeQuality::Best>
    class DiodeT final : public RootWDF
    {
    public:
        /**
         * Creates a new WDF diode, with the given diode specifications.
         * @param n: the next element in the WDF connection tree
         * @param Is: reverse saturation current
         * @param Vt: thermal voltage
         * @param nDiodes: the number of series diodes
         */
        DiodeT (Next& n, T Is, T Vt = NumericType<T> (25.85e-3), T nDiodes = 1) : next (n)
        {
            n.connectToParent (this);
            setDiodeParameters (Is, Vt, nDiodes);
        }

        /** Sets diode specific parameters */
        void setDiodeParameters (T newIs, T newVt, T nDiodes)
        {
            Is = newIs;
            Vt = nDiodes * newVt;
            twoVt = (T) 2 * Vt;
            oneOverVt = (T) 1 / Vt;
            calcImpedance();
        }

        inline void calcImpedance() override
        {
#if defined(XSIMD_HPP)
            using xsimd::log;
#endif
            using std::log;

            twoR_Is = (T) 2 * next.wdf.R * Is;
            R_Is_overVt = next.wdf.R * Is * oneOverVt;
            logR_Is_overVt = log (R_Is_overVt);
        }

        /** Accepts an incident wave into a WDF diode. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
        }

        /** Propogates a reflected wave from a WDF diode. */
        inline T reflected() noexcept
        {
            // See eqn (10) from reference paper
            wdf.b = wdf.a + twoR_Is - twoVt * Omega::omega4 (logR_Is_overVt + wdf.a * oneOverVt + R_Is_overVt);
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        T Is; // reverse saturation current
        T Vt; // thermal voltage

        // pre-computed vars
        T twoVt;
        T oneOverVt;
        T twoR_Is;
        T R_Is_overVt;
        T logR_Is_overVt;

        const Next& next;
    };

    /** WDF Switch (non-adaptable) */
    template <typename T, typename Next>
    class SwitchT final : public RootWDF
    {
    public:
        explicit SwitchT (Next& next)
        {
            next.connectToParent (this);
        }

        inline void calcImpedance() override {}

        /** Sets the state of the switch. */
        void setClosed (bool shouldClose) { closed = shouldClose; }

        /** Accepts an incident wave into a WDF switch. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
        }

        /** Propogates a reflected wave from a WDF switch. */
        inline T reflected() noexcept
        {
            wdf.b = closed ? -wdf.a : wdf.a;
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        bool closed = true;
    };
} // namespace wdft
} // namespace chowdsp

#endif //CHOWDSP_WDF_WDFT_NONLINEARITIES_H


#endif // CHOWDSP_WDF_T_INCLUDED

// #include "wdf/wdf.h"
#ifndef CHOWDSP_WDF_H_INCLUDED
#define CHOWDSP_WDF_H_INCLUDED

namespace chowdsp
{
/** API for constructing Wave Digital Filters with run-time flexibility */
namespace wdf
{
}

} // namespace chowdsp

// #include "wdf_one_ports.h"
#ifndef CHOWDSP_WDF_WDF_ONE_PORTS_H
#define CHOWDSP_WDF_WDF_ONE_PORTS_H

// #include "wdf_base.h"
#ifndef CHOWDSP_WDF_WDF_BASE_H
#define CHOWDSP_WDF_WDF_BASE_H

#include <string>
#include <utility>

// #include "../wdft/wdft.h"
#ifndef CHOWDSP_WDF_T_INCLUDED
#define CHOWDSP_WDF_T_INCLUDED

namespace chowdsp
{
/** API for constructing Wave Digital Filters with a fixed compile-time architecture */
namespace wdft
{
}

} // namespace chowdsp

// #include "wdft_one_ports.h"
#ifndef CHOWDSP_WDF_WDFT_ONE_PORTS_H
#define CHOWDSP_WDF_WDFT_ONE_PORTS_H

// #include "wdft_base.h"
#ifndef CHOWDSP_WDF_WDFT_BASE_H
#define CHOWDSP_WDF_WDFT_BASE_H

// #include "../math/sample_type.h"
#ifndef CHOWDSP_WDF_SAMPLE_TYPE_H
#define CHOWDSP_WDF_SAMPLE_TYPE_H

#include <type_traits>

#ifndef DOXYGEN

namespace chowdsp
{
#if ! (JUCE_MODULE_AVAILABLE_chowdsp_dsp)
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
#endif

/** Type alias for a SIMD numeric type */
template <typename T>
using NumericType = typename SampleTypeHelpers::ElementType<T>::Type;

/** Returns true if all the elements in a SIMD vector are equal */
inline bool all (bool x)
{
    return x;
}

/** Ternary select operation */
template <typename T>
inline T select (bool b, const T& t, const T& f)
{
    return b ? t : f;
}
} // namespace chowdsp

#endif // DOXYGEN

#endif //CHOWDSP_WDF_SAMPLE_TYPE_H


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
            wdf.a = (T) 0;
            wdf.b = (T) 0;
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
            z -= T_over_T_plus_2RC * (wdf.a + z);
        }

        /** Propogates a reflected wave from the WDF. */
        inline T reflected() noexcept
        {
            wdf.b = -z;
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
            wdf.a = (T) 0;
            wdf.b = (T) 0;
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
            z = wdf.b + wdf.a - z;
        }

        /** Propogates a reflected wave from the WDF. */
        inline T reflected() noexcept
        {
            wdf.b = twoRC_over_twoRC_plus_T * z;
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

// #include "wdft_sources.h"
#ifndef CHOWDSP_WDF_WDFT_SOURCES_H
#define CHOWDSP_WDF_WDFT_SOURCES_H

// #include "wdft_base.h"


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

// #include "wdft_adaptors.h"
#ifndef CHOWDSP_WDF_WDFT_ADAPTORS_H
#define CHOWDSP_WDF_WDFT_ADAPTORS_H

// #include "wdft_base.h"


namespace chowdsp
{
namespace wdft
{
    /** WDF 3-port parallel adaptor */
    template <typename T, typename Port1Type, typename Port2Type>
    class WDFParallelT final : public BaseWDF
    {
    public:
        /** Creates a new WDF parallel adaptor from two connected ports. */
        WDFParallelT (Port1Type& p1, Port2Type& p2) : port1 (p1),
                                                      port2 (p2)
        {
            port1.connectToParent (this);
            port2.connectToParent (this);
            calcImpedance();
        }

        /** Computes the impedance for a WDF parallel adaptor.
         *  1     1     1
         * --- = --- + ---
         * Z_p   Z_1   Z_2
         */
        inline void calcImpedance() override
        {
            wdf.G = port1.wdf.G + port2.wdf.G;
            wdf.R = (T) 1.0 / wdf.G;
            port1Reflect = port1.wdf.G / wdf.G;
        }

        /** Accepts an incident wave into a WDF parallel adaptor. */
        inline void incident (T x) noexcept
        {
            const auto b2 = wdf.b - port2.wdf.b + x;
            port1.incident (b2 + bDiff);
            port2.incident (b2);

            wdf.a = x;
        }

        /** Propogates a reflected wave from a WDF parallel adaptor. */
        inline T reflected() noexcept
        {
            port1.reflected();
            port2.reflected();

            bDiff = port2.wdf.b - port1.wdf.b;
            wdf.b = port2.wdf.b - port1Reflect * bDiff;

            return wdf.b;
        }

        Port1Type& port1;
        Port2Type& port2;

        WDFMembers<T> wdf;

    private:
        T port1Reflect = (T) 1.0;
        T bDiff = (T) 0.0;
    };

    /** WDF 3-port series adaptor */
    template <typename T, typename Port1Type, typename Port2Type>
    class WDFSeriesT final : public BaseWDF
    {
    public:
        /** Creates a new WDF series adaptor from two connected ports. */
        WDFSeriesT (Port1Type& p1, Port2Type& p2) : port1 (p1),
                                                    port2 (p2)
        {
            port1.connectToParent (this);
            port2.connectToParent (this);
            calcImpedance();
        }

        /** Computes the impedance for a WDF parallel adaptor.
         * Z_s = Z_1 + Z_2
         */
        inline void calcImpedance() override
        {
            wdf.R = port1.wdf.R + port2.wdf.R;
            wdf.G = (T) 1.0 / wdf.R;
            port1Reflect = port1.wdf.R / wdf.R;
        }

        /** Accepts an incident wave into a WDF series adaptor. */
        inline void incident (T x) noexcept
        {
            const auto b1 = port1.wdf.b - port1Reflect * (x + port1.wdf.b + port2.wdf.b);
            port1.incident (b1);
            port2.incident (-(x + b1));

            wdf.a = x;
        }

        /** Propogates a reflected wave from a WDF series adaptor. */
        inline T reflected() noexcept
        {
            wdf.b = -(port1.reflected() + port2.reflected());
            return wdf.b;
        }

        Port1Type& port1;
        Port2Type& port2;

        WDFMembers<T> wdf;

    private:
        T port1Reflect = (T) 1.0;
    };

    /** WDF Voltage Polarity Inverter */
    template <typename T, typename PortType>
    class PolarityInverterT final : public BaseWDF
    {
    public:
        /** Creates a new WDF polarity inverter */
        explicit PolarityInverterT (PortType& p) : port1 (p)
        {
            port1.connectToParent (this);
            calcImpedance();
        }

        /** Calculates the impedance of the WDF inverter
         * (same impedance as the connected node).
         */
        inline void calcImpedance() override
        {
            wdf.R = port1.wdf.R;
            wdf.G = (T) 1.0 / wdf.R;
        }

        /** Accepts an incident wave into a WDF inverter. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
            port1.incident (-x);
        }

        /** Propogates a reflected wave from a WDF inverter. */
        inline T reflected() noexcept
        {
            wdf.b = -port1.reflected();
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        PortType& port1;
    };

    /** WDF y-parameter 2-port (short circuit admittance) */
    template <typename T, typename PortType>
    class YParameterT final : public BaseWDF
    {
    public:
        /** Creates a new WDF Y-Parameter, with the given coefficients */
        YParameterT (PortType& port1, T y11, T y12, T y21, T y22) : port1 (port1)
        {
            y[0][0] = y11;
            y[0][1] = y12;
            y[1][0] = y21;
            y[1][1] = y22;

            port1.connectToParent (this);
            calcImpedance();
        }

        /** Calculates the impedance of the WDF Y-Parameter */
        inline void calcImpedance() override
        {
            denominator = y[1][1] + port1.wdf.R * y[0][0] * y[1][1] - port1.wdf.R * y[0][1] * y[1][0];
            wdf.R = (port1.wdf.R * y[0][0] + (T) 1.0) / denominator;
            wdf.G = (T) 1.0 / wdf.R;

            T rSq = port1.wdf.R * port1.wdf.R;
            T num1A = -y[1][1] * rSq * y[0][0] * y[0][0];
            T num2A = y[0][1] * y[1][0] * rSq * y[0][0];

            A = (num1A + num2A + y[1][1]) / (denominator * (port1.wdf.R * y[0][0] + (T) 1.0));
            B = -port1.wdf.R * y[0][1] / (port1.wdf.R * y[0][0] + (T) 1.0);
            C = -y[1][0] / denominator;
        }

        /** Accepts an incident wave into a WDF Y-Parameter. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
            port1.incident (A * port1.wdf.b + B * x);
        }

        /** Propogates a reflected wave from a WDF Y-Parameter. */
        inline T reflected() noexcept
        {
            wdf.b = C * port1.reflected();
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        PortType& port1;
        T y[2][2] = { { (T) 0.0, (T) 0.0 }, { (T) 0.0, (T) 0.0 } };

        T denominator = (T) 1.0;
        T A = (T) 1.0;
        T B = (T) 1.0;
        T C = (T) 1.0;
    };

    // useful "factory" functions so you don't have to declare all the template parameters

    /** Factory method for creating a parallel adaptor between two elements. */
    template <typename T, typename P1Type, typename P2Type>
    CHOWDSP_WDF_MAYBE_UNUSED WDFParallelT<T, P1Type, P2Type> makeParallel (P1Type& p1, P2Type& p2)
    {
        return WDFParallelT<T, P1Type, P2Type> (p1, p2);
    }

    /** Factory method for creating a series adaptor between two elements. */
    template <typename T, typename P1Type, typename P2Type>
    CHOWDSP_WDF_MAYBE_UNUSED WDFSeriesT<T, P1Type, P2Type> makeSeries (P1Type& p1, P2Type& p2)
    {
        return WDFSeriesT<T, P1Type, P2Type> (p1, p2);
    }

    /** Factory method for creating a polarity inverter. */
    template <typename T, typename PType>
    CHOWDSP_WDF_MAYBE_UNUSED PolarityInverterT<T, PType> makeInverter (PType& p1)
    {
        return PolarityInverterT<T, PType> (p1);
    }
} // namespace wdft
} // namespace chowdsp

#endif //CHOWDSP_WDF_WDFT_ADAPTORS_H

// #include "wdft_nonlinearities.h"
#ifndef CHOWDSP_WDF_WDFT_NONLINEARITIES_H
#define CHOWDSP_WDF_WDFT_NONLINEARITIES_H

#include <cmath>

// #include "wdft_base.h"


// #include "../math/signum.h"


namespace chowdsp
{
/** Methods for implementing the signum function */
namespace signum
{
    /** Signum function to determine the sign of the input. */
    template <typename T>
    inline int signum (T val)
    {
        return (T (0) < val) - (val < T (0));
    }

#if defined(XSIMD_HPP)
    /** Signum function to determine the sign of the input. */
    template <typename T>
    inline xsimd::batch<T> signum (xsimd::batch<T> val)
    {
        using v_type = xsimd::batch<T>;
        const auto positive = xsimd::select (val > v_type ((T) 0), v_type ((T) 1), v_type ((T) 0));
        const auto negative = xsimd::select (val < v_type ((T) 0), v_type ((T) 1), v_type ((T) 0));
        return positive - negative;
    }
#endif

} // namespace signum
} // namespace chowdsp

// #include "../math/omega.h"
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
// #include "sample_type.h"


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


namespace chowdsp
{
namespace wdft
{
    /** Enum to determine which diode approximation eqn. to use */
    enum DiodeQuality
    {
        Good, // see reference eqn (18)
        Best, // see reference eqn (39)
    };

    /**
     * WDF diode pair (non-adaptable)
     * See Werner et al., "An Improved and Generalized Diode Clipper Model for Wave Digital Filters"
     * https://www.researchgate.net/publication/299514713_An_Improved_and_Generalized_Diode_Clipper_Model_for_Wave_Digital_Filters
     */
    template <typename T, typename Next, DiodeQuality Quality = DiodeQuality::Best>
    class DiodePairT final : public RootWDF
    {
    public:
        /**
         * Creates a new WDF diode pair, with the given diode specifications.
         * @param n: the next element in the WDF connection tree
         * @param Is: reverse saturation current
         * @param Vt: thermal voltage
         * @param nDiodes: the number of series diodes
         */
        DiodePairT (Next& n, T Is, T Vt = NumericType<T> (25.85e-3), T nDiodes = 1) : next (n)
        {
            n.connectToParent (this);
            setDiodeParameters (Is, Vt, nDiodes);
        }

        /** Sets diode specific parameters */
        void setDiodeParameters (T newIs, T newVt, T nDiodes)
        {
            Is = newIs;
            Vt = nDiodes * newVt;
            twoVt = (T) 2 * Vt;
            oneOverVt = (T) 1 / Vt;
            calcImpedance();
        }

        inline void calcImpedance() override
        {
#if defined(XSIMD_HPP)
            using xsimd::log;
#endif
            using std::log;

            R_Is = next.wdf.R * Is;
            R_Is_overVt = R_Is * oneOverVt;
            logR_Is_overVt = log (R_Is_overVt);
        }

        /** Accepts an incident wave into a WDF diode pair. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
        }

        /** Propogates a reflected wave from a WDF diode pair. */
        inline T reflected() noexcept
        {
            reflectedInternal();
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        /** Implementation for float/double (Good). */
        template <typename C = T, DiodeQuality Q = Quality>
        inline typename std::enable_if<Q == Good, void>::type
            reflectedInternal() noexcept
        {
            // See eqn (18) from reference paper
            T lambda = (T) signum::signum (wdf.a);
            wdf.b = wdf.a + (T) 2 * lambda * (R_Is - Vt * Omega::omega4 (logR_Is_overVt + lambda * wdf.a * oneOverVt + R_Is_overVt));
        }

        /** Implementation for float/double (Best). */
        template <typename C = T, DiodeQuality Q = Quality>
        inline typename std::enable_if<Q == Best, void>::type
            reflectedInternal() noexcept
        {
            // See eqn (39) from reference paper
            T lambda = (T) signum::signum (wdf.a);
            T lambda_a_over_vt = lambda * wdf.a * oneOverVt;
            wdf.b = wdf.a - twoVt * lambda * (Omega::omega4 (logR_Is_overVt + lambda_a_over_vt) - Omega::omega4 (logR_Is_overVt - lambda_a_over_vt));
        }

        T Is; // reverse saturation current
        T Vt; // thermal voltage

        // pre-computed vars
        T twoVt;
        T oneOverVt;
        T R_Is;
        T R_Is_overVt;
        T logR_Is_overVt;

        const Next& next;
    };

    /**
     * WDF diode (non-adaptable)
     * See Werner et al., "An Improved and Generalized Diode Clipper Model for Wave Digital Filters"
     * https://www.researchgate.net/publication/299514713_An_Improved_and_Generalized_Diode_Clipper_Model_for_Wave_Digital_Filters
     */
    template <typename T, typename Next, DiodeQuality Quality = DiodeQuality::Best>
    class DiodeT final : public RootWDF
    {
    public:
        /**
         * Creates a new WDF diode, with the given diode specifications.
         * @param n: the next element in the WDF connection tree
         * @param Is: reverse saturation current
         * @param Vt: thermal voltage
         * @param nDiodes: the number of series diodes
         */
        DiodeT (Next& n, T Is, T Vt = NumericType<T> (25.85e-3), T nDiodes = 1) : next (n)
        {
            n.connectToParent (this);
            setDiodeParameters (Is, Vt, nDiodes);
        }

        /** Sets diode specific parameters */
        void setDiodeParameters (T newIs, T newVt, T nDiodes)
        {
            Is = newIs;
            Vt = nDiodes * newVt;
            twoVt = (T) 2 * Vt;
            oneOverVt = (T) 1 / Vt;
            calcImpedance();
        }

        inline void calcImpedance() override
        {
#if defined(XSIMD_HPP)
            using xsimd::log;
#endif
            using std::log;

            twoR_Is = (T) 2 * next.wdf.R * Is;
            R_Is_overVt = next.wdf.R * Is * oneOverVt;
            logR_Is_overVt = log (R_Is_overVt);
        }

        /** Accepts an incident wave into a WDF diode. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
        }

        /** Propogates a reflected wave from a WDF diode. */
        inline T reflected() noexcept
        {
            // See eqn (10) from reference paper
            wdf.b = wdf.a + twoR_Is - twoVt * Omega::omega4 (logR_Is_overVt + wdf.a * oneOverVt + R_Is_overVt);
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        T Is; // reverse saturation current
        T Vt; // thermal voltage

        // pre-computed vars
        T twoVt;
        T oneOverVt;
        T twoR_Is;
        T R_Is_overVt;
        T logR_Is_overVt;

        const Next& next;
    };

    /** WDF Switch (non-adaptable) */
    template <typename T, typename Next>
    class SwitchT final : public RootWDF
    {
    public:
        explicit SwitchT (Next& next)
        {
            next.connectToParent (this);
        }

        inline void calcImpedance() override {}

        /** Sets the state of the switch. */
        void setClosed (bool shouldClose) { closed = shouldClose; }

        /** Accepts an incident wave into a WDF switch. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
        }

        /** Propogates a reflected wave from a WDF switch. */
        inline T reflected() noexcept
        {
            wdf.b = closed ? -wdf.a : wdf.a;
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        bool closed = true;
    };
} // namespace wdft
} // namespace chowdsp

#endif //CHOWDSP_WDF_WDFT_NONLINEARITIES_H


#endif // CHOWDSP_WDF_T_INCLUDED


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


namespace chowdsp
{
namespace wdf
{
    /** WDF Resistor Node */
    template <typename T>
    class Resistor final : public WDFWrapper<T, wdft::ResistorT<T>>
    {
    public:
        /** Creates a new WDF Resistor with a given resistance.
         * @param value: resistance in Ohms
         */
        explicit Resistor (T value) : WDFWrapper<T, wdft::ResistorT<T>> ("Resistor", value)
        {
        }

        /** Sets the resistance value of the WDF resistor, in Ohms. */
        void setResistanceValue (T newR)
        {
            this->internalWDF.setResistanceValue (newR);
            this->propagateImpedance();
        }
    };

    /** WDF Capacitor Node */
    template <typename T>
    class Capacitor final : public WDFWrapper<T, wdft::CapacitorT<T>>
    {
    public:
        /** Creates a new WDF Capacitor.
         * @param value: Capacitance value in Farads
         * @param fs: WDF sample rate
         */
        explicit Capacitor (T value, T fs = (T) 48000.0) : WDFWrapper<T, wdft::CapacitorT<T>> ("Capacitor", value, fs)
        {
        }

        /** Sets the capacitance value of the WDF capacitor, in Farads. */
        void setCapacitanceValue (T newC)
        {
            this->internalWDF.setCapacitanceValue (newC);
            this->propagateImpedance();
        }

        /** Prepares the capacitor to operate at a new sample rate */
        void prepare (T sampleRate)
        {
            this->internalWDF.prepare (sampleRate);
            this->propagateImpedance();
        }

        /** Resets the capacitor state */
        void reset()
        {
            this->internalWDF.reset();
        }
    };

    /** WDF Capacitor Node with alpha transform parameter */
    template <typename T>
    class CapacitorAlpha final : public WDFWrapper<T, wdft::CapacitorAlphaT<T>>
    {
    public:
        /** Creates a new WDF Capacitor.
         * @param value: Capacitance value in Farads
         * @param fs: WDF sample rate
         * @param alpha: alpha value to be used for the alpha transform,
         *               use 0 for Backwards Euler, use 1 for Bilinear Transform.
         */
        explicit CapacitorAlpha (T value, T fs = (T) 48000.0, T alpha = (T) 1.0) : WDFWrapper<T, wdft::CapacitorAlphaT<T>> ("Capacitor", value, fs, alpha)
        {
        }

        /** Sets the capacitance value of the WDF capacitor, in Farads. */
        void setCapacitanceValue (T newC)
        {
            this->internalWDF.setCapacitanceValue (newC);
            this->propagateImpedance();
        }

        /** Prepares the capacitor to operate at a new sample rate */
        void prepare (T sampleRate)
        {
            this->internalWDF.prepare (sampleRate);
            this->propagateImpedance();
        }

        /** Resets the capacitor state */
        void reset()
        {
            this->internalWDF.reset();
        }

        /** Sets a new alpha value to use for the alpha transform */
        void setAlpha (T newAlpha)
        {
            this->internalWDF.setAlpha (newAlpha);
            this->propagateImpedance();
        }
    };

    /** WDF Inductor Node */
    template <typename T>
    class Inductor final : public WDFWrapper<T, wdft::InductorT<T>>
    {
    public:
        /** Creates a new WDF Inductor.
     * @param value: Inductance value in Farads
     * @param fs: WDF sample rate
     */
        explicit Inductor (T value, T fs = (T) 48000.0) : WDFWrapper<T, wdft::InductorT<T>> ("Inductor", value, fs)
        {
        }

        /** Sets the inductance value of the WDF inductor, in Henries. */
        void setInductanceValue (T newL)
        {
            this->internalWDF.setInductanceValue (newL);
            this->propagateImpedance();
        }

        /** Prepares the inductor to operate at a new sample rate */
        void prepare (T sampleRate)
        {
            this->internalWDF.prepare (sampleRate);
            this->propagateImpedance();
        }

        /** Resets the inductor state */
        void reset()
        {
            this->internalWDF.reset();
        }
    };

    /** WDF Inductor Node with alpha transform parameter */
    template <typename T>
    class InductorAlpha final : public WDFWrapper<T, wdft::InductorAlphaT<T>>
    {
    public:
        /** Creates a new WDF Inductor.
     * @param value: Inductance value in Farads
     * @param fs: WDF sample rate
     * @param alpha: alpha value to be used for the alpha transform,
     *               use 0 for Backwards Euler, use 1 for Bilinear Transform.
     */
        explicit InductorAlpha (T value, T fs = 48000.0, T alpha = 1.0) : WDFWrapper<T, wdft::InductorAlphaT<T>> ("Inductor", value, fs, alpha)
        {
        }

        /** Sets the inductance value of the WDF inductor, in Henries. */
        void setInductanceValue (T newL)
        {
            this->internalWDF.setInductanceValue (newL);
            this->propagateImpedance();
        }

        /** Prepares the inductor to operate at a new sample rate */
        void prepare (T sampleRate)
        {
            this->internalWDF.prepare (sampleRate);
            this->propagateImpedance();
        }

        /** Resets the inductor state */
        void reset()
        {
            this->internalWDF.reset();
        }

        /** Sets a new alpha value to use for the alpha transform */
        void setAlpha (T newAlpha)
        {
            this->internalWDF.setAlpha (newAlpha);
            this->propagateImpedance();
        }
    };

    /** WDF Resistor/Capacitor Series Node */
    template <typename T>
    class ResistorCapacitorSeries final : public WDFWrapper<T, wdft::ResistorCapacitorSeriesT<T>>
    {
    public:
        /** Creates a new WDF Resistor/Capacitor Series node.
         * @param res_value: resistance in Ohms
         * @param cap_value: capacitance in Farads
         */
        explicit ResistorCapacitorSeries (T res_value, T cap_value)
            : WDFWrapper<T, wdft::ResistorCapacitorSeriesT<T>> ("Resistor/Capacitor Series", res_value, cap_value)
        {
        }

        /** Sets the resistance value of the WDF resistor, in Ohms. */
        void setResistanceValue (T newR)
        {
            this->internalWDF.setResistanceValue (newR);
            this->propagateImpedance();
        }

        /** Sets the capacitance value of the WDF capacitor, in Farads. */
        void setCapacitanceValue (T newC)
        {
            this->internalWDF.setCapacitanceValue (newC);
            this->propagateImpedance();
        }

        /** Prepares the capacitor to operate at a new sample rate */
        void prepare (T sampleRate)
        {
            this->internalWDF.prepare (sampleRate);
            this->propagateImpedance();
        }

        /** Resets the capacitor state */
        void reset()
        {
            this->internalWDF.reset();
        }
    };

    /** WDF Resistor/Capacitor Parallel Node */
    template <typename T>
    class ResistorCapacitorParallel final : public WDFWrapper<T, wdft::ResistorCapacitorParallelT<T>>
    {
    public:
        /** Creates a new WDF Resistor/Capacitor Parallel node.
         * @param res_value: resistance in Ohms
         * @param cap_value: capacitance in Farads
         */
        explicit ResistorCapacitorParallel (T res_value, T cap_value)
            : WDFWrapper<T, wdft::ResistorCapacitorParallelT<T>> ("Resistor/Capacitor Parallel", res_value, cap_value)
        {
        }

        /** Sets the resistance value of the WDF resistor, in Ohms. */
        void setResistanceValue (T newR)
        {
            this->internalWDF.setResistanceValue (newR);
            this->propagateImpedance();
        }

        /** Sets the capacitance value of the WDF capacitor, in Farads. */
        void setCapacitanceValue (T newC)
        {
            this->internalWDF.setCapacitanceValue (newC);
            this->propagateImpedance();
        }

        /** Prepares the capacitor to operate at a new sample rate */
        void prepare (T sampleRate)
        {
            this->internalWDF.prepare (sampleRate);
            this->propagateImpedance();
        }

        /** Resets the capacitor state */
        void reset()
        {
            this->internalWDF.reset();
        }
    };
} // namespace wdf
} // namespace chowdsp

#endif //CHOWDSP_WDF_WDF_ONE_PORTS_H

// #include "wdf_sources.h"
#ifndef CHOWDSP_WDF_WDF_SOURCES_H
#define CHOWDSP_WDF_WDF_SOURCES_H

// #include "wdf_base.h"


namespace chowdsp
{
namespace wdf
{
    /** WDF Voltage source with series resistance */
    template <typename T>
    class ResistiveVoltageSource final : public WDFWrapper<T, wdft::ResistiveVoltageSourceT<T>>
    {
    public:
        /** Creates a new resistive voltage source.
     * @param value: initial resistance value, in Ohms
     */
        explicit ResistiveVoltageSource (T value = (NumericType<T>) 1.0e-9) : WDFWrapper<T, wdft::ResistiveVoltageSourceT<T>> ("Resistive Voltage", value)
        {
        }

        /** Sets the resistance value of the series resistor, in Ohms. */
        void setResistanceValue (T newR)
        {
            this->internalWDF.setResistanceValue (newR);
            this->propagateImpedance();
        }

        /** Sets the voltage of the voltage source, in Volts */
        void setVoltage (T newV) { this->internalWDF.setVoltage (newV); }
    };

    /** WDF Ideal Voltage source (non-adaptable) */
    template <typename T>
    class IdealVoltageSource final : public WDFWrapper<T, wdft::IdealVoltageSourceT<T, WDF<T>>>
    {
    public:
        explicit IdealVoltageSource (WDF<T>* next) : WDFWrapper<T, wdft::IdealVoltageSourceT<T, WDF<T>>> ("IdealVoltage", *next)
        {
            next->connectToNode (this);
        }

        /** Sets the voltage of the voltage source, in Volts */
        void setVoltage (T newV) { this->internalWDF.setVoltage (newV); }
    };

    /** WDF Current source with parallel resistance */
    template <typename T>
    class ResistiveCurrentSource final : public WDFWrapper<T, wdft::ResistiveCurrentSourceT<T>>
    {
    public:
        /** Creates a new resistive current source.
     * @param value: initial resistance value, in Ohms
     */
        explicit ResistiveCurrentSource (T value = (NumericType<T>) 1.0e9) : WDFWrapper<T, wdft::ResistiveCurrentSourceT<T>> ("Resistive Current", value)
        {
        }

        /** Sets the resistance value of the parallel resistor, in Ohms. */
        void setResistanceValue (T newR)
        {
            this->internalWDF.setResistanceValue (newR);
            this->propagateImpedance();
        }

        /** Sets the current of the current source, in Amps */
        void setCurrent (T newI) { this->internalWDF.setCurrent (newI); }
    };

    /** WDF Current source (non-adpatable) */
    template <typename T>
    class IdealCurrentSource final : public WDFWrapper<T, wdft::IdealCurrentSourceT<T, WDF<T>>>
    {
    public:
        explicit IdealCurrentSource (WDF<T>* next) : WDFWrapper<T, wdft::IdealCurrentSourceT<T, WDF<T>>> ("Ideal Current", *next)
        {
            next->connectToNode (this);
        }

        /** Sets the current of the current source, in Amps */
        void setCurrent (T newI) { this->internalWDF.setCurrent (newI); }
    };
} // namespace wdf
} // namespace chowdsp

#endif //CHOWDSP_WDF_WDF_SOURCES_H

// #include "wdf_adaptors.h"
#ifndef CHOWDSP_WDF_WDF_ADAPTORS_H
#define CHOWDSP_WDF_WDF_ADAPTORS_H

// #include "wdf_base.h"


namespace chowdsp
{
namespace wdf
{
    /** WDF Voltage Polarity Inverter */
    template <typename T>
    class PolarityInverter final : public WDFWrapper<T, wdft::PolarityInverterT<T, WDF<T>>>
    {
    public:
        /** Creates a new WDF polarity inverter
         * @param port1: the port to connect to the inverter
         */
        explicit PolarityInverter (WDF<T>* port1) : WDFWrapper<T, wdft::PolarityInverterT<T, WDF<T>>> ("Polarity Inverter", *port1)
        {
            port1->connectToNode (this);
        }
    };

    /** WDF y-parameter 2-port (short circuit admittance) */
    template <typename T>
    class YParameter final : public WDFWrapper<T, wdft::YParameterT<T, WDF<T>>>
    {
    public:
        YParameter (WDF<T>* port1, T y11, T y12, T y21, T y22) : WDFWrapper<T, wdft::YParameterT<T, WDF<T>>> ("YParameter", *port1, y11, y12, y21, y22)
        {
            port1->connectToNode (this);
        }
    };

    /** WDF 3-port parallel adaptor */
    template <typename T>
    class WDFParallel final : public WDFWrapper<T, wdft::WDFParallelT<T, WDF<T>, WDF<T>>>
    {
    public:
        /** Creates a new WDF parallel adaptor from two connected ports. */
        WDFParallel (WDF<T>* port1, WDF<T>* port2) : WDFWrapper<T, wdft::WDFParallelT<T, WDF<T>, WDF<T>>> ("Parallel", *port1, *port2)
        {
            port1->connectToNode (this);
            port2->connectToNode (this);
        }
    };

    /** WDF 3-port series adaptor */
    template <typename T>
    class WDFSeries final : public WDFWrapper<T, wdft::WDFSeriesT<T, WDF<T>, WDF<T>>>
    {
    public:
        /** Creates a new WDF series adaptor from two connected ports. */
        WDFSeries (WDF<T>* port1, WDF<T>* port2) : WDFWrapper<T, wdft::WDFSeriesT<T, WDF<T>, WDF<T>>> ("Series", *port1, *port2)
        {
            port1->connectToNode (this);
            port2->connectToNode (this);
        }
    };
} // namespace wdf
} // namespace chowdsp

#endif //CHOWDSP_WDF_WDF_ADAPTORS_H

// #include "wdf_nonlinearities.h"
#ifndef CHOWDSP_WDF_WDF_NONLINEARITIES_H
#define CHOWDSP_WDF_WDF_NONLINEARITIES_H

// #include "wdf_base.h"


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


#endif // CHOWDSP_WDF_H_INCLUDED

// #include "rtype/rtype.h"
#ifndef CHOWDSP_WDF_RTYPE_H_INCLUDED
#define CHOWDSP_WDF_RTYPE_H_INCLUDED

// #include "rtype_adaptor.h"
#ifndef CHOWDSP_WDF_RTYPE_ADAPTOR_H
#define CHOWDSP_WDF_RTYPE_ADAPTOR_H

// #include "../wdft/wdft_base.h"
#ifndef CHOWDSP_WDF_WDFT_BASE_H
#define CHOWDSP_WDF_WDFT_BASE_H

// #include "../math/sample_type.h"
#ifndef CHOWDSP_WDF_SAMPLE_TYPE_H
#define CHOWDSP_WDF_SAMPLE_TYPE_H

#include <type_traits>

#ifndef DOXYGEN

namespace chowdsp
{
#if ! (JUCE_MODULE_AVAILABLE_chowdsp_dsp)
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
#endif

/** Type alias for a SIMD numeric type */
template <typename T>
using NumericType = typename SampleTypeHelpers::ElementType<T>::Type;

/** Returns true if all the elements in a SIMD vector are equal */
inline bool all (bool x)
{
    return x;
}

/** Ternary select operation */
template <typename T>
inline T select (bool b, const T& t, const T& f)
{
    return b ? t : f;
}
} // namespace chowdsp

#endif // DOXYGEN

#endif //CHOWDSP_WDF_SAMPLE_TYPE_H


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

// #include "rtype_detail.h"
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


namespace chowdsp
{
namespace wdft
{
    /**
     *  An adaptable R-Type adaptor.
     *  For more information see: https://searchworks.stanford.edu/view/11891203, chapter 2
     *
     *  The upPortIndex argument descibes with port of the scattering matrix is being adapted.
     *
     *  The ImpedanceCalculator template argument with a static method of the form:
     *  @code
     *  template <typename RType>
     *  static T calcImpedance (RType& R);
     *  @endcode
     */
    template <typename T, int upPortIndex, typename ImpedanceCalculator, typename... PortTypes>
    class RtypeAdaptor : public BaseWDF
    {
    public:
        /** Number of ports connected to RtypeAdaptor */
        static constexpr auto numPorts = int (sizeof...(PortTypes) + 1);

        explicit RtypeAdaptor (PortTypes&... dps) : downPorts (std::tie (dps...))
        {
            b_vec.clear();
            a_vec.clear();

            rtype_detail::forEachInTuple ([&] (auto& port, size_t) { port.connectToParent (this); },
                                          downPorts);
        }

        [[deprecated ("Prefer the alternative constuctor which accepts the port references directly")]] explicit RtypeAdaptor (std::tuple<PortTypes&...> dps) : downPorts (dps)
        {
            b_vec.clear();
            a_vec.clear();

            rtype_detail::forEachInTuple ([&] (auto& port, size_t) { port.connectToParent (this); },
                                          downPorts);
        }

        /** Re-computes the port impedance at the adapted upward-facing port */
        void calcImpedance() override
        {
            wdf.R = ImpedanceCalculator::calcImpedance (*this);
            wdf.G = (T) 1 / wdf.R;
        }

        constexpr auto getPortImpedances()
        {
            std::array<T, numPorts - 1> portImpedances {};
            rtype_detail::forEachInTuple ([&] (auto& port, size_t i) { portImpedances[i] = port.wdf.R; },
                                          downPorts);

            return portImpedances;
        }

        /** Use this function to set the scattering matrix data. */
        void setSMatrixData (const T (&mat)[numPorts][numPorts])
        {
            for (int i = 0; i < numPorts; ++i)
                for (int j = 0; j < numPorts; ++j)
                    S_matrix[j][i] = mat[i][j];
        }

        /** Computes the incident wave. */
        inline void incident (T downWave) noexcept
        {
            wdf.a = downWave;
            a_vec[upPortIndex] = wdf.a;

            rtype_detail::RtypeScatter (S_matrix, a_vec, b_vec);
            rtype_detail::forEachInTuple ([&] (auto& port, size_t i) {
                                              auto portIndex = getPortIndex ((int) i);
                                              port.incident (b_vec[portIndex]); },
                                          downPorts);
        }

        /** Computes the reflected wave */
        inline T reflected() noexcept
        {
            rtype_detail::forEachInTuple ([&] (auto& port, size_t i) {
                                              auto portIndex = getPortIndex ((int) i);
                                              a_vec[portIndex] = port.reflected(); },
                                          downPorts);

            wdf.b = b_vec[upPortIndex];
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        constexpr auto getPortIndex (int tupleIndex)
        {
            return tupleIndex < upPortIndex ? tupleIndex : tupleIndex + 1;
        }

        std::tuple<PortTypes&...> downPorts; // tuple of ports connected to RtypeAdaptor

        rtype_detail::Matrix<T, numPorts> S_matrix; // square matrix representing S
        rtype_detail::AlignedArray<T, numPorts> a_vec; // temp matrix of inputs to Rport
        rtype_detail::AlignedArray<T, numPorts> b_vec; // temp matrix of outputs from Rport
    };
} // namespace wdft
} // namespace chowdsp

#endif //CHOWDSP_WDF_RTYPE_ADAPTOR_H

// #include "root_rtype_adaptor.h"
#ifndef CHOWDSP_WDF_ROOT_RTYPE_ADAPTOR_H
#define CHOWDSP_WDF_ROOT_RTYPE_ADAPTOR_H

// #include "../wdft/wdft_base.h"

// #include "rtype_detail.h"


namespace chowdsp
{
namespace wdft
{
    /**
     *  A non-adaptable R-Type adaptor.
     *  For more information see: https://searchworks.stanford.edu/view/11891203, chapter 2
     *
     *  The ImpedanceCalculator template argument with a static method of the form:
     *  @code
     *  template <typename RType>
     *  static void calcImpedance (RType& R);
     *  @endcode
     */
    template <typename T, typename ImpedanceCalculator, typename... PortTypes>
    class RootRtypeAdaptor : public RootWDF
    {
    public:
        /** Number of ports connected to RootRtypeAdaptor */
        static constexpr auto numPorts = int (sizeof...(PortTypes));

        explicit RootRtypeAdaptor (PortTypes&... dps) : downPorts (std::tie (dps...))
        {
            b_vec.clear();
            a_vec.clear();

            rtype_detail::forEachInTuple ([&] (auto& port, size_t) { port.connectToParent (this); },
                                          downPorts);
        }

        [[deprecated ("Prefer the alternative constuctor which accepts the port references directly")]] explicit RootRtypeAdaptor (std::tuple<PortTypes&...> dps) : downPorts (dps)
        {
            b_vec.clear();
            a_vec.clear();

            rtype_detail::forEachInTuple ([&] (auto& port, size_t) { port.connectToParent (this); },
                                          downPorts);
        }

        /** Recomputes internal variables based on the incoming impedances */
        void calcImpedance() override
        {
            ImpedanceCalculator::calcImpedance (*this);
        }

        constexpr auto getPortImpedances()
        {
            std::array<T, numPorts> portImpedances {};
            rtype_detail::forEachInTuple ([&] (auto& port, size_t i) { portImpedances[i] = port.wdf.R; },
                                          downPorts);

            return portImpedances;
        }

        /** Use this function to set the scattering matrix data. */
        void setSMatrixData (const T (&mat)[numPorts][numPorts])
        {
            for (int i = 0; i < numPorts; ++i)
                for (int j = 0; j < numPorts; ++j)
                    S_matrix[j][i] = mat[i][j];
        }

        /** Computes both the incident and reflected waves at this root node. */
        inline void compute() noexcept
        {
            rtype_detail::RtypeScatter (S_matrix, a_vec, b_vec);
            rtype_detail::forEachInTuple ([&] (auto& port, size_t i) {
                                          port.incident (b_vec[i]);
                                          a_vec[i] = port.reflected(); },
                                          downPorts);
        }

    private:
        std::tuple<PortTypes&...> downPorts; // tuple of ports connected to RtypeAdaptor

        rtype_detail::Matrix<T, numPorts> S_matrix; // square matrix representing S
        rtype_detail::AlignedArray<T, numPorts> a_vec; // temp matrix of inputs to Rport
        rtype_detail::AlignedArray<T, numPorts> b_vec; // temp matrix of outputs from Rport
    };
} // namespace wdft
} // namespace chowdsp

#endif //CHOWDSP_WDF_ROOT_RTYPE_ADAPTOR_H

// #include "wdf_rtype.h"
#ifndef CHOWDSP_WDF_WDF_RTYPE_H
#define CHOWDSP_WDF_WDF_RTYPE_H

#include <functional>
// #include "../wdf/wdf_base.h"
#ifndef CHOWDSP_WDF_WDF_BASE_H
#define CHOWDSP_WDF_WDF_BASE_H

#include <string>
#include <utility>

// #include "../wdft/wdft.h"
#ifndef CHOWDSP_WDF_T_INCLUDED
#define CHOWDSP_WDF_T_INCLUDED

namespace chowdsp
{
/** API for constructing Wave Digital Filters with a fixed compile-time architecture */
namespace wdft
{
}

} // namespace chowdsp

// #include "wdft_one_ports.h"
#ifndef CHOWDSP_WDF_WDFT_ONE_PORTS_H
#define CHOWDSP_WDF_WDFT_ONE_PORTS_H

// #include "wdft_base.h"
#ifndef CHOWDSP_WDF_WDFT_BASE_H
#define CHOWDSP_WDF_WDFT_BASE_H

// #include "../math/sample_type.h"
#ifndef CHOWDSP_WDF_SAMPLE_TYPE_H
#define CHOWDSP_WDF_SAMPLE_TYPE_H

#include <type_traits>

#ifndef DOXYGEN

namespace chowdsp
{
#if ! (JUCE_MODULE_AVAILABLE_chowdsp_dsp)
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
#endif

/** Type alias for a SIMD numeric type */
template <typename T>
using NumericType = typename SampleTypeHelpers::ElementType<T>::Type;

/** Returns true if all the elements in a SIMD vector are equal */
inline bool all (bool x)
{
    return x;
}

/** Ternary select operation */
template <typename T>
inline T select (bool b, const T& t, const T& f)
{
    return b ? t : f;
}
} // namespace chowdsp

#endif // DOXYGEN

#endif //CHOWDSP_WDF_SAMPLE_TYPE_H


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
            wdf.a = (T) 0;
            wdf.b = (T) 0;
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
            z -= T_over_T_plus_2RC * (wdf.a + z);
        }

        /** Propogates a reflected wave from the WDF. */
        inline T reflected() noexcept
        {
            wdf.b = -z;
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
            wdf.a = (T) 0;
            wdf.b = (T) 0;
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
            z = wdf.b + wdf.a - z;
        }

        /** Propogates a reflected wave from the WDF. */
        inline T reflected() noexcept
        {
            wdf.b = twoRC_over_twoRC_plus_T * z;
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

// #include "wdft_sources.h"
#ifndef CHOWDSP_WDF_WDFT_SOURCES_H
#define CHOWDSP_WDF_WDFT_SOURCES_H

// #include "wdft_base.h"


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

// #include "wdft_adaptors.h"
#ifndef CHOWDSP_WDF_WDFT_ADAPTORS_H
#define CHOWDSP_WDF_WDFT_ADAPTORS_H

// #include "wdft_base.h"


namespace chowdsp
{
namespace wdft
{
    /** WDF 3-port parallel adaptor */
    template <typename T, typename Port1Type, typename Port2Type>
    class WDFParallelT final : public BaseWDF
    {
    public:
        /** Creates a new WDF parallel adaptor from two connected ports. */
        WDFParallelT (Port1Type& p1, Port2Type& p2) : port1 (p1),
                                                      port2 (p2)
        {
            port1.connectToParent (this);
            port2.connectToParent (this);
            calcImpedance();
        }

        /** Computes the impedance for a WDF parallel adaptor.
         *  1     1     1
         * --- = --- + ---
         * Z_p   Z_1   Z_2
         */
        inline void calcImpedance() override
        {
            wdf.G = port1.wdf.G + port2.wdf.G;
            wdf.R = (T) 1.0 / wdf.G;
            port1Reflect = port1.wdf.G / wdf.G;
        }

        /** Accepts an incident wave into a WDF parallel adaptor. */
        inline void incident (T x) noexcept
        {
            const auto b2 = wdf.b - port2.wdf.b + x;
            port1.incident (b2 + bDiff);
            port2.incident (b2);

            wdf.a = x;
        }

        /** Propogates a reflected wave from a WDF parallel adaptor. */
        inline T reflected() noexcept
        {
            port1.reflected();
            port2.reflected();

            bDiff = port2.wdf.b - port1.wdf.b;
            wdf.b = port2.wdf.b - port1Reflect * bDiff;

            return wdf.b;
        }

        Port1Type& port1;
        Port2Type& port2;

        WDFMembers<T> wdf;

    private:
        T port1Reflect = (T) 1.0;
        T bDiff = (T) 0.0;
    };

    /** WDF 3-port series adaptor */
    template <typename T, typename Port1Type, typename Port2Type>
    class WDFSeriesT final : public BaseWDF
    {
    public:
        /** Creates a new WDF series adaptor from two connected ports. */
        WDFSeriesT (Port1Type& p1, Port2Type& p2) : port1 (p1),
                                                    port2 (p2)
        {
            port1.connectToParent (this);
            port2.connectToParent (this);
            calcImpedance();
        }

        /** Computes the impedance for a WDF parallel adaptor.
         * Z_s = Z_1 + Z_2
         */
        inline void calcImpedance() override
        {
            wdf.R = port1.wdf.R + port2.wdf.R;
            wdf.G = (T) 1.0 / wdf.R;
            port1Reflect = port1.wdf.R / wdf.R;
        }

        /** Accepts an incident wave into a WDF series adaptor. */
        inline void incident (T x) noexcept
        {
            const auto b1 = port1.wdf.b - port1Reflect * (x + port1.wdf.b + port2.wdf.b);
            port1.incident (b1);
            port2.incident (-(x + b1));

            wdf.a = x;
        }

        /** Propogates a reflected wave from a WDF series adaptor. */
        inline T reflected() noexcept
        {
            wdf.b = -(port1.reflected() + port2.reflected());
            return wdf.b;
        }

        Port1Type& port1;
        Port2Type& port2;

        WDFMembers<T> wdf;

    private:
        T port1Reflect = (T) 1.0;
    };

    /** WDF Voltage Polarity Inverter */
    template <typename T, typename PortType>
    class PolarityInverterT final : public BaseWDF
    {
    public:
        /** Creates a new WDF polarity inverter */
        explicit PolarityInverterT (PortType& p) : port1 (p)
        {
            port1.connectToParent (this);
            calcImpedance();
        }

        /** Calculates the impedance of the WDF inverter
         * (same impedance as the connected node).
         */
        inline void calcImpedance() override
        {
            wdf.R = port1.wdf.R;
            wdf.G = (T) 1.0 / wdf.R;
        }

        /** Accepts an incident wave into a WDF inverter. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
            port1.incident (-x);
        }

        /** Propogates a reflected wave from a WDF inverter. */
        inline T reflected() noexcept
        {
            wdf.b = -port1.reflected();
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        PortType& port1;
    };

    /** WDF y-parameter 2-port (short circuit admittance) */
    template <typename T, typename PortType>
    class YParameterT final : public BaseWDF
    {
    public:
        /** Creates a new WDF Y-Parameter, with the given coefficients */
        YParameterT (PortType& port1, T y11, T y12, T y21, T y22) : port1 (port1)
        {
            y[0][0] = y11;
            y[0][1] = y12;
            y[1][0] = y21;
            y[1][1] = y22;

            port1.connectToParent (this);
            calcImpedance();
        }

        /** Calculates the impedance of the WDF Y-Parameter */
        inline void calcImpedance() override
        {
            denominator = y[1][1] + port1.wdf.R * y[0][0] * y[1][1] - port1.wdf.R * y[0][1] * y[1][0];
            wdf.R = (port1.wdf.R * y[0][0] + (T) 1.0) / denominator;
            wdf.G = (T) 1.0 / wdf.R;

            T rSq = port1.wdf.R * port1.wdf.R;
            T num1A = -y[1][1] * rSq * y[0][0] * y[0][0];
            T num2A = y[0][1] * y[1][0] * rSq * y[0][0];

            A = (num1A + num2A + y[1][1]) / (denominator * (port1.wdf.R * y[0][0] + (T) 1.0));
            B = -port1.wdf.R * y[0][1] / (port1.wdf.R * y[0][0] + (T) 1.0);
            C = -y[1][0] / denominator;
        }

        /** Accepts an incident wave into a WDF Y-Parameter. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
            port1.incident (A * port1.wdf.b + B * x);
        }

        /** Propogates a reflected wave from a WDF Y-Parameter. */
        inline T reflected() noexcept
        {
            wdf.b = C * port1.reflected();
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        PortType& port1;
        T y[2][2] = { { (T) 0.0, (T) 0.0 }, { (T) 0.0, (T) 0.0 } };

        T denominator = (T) 1.0;
        T A = (T) 1.0;
        T B = (T) 1.0;
        T C = (T) 1.0;
    };

    // useful "factory" functions so you don't have to declare all the template parameters

    /** Factory method for creating a parallel adaptor between two elements. */
    template <typename T, typename P1Type, typename P2Type>
    CHOWDSP_WDF_MAYBE_UNUSED WDFParallelT<T, P1Type, P2Type> makeParallel (P1Type& p1, P2Type& p2)
    {
        return WDFParallelT<T, P1Type, P2Type> (p1, p2);
    }

    /** Factory method for creating a series adaptor between two elements. */
    template <typename T, typename P1Type, typename P2Type>
    CHOWDSP_WDF_MAYBE_UNUSED WDFSeriesT<T, P1Type, P2Type> makeSeries (P1Type& p1, P2Type& p2)
    {
        return WDFSeriesT<T, P1Type, P2Type> (p1, p2);
    }

    /** Factory method for creating a polarity inverter. */
    template <typename T, typename PType>
    CHOWDSP_WDF_MAYBE_UNUSED PolarityInverterT<T, PType> makeInverter (PType& p1)
    {
        return PolarityInverterT<T, PType> (p1);
    }
} // namespace wdft
} // namespace chowdsp

#endif //CHOWDSP_WDF_WDFT_ADAPTORS_H

// #include "wdft_nonlinearities.h"
#ifndef CHOWDSP_WDF_WDFT_NONLINEARITIES_H
#define CHOWDSP_WDF_WDFT_NONLINEARITIES_H

#include <cmath>

// #include "wdft_base.h"


// #include "../math/signum.h"


namespace chowdsp
{
/** Methods for implementing the signum function */
namespace signum
{
    /** Signum function to determine the sign of the input. */
    template <typename T>
    inline int signum (T val)
    {
        return (T (0) < val) - (val < T (0));
    }

#if defined(XSIMD_HPP)
    /** Signum function to determine the sign of the input. */
    template <typename T>
    inline xsimd::batch<T> signum (xsimd::batch<T> val)
    {
        using v_type = xsimd::batch<T>;
        const auto positive = xsimd::select (val > v_type ((T) 0), v_type ((T) 1), v_type ((T) 0));
        const auto negative = xsimd::select (val < v_type ((T) 0), v_type ((T) 1), v_type ((T) 0));
        return positive - negative;
    }
#endif

} // namespace signum
} // namespace chowdsp

// #include "../math/omega.h"
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
// #include "sample_type.h"


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


namespace chowdsp
{
namespace wdft
{
    /** Enum to determine which diode approximation eqn. to use */
    enum DiodeQuality
    {
        Good, // see reference eqn (18)
        Best, // see reference eqn (39)
    };

    /**
     * WDF diode pair (non-adaptable)
     * See Werner et al., "An Improved and Generalized Diode Clipper Model for Wave Digital Filters"
     * https://www.researchgate.net/publication/299514713_An_Improved_and_Generalized_Diode_Clipper_Model_for_Wave_Digital_Filters
     */
    template <typename T, typename Next, DiodeQuality Quality = DiodeQuality::Best>
    class DiodePairT final : public RootWDF
    {
    public:
        /**
         * Creates a new WDF diode pair, with the given diode specifications.
         * @param n: the next element in the WDF connection tree
         * @param Is: reverse saturation current
         * @param Vt: thermal voltage
         * @param nDiodes: the number of series diodes
         */
        DiodePairT (Next& n, T Is, T Vt = NumericType<T> (25.85e-3), T nDiodes = 1) : next (n)
        {
            n.connectToParent (this);
            setDiodeParameters (Is, Vt, nDiodes);
        }

        /** Sets diode specific parameters */
        void setDiodeParameters (T newIs, T newVt, T nDiodes)
        {
            Is = newIs;
            Vt = nDiodes * newVt;
            twoVt = (T) 2 * Vt;
            oneOverVt = (T) 1 / Vt;
            calcImpedance();
        }

        inline void calcImpedance() override
        {
#if defined(XSIMD_HPP)
            using xsimd::log;
#endif
            using std::log;

            R_Is = next.wdf.R * Is;
            R_Is_overVt = R_Is * oneOverVt;
            logR_Is_overVt = log (R_Is_overVt);
        }

        /** Accepts an incident wave into a WDF diode pair. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
        }

        /** Propogates a reflected wave from a WDF diode pair. */
        inline T reflected() noexcept
        {
            reflectedInternal();
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        /** Implementation for float/double (Good). */
        template <typename C = T, DiodeQuality Q = Quality>
        inline typename std::enable_if<Q == Good, void>::type
            reflectedInternal() noexcept
        {
            // See eqn (18) from reference paper
            T lambda = (T) signum::signum (wdf.a);
            wdf.b = wdf.a + (T) 2 * lambda * (R_Is - Vt * Omega::omega4 (logR_Is_overVt + lambda * wdf.a * oneOverVt + R_Is_overVt));
        }

        /** Implementation for float/double (Best). */
        template <typename C = T, DiodeQuality Q = Quality>
        inline typename std::enable_if<Q == Best, void>::type
            reflectedInternal() noexcept
        {
            // See eqn (39) from reference paper
            T lambda = (T) signum::signum (wdf.a);
            T lambda_a_over_vt = lambda * wdf.a * oneOverVt;
            wdf.b = wdf.a - twoVt * lambda * (Omega::omega4 (logR_Is_overVt + lambda_a_over_vt) - Omega::omega4 (logR_Is_overVt - lambda_a_over_vt));
        }

        T Is; // reverse saturation current
        T Vt; // thermal voltage

        // pre-computed vars
        T twoVt;
        T oneOverVt;
        T R_Is;
        T R_Is_overVt;
        T logR_Is_overVt;

        const Next& next;
    };

    /**
     * WDF diode (non-adaptable)
     * See Werner et al., "An Improved and Generalized Diode Clipper Model for Wave Digital Filters"
     * https://www.researchgate.net/publication/299514713_An_Improved_and_Generalized_Diode_Clipper_Model_for_Wave_Digital_Filters
     */
    template <typename T, typename Next, DiodeQuality Quality = DiodeQuality::Best>
    class DiodeT final : public RootWDF
    {
    public:
        /**
         * Creates a new WDF diode, with the given diode specifications.
         * @param n: the next element in the WDF connection tree
         * @param Is: reverse saturation current
         * @param Vt: thermal voltage
         * @param nDiodes: the number of series diodes
         */
        DiodeT (Next& n, T Is, T Vt = NumericType<T> (25.85e-3), T nDiodes = 1) : next (n)
        {
            n.connectToParent (this);
            setDiodeParameters (Is, Vt, nDiodes);
        }

        /** Sets diode specific parameters */
        void setDiodeParameters (T newIs, T newVt, T nDiodes)
        {
            Is = newIs;
            Vt = nDiodes * newVt;
            twoVt = (T) 2 * Vt;
            oneOverVt = (T) 1 / Vt;
            calcImpedance();
        }

        inline void calcImpedance() override
        {
#if defined(XSIMD_HPP)
            using xsimd::log;
#endif
            using std::log;

            twoR_Is = (T) 2 * next.wdf.R * Is;
            R_Is_overVt = next.wdf.R * Is * oneOverVt;
            logR_Is_overVt = log (R_Is_overVt);
        }

        /** Accepts an incident wave into a WDF diode. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
        }

        /** Propogates a reflected wave from a WDF diode. */
        inline T reflected() noexcept
        {
            // See eqn (10) from reference paper
            wdf.b = wdf.a + twoR_Is - twoVt * Omega::omega4 (logR_Is_overVt + wdf.a * oneOverVt + R_Is_overVt);
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        T Is; // reverse saturation current
        T Vt; // thermal voltage

        // pre-computed vars
        T twoVt;
        T oneOverVt;
        T twoR_Is;
        T R_Is_overVt;
        T logR_Is_overVt;

        const Next& next;
    };

    /** WDF Switch (non-adaptable) */
    template <typename T, typename Next>
    class SwitchT final : public RootWDF
    {
    public:
        explicit SwitchT (Next& next)
        {
            next.connectToParent (this);
        }

        inline void calcImpedance() override {}

        /** Sets the state of the switch. */
        void setClosed (bool shouldClose) { closed = shouldClose; }

        /** Accepts an incident wave into a WDF switch. */
        inline void incident (T x) noexcept
        {
            wdf.a = x;
        }

        /** Propogates a reflected wave from a WDF switch. */
        inline T reflected() noexcept
        {
            wdf.b = closed ? -wdf.a : wdf.a;
            return wdf.b;
        }

        WDFMembers<T> wdf;

    private:
        bool closed = true;
    };
} // namespace wdft
} // namespace chowdsp

#endif //CHOWDSP_WDF_WDFT_NONLINEARITIES_H


#endif // CHOWDSP_WDF_T_INCLUDED


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

// #include "rtype_detail.h"


namespace chowdsp
{
namespace wdf
{
    /**
     *  A non-adaptable R-Type adaptor.
     *  For more information see: https://searchworks.stanford.edu/view/11891203, chapter 2
     */
    template <typename T>
    class RootRtypeAdaptor : public WDF<T>
    {
    public:
        RootRtypeAdaptor (std::initializer_list<WDF<T>*> dps)
            : WDF<T> ("Root R-Type Adaptor"),
              downPorts (dps),
              S_matrix (downPorts.size(), downPorts.size()),
              a_vec (downPorts.size()),
              b_vec (downPorts.size())
        {
            for (auto* port : downPorts)
                port->connectToParent (this);
        }

        /** Returns the number of ports connected to RootRtypeAdaptor */
        size_t getNumPorts() const noexcept { return downPorts.size(); }

        /**
         * Returns the port impedance for the given port index.
         * Note: it is the caller's responsibility to ensure that the portIndex is in range!
         */
        T getPortImpedance (size_t portIndex) const noexcept { return downPorts[portIndex]->wdf.R; }

        /** Recomputes internal variables based on the incoming impedances */
        void calcImpedance() override
        {
            impedanceCalculator (*this);
        }

        /** Use this function to set the scattering matrix data. */
        template <size_t N>
        void setSMatrixData (const T (&mat)[N][N])
        {
            const auto numPorts = a_vec.size();
            for (int i = 0; i < numPorts; ++i)
                for (int j = 0; j < numPorts; ++j)
                    S_matrix[j][i] = mat[i][j];
        }

        /** Computes both the incident and reflected waves at this root node. */
        inline void compute() noexcept
        {
            rtype_detail::RtypeScatter (S_matrix, a_vec, b_vec);

            int i = 0;
            for (auto* port : downPorts)
            {
                port->incident (b_vec[i]);
                a_vec[i] = port->reflected();
                i++;
            }
        }

        /** Implement this function to set the scattering matrix when an incoming impedance changes */
        std::function<void (RootRtypeAdaptor&)> impedanceCalculator = [] (auto&) {};

    private:
        void incident (T) noexcept override {}
        T reflected() noexcept override { return T {}; }

        std::vector<WDF<T>*> downPorts;

        rtype_detail::Matrix<T> S_matrix; // square matrix representing S
        rtype_detail::AlignedArray<T> a_vec; // temp matrix of inputs to Rport
        rtype_detail::AlignedArray<T> b_vec; // temp matrix of outputs from Rport
    };

    /**
     *  An adaptable R-Type adaptor.
     *  For more information see: https://searchworks.stanford.edu/view/11891203, chapter 2
     */
    template <typename T>
    class RtypeAdaptor : public WDF<T>
    {
    public:
        /** The upPortIndex argument describes with port of the scattering matrix is being adapted. */
        RtypeAdaptor (std::initializer_list<WDF<T>*> dps, int upPortIndex)
            : WDF<T> ("R-Type Adaptor"),
              m_upPortIndex (upPortIndex),
              downPorts (dps),
              S_matrix (downPorts.size() + 1, downPorts.size() + 1),
              a_vec (downPorts.size() + 1),
              b_vec (downPorts.size() + 1)
        {
            for (auto* port : downPorts)
                port->connectToParent (this);
        }

        /** Returns the number of ports connected to RtypeAdaptor */
        size_t getNumPorts() const noexcept { return downPorts.size() + 1; }

        /**
         * Returns the port impedance for the given port index.
         * Note: it is the caller's responsibility to ensure that the portIndex is in range!
         */
        T getPortImpedance (size_t portIndex) const noexcept { return downPorts[portIndex]->wdf.R; }

        /** Recomputes internal variables based on the incoming impedances */
        void calcImpedance() override
        {
            this->wdf.R = impedanceCalculator (*this);
            this->wdf.G = (T) 1 / this->wdf.R;
        }

        /** Use this function to set the scattering matrix data. */
        template <size_t N>
        void setSMatrixData (const T (&mat)[N][N])
        {
            const auto numPorts = a_vec.size();
            for (int i = 0; i < numPorts; ++i)
                for (int j = 0; j < numPorts; ++j)
                    S_matrix[j][i] = mat[i][j];
        }

        /** Computes the incident wave. */
        inline void incident (T downWave) noexcept override
        {
            this->wdf.a = downWave;
            a_vec[m_upPortIndex] = this->wdf.a;

            rtype_detail::RtypeScatter (S_matrix, a_vec, b_vec);

            int i = 0;
            for (auto* port : downPorts)
            {
                auto portIndex = getPortIndex (i);
                port->incident (b_vec[i]);
                i++;
            }
        }

        /** Computes the reflected wave */
        inline T reflected() noexcept override
        {
            int i = 0;
            for (auto* port : downPorts)
            {
                auto portIndex = getPortIndex (i);
                a_vec[portIndex] = port->reflected();
                i++;
            }

            this->wdf.b = b_vec[m_upPortIndex];
            return this->wdf.b;
        }

        /** Implement this function to set the scattering matrix when an incoming impedance changes */
        std::function<T (RtypeAdaptor&)> impedanceCalculator = [] (auto&) { return (T) 1; };

    private:
        int getPortIndex (int vectorIndex)
        {
            return vectorIndex < m_upPortIndex ? vectorIndex : vectorIndex + 1;
        }

        std::vector<WDF<T>*> downPorts;

        const int m_upPortIndex;

        rtype_detail::Matrix<T> S_matrix; // square matrix representing S
        rtype_detail::AlignedArray<T> a_vec; // temp matrix of inputs to Rport
        rtype_detail::AlignedArray<T> b_vec; // temp matrix of outputs from Rport
    };
} // namespace wdf
} // namespace chowdsp

#endif //CHOWDSP_WDF_WDF_RTYPE_H


#endif // CHOWDSP_WDF_RTYPE_H_INCLUDED


// #include "util/defer_impedance.h"
#ifndef WAVEDIGITALFILTERS_DEFER_IMPEDANCE_H
#define WAVEDIGITALFILTERS_DEFER_IMPEDANCE_H

namespace chowdsp
{
namespace wdft
{
    /**
 * Let's say that you've got some fancy adaptor in your WDF (e.g. an R-Type adaptor),
 * which requires a somewhat expensive computation to re-calculate the adaptor's impedance.
 * In that case, you may not want to re-calculate the impedance every time a downstream element
 * changes its impedance, since the impedance might need to be re-calculated several times.
 * For that purpose, you can use this class as follows:
 * ```cpp
 * class MyWDF
 * {
 *   // ...
 *
 *   Resistor pot1 {};
 *   WDFSeries S1 { pot1, ... };
 *
 *   Resistor pot2 {};
 *   WDFParallel P1 { pot2, ... };
 *
 *   FancyAdaptor adaptor { std::tie (S1, P1, ...); }; // this adaptor does heavy computation when re-calculating the impedance
 *
 * public:
 *   void setParameters (float pot1Value, float pot2Value)
 *   {
 *     {
 *       // while this object is alive, any impedance changes that are propagated
 *       // to S1 will cause S1 to recompute its impedance, but will not allow impeance
 *       // changes to be propagated any further up the tree, and the same for P1.
 *
 *       ScopedDeferImpedancePropagation deferImpedance { S1, P1 };
 *       pot1.setResistanceValue (pot1Value);
 *       pot2.setResistanceValue (pot2Value);
 *     }
 *
 *     // Now we must manually tell the fancyAdaptor that downstream impedances have changed:
 *     fancyAdaptor.propagateImpedanceChange();
 *   }
 * }
 * ```
 */
    template <typename... Elements>
    class ScopedDeferImpedancePropagation
    {
    public:
        explicit ScopedDeferImpedancePropagation (Elements&... elems) : elements (std::tie (elems...))
        {
#if __cplusplus >= 201703L // With C++17 and later, it's easy to assert that all the elements are derived from BaseWDF
            static_assert ((std::is_base_of<BaseWDF, Elements>::value && ...), "All element types must be derived from BaseWDF");
#endif

            rtype_detail::forEachInTuple (
                [] (auto& el, size_t) {
                    el.dontPropagateImpedance = true;
                },
                elements);
        }

        ~ScopedDeferImpedancePropagation()
        {
            rtype_detail::forEachInTuple (
                [] (auto& el, size_t) {
                    el.dontPropagateImpedance = false;
                    el.calcImpedance();
                },
                elements);
        }

    private:
        std::tuple<Elements&...> elements;
    };
} // namespace wdft
} // namespace chowdsp

#endif //WAVEDIGITALFILTERS_DEFER_IMPEDANCE_H


#if defined(_MSC_VER)
#pragma warning(pop)
#endif
