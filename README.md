[![CI](https://github.com/Chowdhury-DSP/chowdsp_wdf/actions/workflows/test.yml/badge.svg)](https://github.com/Chowdhury-DSP/chowdsp_wdf/actions/workflows/test.yml)
[![License](https://img.shields.io/badge/License-BSD-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)
[![codecov](https://codecov.io/gh/Chowdhury-DSP/chowdsp_wdf/branch/main/graph/badge.svg?token=DR1OKVN2KJ)](https://codecov.io/gh/Chowdhury-DSP/chowdsp_wdf)

# Wave Digital Filters

`chowdsp_wdf` is a header only library for implementing real-time
Wave Digital Filter (WDF) circuit models in C++.

More information:
- [API docs](https://ccrma.stanford.edu/~jatin/chowdsp/chowdsp_wdf)
- [Examples](https://github.com/jatinchowdhury18/WaveDigitalFilters)
- Other projects using `chowdsp_wdf`:
  - [Build Your Own Distortion](https://github.com/Chowdhury-DSP/BYOD)
  - [ChowKick](https://github.com/Chowdhury-DSP/ChowKick)
  - [ChowCentaur](https://github.com/jatinchowdhury18/KlonCentaur)

## Usage

Since `chowdsp_wdf` is a header-only library, it should be possible to use the library
simply by including `include/chowdsp_wdf/chowdsp_wdf.h`. However, it is recommended to
use the library via CMake.

```cmake
add_subdirectory(chowdsp_wdf)
target_link_libraries(my_cmake_target PUBLIC chowdsp_wdf)
```

### Dependencies
- C++14 or higher
- CMake (Version 3.1+, optional)
- XSIMD (optional, see below)

### Basic Usage

A basic RC lowpass filter can be constructed as follows:
```cpp
namespace wdft = chowdsp::wdft;
struct RCLowpass {
    wdft::ResistorT<double> r1 { 1.0e3 };     // 1 KOhm Resistor
    wdft::CapacitorT<double> c1 { 1.0e-6 };   // 1 uF capacitor
    
    wdft::WDFSeriesT<double, decltype (r1), decltype (c1)> s1 { r1, c1 };   // series connection of r1 and c1
    wdft::IdealVoltageSourceT<double, decltype (s1)> vSource { s1 };        // input voltage source
    
    // prepare the WDF model here...
    void prepare (double sampleRate) {
        c1.prepare (sampleRate);
    }
    
    // use the WDF model to process one sample of data
    inline double processSample (double x) {
        vSource.setVoltage (x);

        vs.incident (p1.reflected());
        p1.incident (vs.reflected());

        return wdft::voltage<double> (c1);
    }
};
```

More complicated examples can be found in the
[examples](https://github.com/jatinchowdhury18/WaveDigitalFilters) repository.

### Using XSIMD

There are two specific situations where you may want to use
SIMD intrinsics as part of your WDF model:
1. You want to run the same circuit model on several values in parallel. For example,
   maybe you have a WDF model of a synthesizer voice, and want to run several voices,
   like for a polyphonic synthesizer.
2. You have a circuit model that requires an R-Type adaptor. The R-Type adaptor requires
   a matrix multiply operation which can be greatly accelerated with SIMD intrinsics.

In both cases, to use SIMD intrinsics in your WDF model, you must include `XSIMD`
in your project _before_ `chowdsp_wdf`.

```cpp
#include <xsimd/xsmd.hpp> // this must be included _before_ the chowdsp_wdf header!
#include <chowdsp_wdf/chowdsp_wdf.h>
```

For case 2 above, simply construct your circuit with an R-Type adaptor as desired,
and the SIMD optomizations will be taken care of behind the scenes!

For case 1 above, you will want to construct your WDF model so that the circuit elements
may process XSIMD types. Going back to the RC lowpass example:
```cpp
namespace wdft = chowdsp::wdft;

// Define the WDF model to process a template-defined FloatType
template <typename FloatType>
struct RCLowpass {
    wdft::ResistorT<FloatType> r1 { 1.0e3 };     // 1 KOhm Resistor
    wdft::CapacitorT<FloatType> c1 { 1.0e-6 };   // 1 uF capacitor
    
    wdft::WDFSeriesT<FloatType, decltype (r1), decltype (c1)> s1 { r1, c1 };   // series connection of r1 and c1
    wdft::IdealVoltageSourceT<FloatType, decltype (s1)> vSource { s1 };        // input voltage source
    
    // prepare the WDF model here...
    void prepare (double sampleRate) {
        c1.prepare ((FloatType) sampleRate);
    }
    
    // use the WDF model to process one sample of data
    inline FloatType processSample (FloatType x) {
        vSource.setVoltage (x);

        vs.incident (p1.reflected());
        p1.incident (vs.reflected());

        return wdft::voltage<FloatType> (c1);
    }
};

RCLowpass<xsimd::batch<double>> myFilter; // instantiate the WDF to process an XSIMD type!
```

If you are using `chowdsp_wdf` with XSIMD, please remember to abide by the XSIMD license.

## Resources

The design and implementation of the library were discussed on The Audio Programmer
meetup in December 2021. The presentation can be watched on [YouTube](https://www.youtube.com/watch?v=Auwf9z0k_7E&t=1s).

The following academic papers may also be useful:

[1] Alfred Fettweis, "Wave Digital Filters: Theory and Practice",
1986, IEEE Invited Paper,
[link](https://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=1457726).

[2] Julius Smith, "Wave Digital Filters", (Chapter from *Physical
Audio Signal Processing*) [link](https://ccrma.stanford.edu/~jos/pasp/Wave_Digital_Filters_I.html).

[3] David Yeh and Julius Smith, "Simulating Guitar Distortion Circuits
Using Wave Digital And Nonlinear State Space Formulations", Proc. of the
11th Int. Conference on Digital Audio Effects, 2008,
[link](http://legacy.spa.aalto.fi/dafx08/papers/dafx08_04.pdf).

[4] Kurt Werner, et al., "Wave Digital Filter Modeling of Circuits
with Operational Amplifiers", 24th European Signal Processing Conference,
2016, [link](https://www.eurasip.org/Proceedings/Eusipco/Eusipco2016/papers/1570255463.pdf).

[5] Kurt Werner, et al., "Resolving Wave Digital Filters with
Multiple/Multiport Nonlinearities", Proc. of the 18th Int. Conference
on Digital Audio Effects, 2015, [link](https://ccrma.stanford.edu/~jingjiez/portfolio/gtr-amp-sim/pdfs/Resolving%20Wave%20Digital%20Filters%20with%20MultipleMultiport%20Nonlinearities.pdf).

[6] Kurt Werner, "Virtual Analog Modeling of Audio Circuitry Using
Wave Digital Filters", PhD. Dissertation, Stanford University, 2016,
[link](https://stacks.stanford.edu/file/druid:jy057cz8322/KurtJamesWernerDissertation-augmented.pdf).

[7] Jingjie Zhang and Julius Smith, "Real-time Wave Digital Simulation
of Cascaded Vacuum Tube Amplifiers Using Modified Blockwise Method",
Proc. of the 21st International Conference on Digital Audio Effects,
2018, [link](https://www.dafx.de/paper-archive/2018/papers/DAFx2018_paper_25.pdf).

## Credits

The diode models in the library utilise an approximation of the Wright Omega
function based on [Stephen D'Angelo's implementation](https://www.dangelo.audio/dafx2019-omega.html),
which is licensed under the MIT license.

Many thanks to the following individuals who have contributed to the
theory, design, and implementation of the library:
- Julius Smith
- Jingjie Zhang
- Kurt Werner
- [Paul Walker](https://github.com/baconpaul)
- [Eyal Amir](https://github.com/eyalamirmusic)
- [Dirk Roosenburg](https://github.com/droosenb)

## Licensing

The code in this repository is open source, and licensed under the BSD 3-Clause License.
Enjoy!
