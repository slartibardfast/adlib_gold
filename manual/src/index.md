# Ad Lib Gold Developer Toolkit

*A searchable reproduction of the **Ad Lib Gold Developer Toolkit, Version 1.01** developer manual.*

The Ad Lib Gold 1000/2000 was a 1992 16-bit sound card built around the Yamaha
**OPL3 (YMF262)** FM synthesizer, a **MMA** digital-audio controller, an optional
**YM7128 (SP2)** surround processor, a MIDI/game port and an optional SCSI interface.
This toolkit documents the hardware, the bundled DOS applications, the DOS driver
API, and the low-level register interface needed to program the card directly.

## How this book is organized

| Section | What it covers |
|---------|----------------|
| [Release Notes](release-notes.md) | v1.01 addenda and driver caveats |
| [Introduction](introduction.md) | Copyright, licensing, contents |
| [Chapter 2 - Quick Start](ch02-quick-start.md) | Installing hardware and software |
| [Chapter 3 - Gold Hardware](ch03-gold-hardware.md) | Card layout, jumpers, connectors, surround module |
| [Chapter 4 - Software Applications](ch04-software-applications.md) | Setup, test, mixer, jukebox, instrument/sample makers |
| [Chapter 5 - DOS Software Drivers](ch05-dos-drivers.md) | Control, FM, Wave, Timer and MIDI driver APIs |
| [Chapter 7 - Low-Level Programming](ch07-low-level.md) | MMA / OPL3 / mixer register reference |
| [Appendix: GSS](appendix-gss.md) | Gold Sound Standard specification |
| [Appendix: SP2](appendix-sp2.md) | YM7128 surround processor datasheet |
| [Appendix D](appendix-d-files.md) | List of installed files |

## About this reproduction

This edition was produced with [ocr.z.ai](https://ocr.z.ai) (GLM-OCR), which
performed optical character recognition over the original scanned PDF, then
rebuilding the result as an mdBook. The **technical content is preserved as
written** in 1992; only structure and formatting were modernized:

- the six OCR fragments were reassembled into the manual's original chapters;
- OCR mojibake (for example `AdLib®`) was repaired;
- C function prototypes were placed in code blocks and register listings tidied;
- cross-links were added between the manual and the driver source in this repository.

Because it is a historical document, some names, phone numbers and addresses
(such as the Ad Lib Developer Support line) are of purely historical interest.
Text recovered by OCR may contain occasional transcription errors; consult the
original scan where exactness matters.

> This SDK lives in the
> [`slartibardfast/adlib_gold`](https://github.com/slartibardfast/adlib_gold)
> repository.
