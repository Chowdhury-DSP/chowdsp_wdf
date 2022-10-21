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
