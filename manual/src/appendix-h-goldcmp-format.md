# Appendix H — The "Ad Lib Comp." format

The Ad Lib Gold software ships compressed in a proprietary container,
`GOLD*.CMP`, expanded at install time by `SETUPGLD.EXE`. Because the installer
gates on the physical card (*"Gold Card not found"*) and its decompressor is
bound up with the C++ runtime, recovering the [installed
files](appendix-f-as-installed.md) meant reverse-engineering the format from the
16-bit executable. This appendix documents both the container and its codec.

## The container

Every stream begins with the ASCII magic `Ad Lib Comp.` followed by `1A 00`.
After the 14-byte header comes a marker stream:

| Marker | Meaning |
|--------|---------|
| `0xFB` + `name\0` | **push directory** (leading `\` = absolute, else a sub-directory) |
| `0xFD` | **pop directory** |
| `0xFF` + `name\0` + `u32` size + data | **file** (little-endian *compressed* size, then that many bytes) |
| `0xFC` + *disk#* | **disk boundary** — the archive spans `GOLD1/2/3.CMP` |
| `0xFE` | **end of archive** |

Concatenating the three spanned parts and walking this stream recovers the full
directory tree — **133 files** across `\GOLD`, `\GOLD\DRIVERS` and `\GOLD\SMP`.
Note the per-file header stores only the *compressed* size; the decoder runs
until its input is exhausted.

## The codec

Each file's payload is a bespoke **LZ77 + static-Huffman** stream (conceptually
DEFLATE-like, but Ad Lib's own bit-format), decoded LSB-first:

- **3-byte prologue:** `mode`, `offset-bits` (4–6), and a bit-buffer seed.
- **Literal/length alphabet:** a 16-entry Huffman tree selects a *category*; a
  base value plus a few extra bits give the symbol. Values `< 0x100` are literal
  bytes; `≥ 0x100` are match lengths (`length = value − 0xFE`, minimum 2). The
  category bases and extra-bit widths are:

  | category | 0–7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 |
  |----------|-----|---|---|----|----|----|----|----|----|
  | base | 0…7 | 8 | 10 | 14 | 22 | 38 | 70 | 134 | 262 |
  | extra bits | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |

- **Offsets:** a 64-entry Huffman position tree, with a *length-dependent* number
  of direct bits appended (2 bits when the match length is 2, otherwise the
  header's `offset-bits`).
- **Window:** an 8 KB sliding window, flushed 4 KB at a time; an end symbol
  (`0x306`) terminates. In *mode 0* — used by **all 133 files** — the Huffman
  tables are the fixed set above rather than being transmitted.

## How it was recovered

Static analysis with `capstone` located the decode routines in `SETUPGLD.EXE`
(the main LZ loop, `decode_char`, `decode_position`, the `getbits` bit-reader and
`make_table`) and mapped the decoder's state structure. The tables `make_table`
reads are addressed `cs:`-relative — and pinning that **code-segment base** was
the whole puzzle: the constants only make sense at the correct segment.

The winning trick was a **Kraft-equality test**. A valid canonical-Huffman
code-length table satisfies `Σ 2^(16−len) = 65536` exactly. Scanning every
plausible segment for a region where both the 16- and 64-entry length tables
pass Kraft yielded **exactly one** candidate (segment `0x35DD`) — with textbook
length tables (`[3,2,3,3,4,4,4,5,5,5,5,6,6,6,7,7]`, …). With the real tables in
hand, the genuine decode routines were driven over them (via a `unicorn` CPU
emulation, sidestepping the C++ init) and validated against the oracle:
`CTRLDRV.EXE`, which ships uncompressed on Disk 1, decoded **byte-for-byte
identically**. All 133 files then decoded in seconds.

> This is a preservation/interoperability reverse-engineering of a 1992 format
> whose author released the surrounding toolkit as freeware; it exists only to
> recover the archived software documented in
> [Appendix F](appendix-f-as-installed.md).
