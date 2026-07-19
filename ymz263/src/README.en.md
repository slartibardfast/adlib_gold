# YMZ263B (MMA) — Overview

> **Reference translation.** This English page is a convenience
> translation. The authoritative document is the
> [original Japanese](./README.md); the [datasheet itself](./datasheet.en.md)
> carries the same notice.

[→ 日本語](./README.md)

This mdBook is a searchable reproduction of the Yamaha **YMZ263B** (Multimedia Audio & Game Interface Controller, commonly **MMA**) datasheet. The original Japanese PDF was OCR'd via GLM-OCR, the OCR errors were repaired, and the original structure, register tables, electrical characteristics, and timing diagrams are preserved.

## Relationship to companion documents

- The Ad Lib Gold Developer Toolkit manual's [Chapter 7 (Low-Level Programming)](https://slartibardfast.github.io/adlib_gold/ch07-low-level.html) is a secondary source: an excerpt and English translation of this chip's MMA section. This book is the primary source (the original Yamaha document).
- The Ad Lib Gold WDM driver (`adlibgold.sys`) treats this datasheet as the authoritative specification for register programming.

## Contents

- [Datasheet (Japanese original)](./datasheet.md) — the cleaned Japanese source.
- [Datasheet (English translation)](./datasheet.en.md) — an English translation placed side-by-side. Technical terms (pin names, register names, signal names) retain their original notation.

## Figure index

| Figure | Content | File |
|---|---|---|
| Block diagram | Chip internal structure | `images/block-diagram.png` |
| Pin layout | 64-pin QFP | `images/pin-layout.png` |
| GP0–7 input equivalent circuit | Game port | `images/game-port-circuit.png` |
| Fig A-1 | Input clock timing | `images/timing-a1-input-clock.png` |
| Fig A-2 | Reset pulse | `images/timing-a2-reset.png` |
| Fig A-3 | Address / data write timing | `images/timing-a3-write.png` |
| Fig A-4 | Status / data read timing | `images/timing-a4-read.png` |
| Fig A-5 | DMA read timing | `images/timing-a5-dma-read.png` |
| Fig A-6 | DMA write timing | `images/timing-a6-dma-write.png` |
| Package outline (1) | 64-pin QFP dimensions | `images/package-1.png` |
| Package outline (2) | 64-pin QFP dimensions | `images/package-2.png` |

## Provenance

- Source PDF: [theretroweb.com — ymz263b-67b9e7aead79f115900527.pdf](https://theretroweb.com/chip/documentation/ymz263b-67b9e7aead79f115900527.pdf)
- OCR: GLM-OCR (2026-07-19)
- The Yamaha Corporation document is the original; no technical content has been altered. Only OCR error repair and reformatting to mdBook structure were performed.
