# Ad Lib Gold WDM Audio Driver

A Windows WDM audio driver for the **Ad Lib Gold 1000/2000** sound card (1992),
built around the Yamaha OPL3 (YMF262) FM synthesizer, the MMA (YMZ263) digital
audio controller, an optional YM7128 (SP2) surround processor, and a MIDI/game
port. The driver is organized as an adapter plus four miniports (topology, wave,
FM synth, MIDI), adapted from the Windows 2000 DDK sample drivers.

## 📖 SDK reference

The **Ad Lib Gold Developer Toolkit v1.01** — the SDK this driver is written
against — is published as a searchable mdBook at:

### 👉 https://slartibardfast.github.io/adlib_gold/

It documents the hardware, the DOS driver APIs, and the low-level register
interface this driver implements. **This is the canonical SDK reference** for
the project; the raw OCR text in [`doc/sdk.txt`](doc/sdk.txt) is its source
material.

## Source layout

| File | Role |
|------|------|
| `adapter.cpp` | Adapter driver: setup, resource allocation, miniport start-up |
| `common.cpp` / `common.h` | Adapter common object: Control Chip register access (bank switching), ISR dispatch, mixer shadow |
| `algtopo.cpp` / `algtopo.h` | Topology miniport: Control Chip mixer as a KS topology filter |
| `algwave.cpp` / `algwave.h` | WaveCyclic miniport: YMZ263 (MMA) PCM/ADPCM digital audio |
| `fmsynth.cpp` / `fmsynth.h` | FM synth miniport: OPL3 (YMF262) |
| `midi.cpp` / `midi.h` | MIDI UART miniport: YMZ263 (MMA) MIDI |
| `adlibgold.inf` | Windows setup information (INF) |
| `adlibgold.rc` / `sources` | Driver resources and DDK build script |
| `doc/` | Reference material (`sdk.txt`, `wdm.txt`) |
| `manual/` | mdBook source for the developer manual (deployed to GitHub Pages) |

## Building

The driver builds with the Windows DDK build environment via the `sources`
file (`TARGETNAME=adlibgold`, `TARGETTYPE=DRIVER`).

The manual builds with [mdBook](https://rust-lang.github.io/mdBook/):

```sh
mdbook serve manual   # live preview at http://localhost:3000
mdbook build manual   # static site under manual/book/
```

Pushes to `main` that touch `manual/**` are automatically rebuilt and deployed
to GitHub Pages by [`.github/workflows/mdbook.yml`](.github/workflows/mdbook.yml).

## About the manual reproduction

The manual is a historical 1992 document. Its technical content is preserved as
written; only structure and formatting were modernized (chapter reassembly, OCR
cleanup, code blocks, cross-links). Text recovered by OCR may contain occasional
transcription errors — consult the original scan where exactness matters.
