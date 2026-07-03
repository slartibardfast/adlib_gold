# Appendix E — Program Disks (as shipped)

The Ad Lib Gold **Program Disks, version 1.00** (1992) are the end-user
software that shipped with the card. This appendix preserves them **exactly as
distributed**: the authentic 720 KB floppy images, every file byte-for-byte in
its original DOS (CP437 / CRLF) form, and a UTF-8 rendering of each text file.

The *expanded* result of running the installer is documented separately in
[Appendix F — Software (as installed)](appendix-f-as-installed.md); the
compression format of the `GOLD*.CMP` archive is reverse-engineered in
[Appendix H — the "Ad Lib Comp." format](appendix-h-goldcmp-format.md).

## Release timeline

The Ad Lib Gold software was released across 1992–93 — all preserved in this
archive. The dates below are the **newest confirmed file timestamp** in each
release's own files (FAT directory entries, or archive metadata for the Windows
drivers) — the most reliable evidence of when each master was cut. Because the
lineages are **versioned independently** (the SDK *Developer Toolkit*, the retail
*Program Disks*, and the *Windows driver* package), the version numbers do not
track the dates:

| Date | Release | Version | Where |
|------|---------|---------|-------|
| Apr 1992 | Beta software | v0.91b | [Appendix G](appendix-g-sdk.md#beta-v091b-disk-contents) |
| Apr 1992 | Developer Toolkit | **v1.01** | [Appendix G](appendix-g-sdk.md) *(this manual)* |
| Oct 1992 | Program Disks | **v1.00** | this appendix |
| Oct 1992 | Windows Mixer & Drivers | v0.9 | [below](#windows-31-mixer--drivers) |
| Dec 1992 | Windows Mixer & Drivers | v0.9b | [below](#windows-31-mixer--drivers) |
| Jul 1993 | Windows Mixer & Drivers | **v1.2** *(latest)* | [below](#windows-31-mixer--drivers) |

<sup>**Dating basis** — the newest genuine file in each set, excluding two
`1994-03-30` archival re-master stamps and disk-ripper tags. Evidence: **Beta**
`SETUP.EXE` 2 Apr, payload `GOLD.CMP` 9 Apr 1992 (its `README` is dated 31 Mar
1992); **Developer Toolkit** files 30 Mar – 2 Apr 1992 — the `SDTK.DOC`
"Thu, Jul 8 1993" page footer is a Microsoft Word auto-date *field*, not the
authoring date; **Program Disks** core install `GOLD1/2/3.CMP` 30 Sep – 1 Oct
1992, but this archived master is a later composite re-cut that also bundles the
1993 GSS Windows drivers (`GSS.DRV` 10 Mar 1993, byte-identical to v1.2) and a
14 Nov 1993 `G2S.EXE`; **Windows** v0.9 to 27 Oct 1992, v0.9b to 4 Dec 1992, v1.2
binaries Mar 1993 / `README` 13 Jul 1993. The "Ad Lib Comp." container stores only
a name and compressed size per file — no timestamps — so the compressed payloads
add no further dates.</sup>

## Provenance & integrity

The three images are the authentic v1.00 release preserved by the Internet
Archive ([`adlib-gold-bundle`](https://archive.org/details/adlib-gold-bundle)),
cross-checked against the VGMPF copy. Each is a genuine **720 KB double-density**
floppy. Before inclusion every image was:

- **boot-sector verified** — a benign, stock loader; the OEM signature is
  `ALF  3.0` ("Copyright 1987 ALF Products Inc."), the disk-duplication hardware
  Ad Lib used to master retail floppies. No boot-sector virus code is present
  (the legitimate "Non-System disk" loader occupies the standard offset, and no
  known signature — Stoned/Form/Michelangelo/… — appears).
- **malware-scanned** — Microsoft Defender reported no threats.

Disk images (mountable in DOSBox / any emulator):

| Disk | Image |
|------|-------|
| 1 | [`Program Disk 1 v1.00.img`](disks/program-disks-v1.00/images/Program%20Disk%201%20v1.00.img) |
| 2 | [`Program Disk 2 v1.00.img`](disks/program-disks-v1.00/images/Program%20Disk%202%20v1.00.img) |
| 3 | [`Program Disk 3 v1.00.img`](disks/program-disks-v1.00/images/Program%20Disk%203%20v1.00.img) |

## Disk 1 — installer & control driver

| Size | File | Notes |
|------|------|-------|
| 470 | [`SETUP.BAT`](disks/program-disks-v1.00/disk1/SETUP.BAT) ([md](disks-md/program-disks-v1.00/disk1/SETUP.BAT.md)) | runs `CTRLDRV` then `SETUPGLD` |
| 291 776 | `SETUPGLD.EXE` | the installer (expands `GOLD*.CMP`) |
| 12 048 | `CTRLDRV.EXE` | control-chip TSR / card detection |
| 417 508 | `GOLD1.CMP` | first part of the spanned install archive |
| 1 113 | [`README.TXT`](disks/program-disks-v1.00/disk1/README.TXT) ([md](disks-md/program-disks-v1.00/disk1/README.TXT.md)) | v1.00 install notes |

## Disk 2 — archive part 2, Sound Blaster emulator, Windows MIDI

| Size | File | Notes |
|------|------|-------|
| 603 174 | `GOLD2.CMP` | second part of the spanned archive |
| 19 120 | `G2S/G2S.EXE` | **Sound Blaster emulator** — a V86-mode TSR (≈352 bytes resident) that traps SB I/O and remaps it to the Gold hardware (~50% of SB apps; also emulates a DAC on LPT1) |
| 3 064 | [`G2S/G2S.DOC`](disks/program-disks-v1.00/disk2/G2S/G2S.DOC) | its documentation |
| 37 742 | `WIN_MIDI/MIDIMAP.CFG` | Windows MIDI mapper config |
| 112 | [`WIN_MIDI/README.TXT`](disks/program-disks-v1.00/disk2/WIN_MIDI/README.TXT) | |

## Disk 3 — archive part 3, Windows 3.1 drivers

| Size | File | Notes |
|------|------|-------|
| 545 106 | `GOLD3.CMP` | third part of the spanned archive |
| 77 312 | `WINDRV/MIXERGLD.EXE` | Windows mixer applet |
| 25 600 | `WINDRV/OPL3.CPL` | OPL3 control-panel applet |
| 20 704 | `WINDRV/GSSOPL3.DRV` | Windows OPL3 driver |
| 17 984 | `WINDRV/GSS.DRV` | Windows Gold Sound Standard driver |
| 13 946 | `WINDRV/OPL3.HLP` | help file |
| 5 854 | `WINDRV/VGSS.386` | Windows 386 virtual device |
| 238 | `WINDRV/OEMSETUP.INF` | Windows driver install info |
| 8 587 | [`WINDRV/README.TXT`](disks/program-disks-v1.00/disk3/WINDRV/README.TXT) | Windows driver notes |

> `GOLD1.CMP` + `GOLD2.CMP` + `GOLD3.CMP` together form one **spanned "Ad Lib
> Comp." archive** — a proprietary LZ+Huffman container expanded by `SETUPGLD`.
> Its format and the recovery of its 133-file payload are documented in
> [Appendix H](appendix-h-goldcmp-format.md); the payload itself is browsable in
> [Appendix F](appendix-f-as-installed.md).

## Windows 3.1 Mixer & Drivers

The Windows drivers shipped separately from the DOS software (via the Ad Lib
BBS) and went through **three revisions** — the newest of which, **v1.2 (July
1993), is the last dated Ad Lib Gold artifact of any kind.** The early releases
used a single combined driver (`SFGOLD.DRV` + `VGOLD.386`); by v1.2 this had been
split and renamed into two *GSS* drivers. The Windows mixer applet `MIXERGLD.EXE`
is unchanged across all three. All are preserved here and malware-scanned clean.

### v1.2 (July 1993) — the latest

Two Windows drivers (the *Yamaha GSS MIDI Synth* and the *GSS Wave/MIDI/Aux*
driver), the mixer applet, and a MIDI-mapper configuration. These driver binaries
are byte-identical to the `WINDRV` set on Program Disk 3, so v1.2 is essentially a
re-release with updated documentation.

| Size | File | Notes |
|------|------|-------|
| 8 480 | [`README.TXT`](disks/windows-drivers-v1.2/README.TXT) ([rendered](disks-md/windows-drivers-v1.2/README.TXT.md)) | v1.2 install guide (July 1993) |
| 17 984 | [`GSS.DRV`](disks/windows-drivers-v1.2/GSS.DRV) | Gold Sound Standard Wave/MIDI/Aux driver |
| 20 704 | [`GSSOPL3.DRV`](disks/windows-drivers-v1.2/GSSOPL3.DRV) | GSS MIDI Synth (OPL3) driver |
| 77 312 | [`MIXERGLD.EXE`](disks/windows-drivers-v1.2/MIXERGLD.EXE) | Windows Gold mixer applet |
| 25 600 | [`OPL3.CPL`](disks/windows-drivers-v1.2/OPL3.CPL) | Control-Panel applet |
| 13 946 | [`OPL3.HLP`](disks/windows-drivers-v1.2/OPL3.HLP) | help file |
| 5 854 | [`VGSS.386`](disks/windows-drivers-v1.2/VGSS.386) | 386 virtual device |
| 238 | [`OEMSETUP.INF`](disks/windows-drivers-v1.2/OEMSETUP.INF) | Windows driver install info |
| 37 742 | [`MIDIMAP.CFG`](disks/windows-drivers-v1.2/MIDIMAP.CFG) | MIDI-mapper configuration |
| 19 120 | [`G2S.EXE`](disks/windows-drivers-v1.2/G2S.EXE) ([`.DOC`](disks-md/windows-drivers-v1.2/G2S.DOC.md)) | the Sound Blaster emulator (bundled) |

### Earlier releases (pre-GSS architecture)

Two pre-release Windows driver disks survive, using the older combined
`SFGOLD.DRV` + `VGOLD.386` design:

| Version | Date | Contents | Notes |
|---------|------|----------|-------|
| v0.9 | Oct 1992 | `SFGOLD.DRV`, `VGOLD.386`, `MIXERGLD.EXE`, `MIDIMAP.CFG`, `OEMSETUP.INF` ([README](disks-md/windows-drivers-v0.9/README.TXT.md)) | first Windows driver |
| v0.9b | Dec 1992 | same set, updated ([README](disks-md/windows-drivers-v0.9b/README.TXT.md)) | adds `[sfgold.drv]` `system.ini` tuning options |

Browse the raw files:
[v0.9](https://github.com/slartibardfast/adlib_gold/tree/main/manual/src/disks/windows-drivers-v0.9) ·
[v0.9b](https://github.com/slartibardfast/adlib_gold/tree/main/manual/src/disks/windows-drivers-v0.9b) ·
[v1.2](https://github.com/slartibardfast/adlib_gold/tree/main/manual/src/disks/windows-drivers-v1.2).
