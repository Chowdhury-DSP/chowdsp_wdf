#ifndef CHOWDSP_WDF_ROOT_RTYPE_ADAPTOR_H
#define CHOWDSP_WDF_ROOT_RTYPE_ADAPTOR_H

#include "../wdft/wdft_base.h"
#include "rtype_detail.h"

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

        explicit RootRtypeAdaptor (std::tuple<PortTypes&...> dps) : downPorts (dps)
        {
            for (int i = 0; i < numPorts; i++)
            {
                b_vec[i] = (T) 0;
                a_vec[i] = (T) 0;
            }

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
                    S_matrix[i][j] = mat[i][j];
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
        T a_vec alignas (CHOWDSP_WDF_DEFAULT_SIMD_ALIGNMENT)[numPorts]; // temp matrix of inputs to Rport
        T b_vec alignas (CHOWDSP_WDF_DEFAULT_SIMD_ALIGNMENT)[numPorts]; // temp matrix of outputs from Rport
    };
} // namespace wdft
} // namespace chowdsp

#endif //CHOWDSP_WDF_ROOT_RTYPE_ADAPTOR_H
