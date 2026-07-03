# Source Code Cross-Reference

This manual documents the Ad Lib Gold hardware and its original DOS drivers. The
repository that hosts this book, [`slartibardfast/adlib_gold`](https://github.com/slartibardfast/adlib_gold),
contains a **modern WDM audio driver** that re-implements the same hardware
interfaces for Windows. The table below maps the manual's chapters to the driver
source, so you can read the specification and the implementation side by side.

Note on naming: the manual's **MMA** digital-audio controller is the **YMZ263**,
the **OPL3** FM synthesizer is the **YMF262**, and the **SP2** surround processor
is the **YM7128**.

| Source file | Role | Related manual sections |
|-------------|------|-------------------------|
| [`adapter.cpp`](https://github.com/slartibardfast/adlib_gold/blob/main/adapter.cpp) | Adapter driver: setup, resource allocation, miniport start-up | [Ch 3 - Gold Hardware](ch03-gold-hardware.md), [Ch 7 - Low-Level](ch07-low-level.md) |
| [`common.cpp`](https://github.com/slartibardfast/adlib_gold/blob/main/common.cpp) / [`common.h`](https://github.com/slartibardfast/adlib_gold/blob/main/common.h) | Adapter common object: Control Chip register access with bank switching, ISR dispatch, mixer shadow | [Ch 5 - Control Features driver](ch05-dos-drivers.md), [Ch 7 register maps](ch07-low-level.md) |
| [`algtopo.cpp`](https://github.com/slartibardfast/adlib_gold/blob/main/algtopo.cpp) / [`algtopo.h`](https://github.com/slartibardfast/adlib_gold/blob/main/algtopo.h) | Topology miniport: exposes the Control Chip mixer as a KS topology filter (volume/tone/mute, surround) | [Ch 4 - Mixer Panel](ch04-software-applications.md), [Ch 7 - Mixer registers](ch07-low-level.md), [SP2 surround](appendix-sp2.md) |
| [`algwave.cpp`](https://github.com/slartibardfast/adlib_gold/blob/main/algwave.cpp) / [`algwave.h`](https://github.com/slartibardfast/adlib_gold/blob/main/algwave.h) | WaveCyclic miniport: YMZ263 (MMA) digital audio - PCM/ADPCM record & playback | [Ch 5 - Wave driver](ch05-dos-drivers.md), [Ch 7 - MMA registers](ch07-low-level.md) |
| [`fmsynth.cpp`](https://github.com/slartibardfast/adlib_gold/blob/main/fmsynth.cpp) / [`fmsynth.h`](https://github.com/slartibardfast/adlib_gold/blob/main/fmsynth.h) | FM synth miniport: OPL3 (YMF262) FM synthesis | [Ch 5 - FM driver](ch05-dos-drivers.md), [Ch 7 - OPL3/ALMSC](ch07-low-level.md), [GSS OPL3](appendix-gss.md) |
| [`midi.cpp`](https://github.com/slartibardfast/adlib_gold/blob/main/midi.cpp) / [`midi.h`](https://github.com/slartibardfast/adlib_gold/blob/main/midi.h) | MIDI UART miniport: YMZ263 (MMA) MIDI subsystem | [Ch 5 - MIDI driver](ch05-dos-drivers.md) |
| [`adlibgold.inf`](https://github.com/slartibardfast/adlib_gold/blob/main/adlibgold.inf) | Windows setup information (INF) | [Ch 2 - Quick Start](ch02-quick-start.md) |
| [`adlibgold.rc`](https://github.com/slartibardfast/adlib_gold/blob/main/adlibgold.rc) / [`sources`](https://github.com/slartibardfast/adlib_gold/blob/main/sources) | Driver resources and DDK build script | - |
| [`doc/sdk.txt`](https://github.com/slartibardfast/adlib_gold/tree/main/doc) | Text SDK reference (this manual's source material) | whole manual |
| [`doc/wdm.txt`](https://github.com/slartibardfast/adlib_gold/tree/main/doc) | WDM porting notes | [Ch 5](ch05-dos-drivers.md), [Ch 7](ch07-low-level.md) |

The driver's overall architecture and its mapping to this hardware are described
in the repository's [`CLAUDE.md`](https://github.com/slartibardfast/adlib_gold/blob/main/CLAUDE.md)
development plan.
