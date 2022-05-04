#ifndef CHOWDSP_WDF_WDF_ADAPTORS_H
#define CHOWDSP_WDF_WDF_ADAPTORS_H

#include "wdf_base.h"

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
