# Appendix E — Program Disks (as shipped)

The Ad Lib Gold **Program Disks, version 1.00** (April 1992) are the end-user
software that shipped with the card. This appendix preserves them **exactly as
distributed**: the authentic 720 KB floppy images, every file byte-for-byte in
its original DOS (CP437 / CRLF) form, and a UTF-8 rendering of each text file.

The *expanded* result of running the installer is documented separately in
[Appendix F — Software (as installed)](appendix-f-as-installed.md); the
compression format of the `GOLD*.CMP` archive is reverse-engineered in
[Appendix H — the "Ad Lib Comp." format](appendix-h-goldcmp-format.md).

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
