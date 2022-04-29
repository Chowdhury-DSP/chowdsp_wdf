#pragma once

// we want to be able to use this header without JUCE, so let's #if out JUCE-specific implementations
#ifndef WDF_USING_JUCE
#define WDF_USING_JUCE JUCE_WINDOWS || JUCE_ANDROID || JUCE_BSD || JUCE_LINUX || JUCE_MAC || JUCE_IOS || JUCE_WASM
#endif

#include "wdft/wdft.h"
#include "wdf/wdf.h"
//#include "r_type.h"
