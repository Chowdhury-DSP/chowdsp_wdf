#ifndef CHOWDSP_WDF_WDFT_NONLINEARITIES_H
#define CHOWDSP_WDF_WDFT_NONLINEARITIES_H

#include <cmath>

#include "wdft_base.h"

#include "../math/signum.h"
#include "../math/omega.h"

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
