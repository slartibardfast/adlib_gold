# Appendix G — Developer SDK

Beyond the end-user [Program Disks](appendix-e-program-disks.md), Ad Lib shipped
a **Developer Toolkit** — the C/assembly source of the DOS drivers documented in
[Chapter 5](ch05-dos-drivers.md) and [Chapter 7](ch07-low-level.md), with
buildable examples. This appendix preserves it.

## Editions

Three distinct developer/pre-release lineages exist; all are preserved here as
authentic floppy images:

| Edition | Disks | Contents |
|---------|-------|----------|
| **Developer Toolkit v1.01** | 3 × 720 KB | installer floppies that expand to the SDK source below |
| **Beta Software v0.91b** | 3 × 720 KB | pre-release drivers/software |
| *(Program Disks v1.00)* | — | the end-user release, in [Appendix E](appendix-e-program-disks.md) |

Toolkit v1.01 images:
[Disk 1](disks/developer-toolkit-v1.01/images/Developer%20Toolkit%20v1.01%20-%20Disk%201.IMA) ·
[Disk 2](disks/developer-toolkit-v1.01/images/Developer%20Toolkit%20v1.01%20-%20Disk%202.IMA) ·
[Disk 3](disks/developer-toolkit-v1.01/images/Developer%20Toolkit%20v1.01%20-%20Disk%203.IMA).
Beta v0.91b images are under
[`disks/beta-v0.91b/images/`](disks/beta-v0.91b/images/).

### Beta v0.91b disk contents

The beta (31 March 1992) is an installer set, structured like the release: a
`SETUP.EXE` plus a spanned `GOLD.CMP` archive that expands to a 130-file `\GOLD`
tree (three weeks before v1.00 — the differences are catalogued in the
[v0.91b → v1.00 changelog](appendix-f-as-installed.md#version-history-v091b-beta--v100)).
The as-shipped files:

| Size | File | Notes |
|------|------|-------|
| 292 536 | [`SETUP.EXE`](disks/beta-v0.91b/disk1/SETUP.EXE) | beta installer |
| 11 962 | [`CTRLDRV.EXE`](disks/beta-v0.91b/disk1/CTRLDRV.EXE) | control-chip TSR |
| 1 165 | [`README.TXT`](disks/beta-v0.91b/disk1/README.TXT) ([rendered](disks-md/beta-v0.91b/disk1/README.TXT.md)) | beta notes / hotkey changes |
| 412 698 / 714 950 / 464 053 | `GOLD.CMP` (disk 1 / 2 / 3) | spanned "Ad Lib Comp." payload ([format](appendix-h-goldcmp-format.md)) |

## SDK source (installed toolkit)

The toolkit expands to the driver source in
[`disks/developer-toolkit-v1.01/installed/`](disks/developer-toolkit-v1.01/installed/).
Each source file is preserved in its original DOS encoding, with a UTF-8
rendering alongside.

| Module | Source | Header | Documents |
|--------|--------|--------|-----------|
| Control chip | [`CONTROL.C`](disks-md/developer-toolkit-v1.01/installed/CONTROL.C.md) | [`CONTROL.H`](disks-md/developer-toolkit-v1.01/installed/CONTROL.H.md) | [Ch. 7.1 mixer/setup](ch07-low-level.md#register-access) |
| FM synthesis | [`FM.ASM`](disks-md/developer-toolkit-v1.01/installed/FM.ASM.md) | [`FM.H`](disks-md/developer-toolkit-v1.01/installed/FM.H.md) | [Ch. 7.2 YMF262](ch07-low-level.md) |
| Digital audio | [`WAVE.C`](disks-md/developer-toolkit-v1.01/installed/WAVE.C.md) | [`WAVE.H`](disks-md/developer-toolkit-v1.01/installed/WAVE.H.md) | [Ch. 7.3 MMA](ch07-low-level.md) |
| MIDI | [`MIDI.C`](disks-md/developer-toolkit-v1.01/installed/MIDI.C.md) | [`MIDI.H`](disks-md/developer-toolkit-v1.01/installed/MIDI.H.md) | [Ch. 5](ch05-dos-drivers.md) |
| Timers | [`TIMER.C`](disks-md/developer-toolkit-v1.01/installed/TIMER.C.md) | [`TIMER.H`](disks-md/developer-toolkit-v1.01/installed/TIMER.H.md) | [Ch. 7 timers](ch07-low-level.md#timers) |
| Interrupts | [`INTERR.C`](disks-md/developer-toolkit-v1.01/installed/INTERR.C.md) | [`INTERR.H`](disks-md/developer-toolkit-v1.01/installed/INTERR.H.md) | |
| Get/Set | [`SET_GET.C`](disks-md/developer-toolkit-v1.01/installed/SET_GET.C.md) | [`SET_GET.H`](disks-md/developer-toolkit-v1.01/installed/SET_GET.H.md) | |

Shared: [`GLOBAL.H`](disks-md/developer-toolkit-v1.01/installed/GLOBAL.H.md),
[`DMA.H`](disks-md/developer-toolkit-v1.01/installed/DMA.H.md),
[`MYMACRO.INC`](disks-md/developer-toolkit-v1.01/installed/MYMACRO.INC.md),
[`MODEL2.MAC`](disks-md/developer-toolkit-v1.01/installed/MODEL2.MAC.md),
[`MAKEFILE`](disks-md/developer-toolkit-v1.01/installed/MAKEFILE.md).
Pre-built library: `DRIVERS.LIB`. Editor: `ED.EXE`. Sampler: `SAMPL.EXE`.

The toolkit manual itself, `SDTK.DOC`, is a Microsoft Word-for-DOS binary. The
DOS-era `.DOC` format was never officially published, so it is preserved
[byte-for-byte](disks/developer-toolkit-v1.01/installed/SDTK.DOC) **and** rendered
to faithful UTF-8 Markdown — the format decoded directly (CP437 body, `0x07` table
cells, `0x13/14/15` Word fields), with all 51 tables reconstructed and every text
byte preserved:
[**SDTK manual (rendered)**](disks-md/developer-toolkit-v1.01/installed/SDTK.DOC.md).

## Example programs (`SAMPLE/`)

Buildable examples exercising each subsystem:
[`FMS.C`](disks-md/developer-toolkit-v1.01/installed/SAMPLE/FMS.C.md) (FM),
[`MIDIS.C`](disks-md/developer-toolkit-v1.01/installed/SAMPLE/MIDIS.C.md) (MIDI),
[`PLAYBACK.C`](disks-md/developer-toolkit-v1.01/installed/SAMPLE/PLAYBACK.C.md) /
[`RECORD.C`](disks-md/developer-toolkit-v1.01/installed/SAMPLE/RECORD.C.md) (digital audio),
[`TIMERS.C`](disks-md/developer-toolkit-v1.01/installed/SAMPLE/TIMERS.C.md) (timers),
each with its `MAKEFILE` and `.LNK`.
