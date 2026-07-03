# Appendix F — Software (as installed)

Running `SETUPGLD` on the [Program Disks](appendix-e-program-disks.md) expands
the spanned `GOLD*.CMP` archive into a `\GOLD` directory of **133 files**: the
DOS drivers, the end-user applications, a large sample library and a set of demo
songs. This appendix presents that complete installed tree.

The files here were **not** obtained by running the 1992 installer (it aborts
with *"Gold Card not found"* on any machine without the hardware, and its
decompressor is entangled with the C++ runtime). Instead the "Ad Lib Comp."
container format and its LZ+Huffman codec were **reverse-engineered from
`SETUPGLD.EXE`** and re-implemented — see
[Appendix H](appendix-h-goldcmp-format.md). Correctness is proven: the recovered
`CTRLDRV.EXE` is byte-for-byte identical (matching MD5) to the copy that ships
*uncompressed* on Program Disk 1, and every recovered executable is a valid
`MZ` image. The tree was malware-scanned clean.

Browse the decoded files under
[`disks/program-disks-v1.00/installed/`](https://github.com/slartibardfast/adlib_gold/tree/main/manual/src/disks/program-disks-v1.00/installed).

## `\GOLD\DRIVERS` — DOS device drivers

The loadable driver set documented in [Chapter 5](ch05-dos-drivers.md).

| Size | Driver | Role |
|------|--------|------|
| 12 048 | `CTRLDRV.EXE` | Control chip — mixer, card setup, IRQ/DMA ([Ch. 7.1](ch07-low-level.md#register-access)) |
| 11 648 | `FMDRV.EXE` | FM synthesis (OPL3 / YMF262) |
| 22 416 | `WAVEDRV.EXE` | Digital audio (MMA / YMZ263) |
| 6 508 | `MIDIDRV.EXE` | MIDI I/O |
| 10 050 | `TIMERDRV.EXE` | Timer services |
| 32 835 | `SYNCDRV.EXE` | Synchronisation |
| 51 553 | `RL2DRV.EXE` | `.RL2` music playback |
| 40 605 | `ALARMDRV.EXE` | Alarm / scheduling |

## `\GOLD` — applications

| Size | Program | Role |
|------|---------|------|
| 278 861 | `TESTGOLD.EXE` | card test / diagnostics ([Ch. 4](ch04-software-applications.md)) |
| 286 385 | `VPAD.EXE` | VoicePad |
| 285 704 | `SNDTRACK.EXE` | SoundTracker |
| 179 518 | `JUKEG.EXE` | Jukebox |
| 73 532 | `ANIMGLD.EXE` | animation player |
| 44 022 | `PLAYDIGI.EXE` | digital-audio player |
| 23 770 | `MIXER.EXE` | mixer panel |
| 15 212 | `PLAYRL2.EXE` | `.RL2` player |

Plus batch launchers (`SETUP.BAT`, `TEST.BAT`, `JUKEGOLD.BAT`,
`STRACKED.BAT`, `VOICEPAD.BAT`, `PLAYANIM.BAT`, `DRIVERS.BAT`, `STRKDRV.BAT`),
instrument banks `OPL3.BNK` / `SAMPLES.BNK`, a sync demo `SYNCDEMO.SNC`, and the
installed [`README.TXT`](disks/program-disks-v1.00/installed/README.TXT)
([md](disks-md/program-disks-v1.00/installed/README.TXT.md)).

## `\GOLD` — demo songs (`.RL2`)

Fourteen `.RL2` sequences ship as demos: `BUILDING`, `CAVE`, `FLIGHT`,
`HIGHWAY2`, `INDUSTRY`, `KRAKEN`, `LORDS`, `LUTECONC`, `MACHINE`, `MIRRORS`,
`NEWERA`, `TOCCATA2`, `ULTIMATE`, `WALKPARK`.

## `\GOLD\SMP` — sample library

**90 `.SMP` sample files** — the percussion and instrument library used by the
tools (bass/snare drums, cymbals, hi-hats, congas, cowbells, claves, bells,
brushes, handclaps, and the `$TEXT*` speech samples). See the
[full listing](https://github.com/slartibardfast/adlib_gold/tree/main/manual/src/disks/program-disks-v1.00/installed/SMP).

## Version history (v0.91b beta → v1.00)

Because the codec is now decodable, the **beta software (v0.91b, 31 March 1992)**
— preserved as floppy images in [Appendix G](appendix-g-sdk.md) — can be
expanded too and diffed against the v1.00 release (23 April 1992, three weeks
later). The changes:

- **Sample library reorganised.** In the beta the ~85 `.SMP` files sat loose in
  `\GOLD`; v1.00 moves them into the new `\GOLD\SMP` sub-directory. This
  accounts for most of the apparent file churn — the samples themselves are
  unchanged.
- **Demo songs renamed** — the beta's `*GLD`/`*GD` suffixes were dropped:
  `BUILDGLD`→`BUILDING`, `CAVEGLD`→`CAVE`, `ERAGLD`→`NEWERA`, `INDUSGLD`→`INDUSTRY`,
  `KRAKENGD`→`KRAKEN`, `MACHIGLD`→`MACHINE`, `MIRRORGD`→`MIRRORS`,
  `ULTIMGLD`→`ULTIMATE`, `WALKGLD`→`WALKPARK`, `LUTECON`→`LUTECONC`. The beta's
  `FUGUE2`/`PRELUDE2` were dropped; `TOCCATA2`, `HIGHWAY2`, `FLIGHT` and `LORDS`
  were added.
- **`SAMPLBNK.EQU` removed** — confirmed verbatim by the v1.00 `README`: *"Due to
  changes in the .RL2 file format, file SAMPLBNK.EQU is no longer necessary and
  has been removed."*
- **Animation player restructured** — the beta's single `PLAYANIM.EXE` became a
  `PLAYANIM.BAT` launcher driving a new `ANIMGLD.EXE`.
- **Every driver and application was recompiled** (all compressed sizes differ);
  most notably `ALARMDRV.EXE` and `PLAYRL2.EXE` roughly halved in size.
- **Batch reshuffle** — `SYNCDRV.BAT` dropped, `STRKDRV.BAT` added, and
  `SETUP.BAT` folded into the payload (it now auto-calls `CTRLDRV.EXE`, per the
  v1.00 `README`).

The beta `README` also records user-facing changes settling in just before
release: the `MIXER`/`RL2DRV` hotkeys became redefinable from `SETUP`, and the
mixer's **F4** key was repurposed to save the configuration permanently to
EEPROM. Developer-facing addenda continued into **v1.01** — the
[Release Notes](release-notes.md) add the `RL2DRV` driver-ordering requirement,
the `SAMPL.EXE` library-conflict warning, and the interrupt-disable discipline
for hardware access.
