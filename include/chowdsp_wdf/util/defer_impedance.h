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
