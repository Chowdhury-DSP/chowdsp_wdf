#ifndef CHOWDSP_WDF_WDF_RTYPE_H
#define CHOWDSP_WDF_WDF_RTYPE_H

#include <functional>
#include "../wdf/wdf_base.h"
#include "rtype_detail.h"

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
