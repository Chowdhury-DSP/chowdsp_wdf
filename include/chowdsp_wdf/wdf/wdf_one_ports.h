#ifndef CHOWDSP_WDF_WDF_ONE_PORTS_H
#define CHOWDSP_WDF_WDF_ONE_PORTS_H

#include "wdf_base.h"

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
