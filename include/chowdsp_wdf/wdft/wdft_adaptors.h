#ifndef CHOWDSP_WDF_WDFT_ADAPTORS_H
#define CHOWDSP_WDF_WDFT_ADAPTORS_H

#include "wdft_base.h"

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
            auto b2 = x + bTemp;
            port1.incident (bDiff + b2);
            port2.incident (b2);
            wdf.a = x;
        }

        /** Propogates a reflected wave from a WDF parallel adaptor. */
        inline T reflected() noexcept
        {
            port1.reflected();
            port2.reflected();

            bDiff = port2.wdf.b - port1.wdf.b;
            bTemp = -port1Reflect * bDiff;
            wdf.b = port2.wdf.b + bTemp;

            return wdf.b;
        }

        Port1Type& port1;
        Port2Type& port2;

        WDFMembers<T> wdf;

    private:
        T port1Reflect = (T) 1.0;

        T bTemp = (T) 0.0;
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
            auto b1 = port1.wdf.b - port1Reflect * (x + port1.wdf.b + port2.wdf.b);
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
