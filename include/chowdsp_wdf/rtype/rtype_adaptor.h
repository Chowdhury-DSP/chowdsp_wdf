#ifndef CHOWDSP_WDF_RTYPE_ADAPTOR_H
#define CHOWDSP_WDF_RTYPE_ADAPTOR_H

#include "../wdft/wdft_base.h"
#include "rtype_detail.h"

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
