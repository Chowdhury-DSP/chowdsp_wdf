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

### Requirements
- C++14 or higher
- CMake (Version 3.1+)
- XSIMD
  - Required for processing SIMD data types
  - Highly recommended for circuits with R-Type adaptors
  - Not needed otherwise

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

## Licensing

The code in this repository is open source, and licensed under the BSD 3-Clause License.
Enjoy!