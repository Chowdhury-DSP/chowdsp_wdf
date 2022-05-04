#pragma once

// we want to be able to use this header without JUCE, so let's #if out JUCE-specific implementations
#ifndef WDF_USING_JUCE
#define WDF_USING_JUCE JUCE_WINDOWS || JUCE_ANDROID || JUCE_BSD || JUCE_LINUX || JUCE_MAC || JUCE_IOS || JUCE_WASM
#endif

// define NODISCARD if we can
#if __cplusplus >= 201703L
#define CHOWDSP_WDF_MAYBE_UNUSED [[maybe_unused]]
#else
#define CHOWDSP_WDF_MAYBE_UNUSED
#endif

#include "wdft/wdft.h"
#include "wdf/wdf.h"
#include "rtype/rtype.h"
