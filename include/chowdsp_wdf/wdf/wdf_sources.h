#ifndef CHOWDSP_WDF_WDF_SOURCES_H
#define CHOWDSP_WDF_WDF_SOURCES_H

#include "wdf_base.h"

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
