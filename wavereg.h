/*****************************************************************************
 * wavereg.h - pure derivation of the YMZ263 play/format register fields that a
 * WaveCyclic stream's channel count controls (finding: channel count never
 * programmed; call/0016). Kept free of DDK headers so a userspace test pins the
 * mapping the driver's ProgramMmaStart uses.
 *
 * Grounded in the manual (ch07) and the validated stereo trace
 * (plan/0008/stereo-mma-reference.md):
 *   Reg 09h (play/record), D5 = L, D6 = R: "Setting L or R enables output from the
 *     left or right channel respectively." A mono stream drives its one FIFO to both
 *     outputs; a stereo stream drives channel 0 to the right output and channel 1 to
 *     the left, matching the reference's index-5 values (PRC_0 sets R, PRC_1 sets L).
 *   Reg 0Ch (format), D7 = ILV: stereo is interleaving, "Channel 0 initiates the
 *     transfer ... ENB must be 1 for both channels", and ENB is the DMA-mode bit. So
 *     the interleave bit is set only on channel 0, and it takes effect only on the DMA
 *     path, never on the 16-bit PIO fill.
 *
 * Stereo is implemented over the DMA interleave path (call/0017, superseding call/0016).
 * The channel-to-output mapping is attested on hardware: the trace routes channel 0 to
 * the right output, opposite the manual's default channel naming.
 *****************************************************************************/
#ifndef _WAVEREG_H_
#define _WAVEREG_H_

/* Reg 09h (play/record control) output-enable bits. */
#define WAVE_PB_LEFT    0x20    /* D5 L: enable the left output   */
#define WAVE_PB_RIGHT   0x40    /* D6 R: enable the right output  */

/* Reg 0Ch (format/control) interleave bit. */
#define WAVE_FMT_ILV    0x80    /* D7 ILV: interleave channel 0 and channel 1 */

/*
 * The reg-09h output-enable bits for MMA channel `mma_channel` (0 or 1) of a stream of
 * `channels` channels. Mono routes its one FIFO (channel 0) to both outputs (L and R);
 * stereo drives channel 0 to the right output and channel 1 to the left, per the
 * validated reference (call/0017). The mapping is pinned by a test.
 */
static unsigned char
WaveMmaPlayChannelBits(int channels, int mma_channel)
{
    if (channels < 2)
        return (unsigned char)(WAVE_PB_LEFT | WAVE_PB_RIGHT);   /* mono: both outputs */
    return (unsigned char)((mma_channel == 0) ? WAVE_PB_RIGHT : WAVE_PB_LEFT);
}

/*
 * The reg-0Ch interleave bit for a stream of `channels` channels. Stereo interleaves
 * the two channels (ILV set on channel 0); mono does not. Interleave takes effect only
 * with the DMA engine (ENB = 1), which is why the stereo path is DMA, not the 16-bit
 * PIO fill (call/0017).
 */
static unsigned char
WaveMmaFormatInterleave(int channels)
{
    return (unsigned char)((channels >= 2) ? WAVE_FMT_ILV : 0);
}

/* Control-chip reg 13h DMA-select field shift (D6-D4, SDK register map page 7-9),
 * matching CTRL_DMA0_SEL_SHIFT (call/0029). */
#define WAVE_DMA0_SEL_SHIFT   4

/*
 * The reg-13h DMA-select bits for a PnP-assigned DMA channel. The 2-bit field only holds
 * channels 0..3; an out-of-range channel falls back to DMA 1 (a Gold 1000 default) rather
 * than silently masking to a wrong channel, mirroring the IRQ validation. Pure so a test
 * pins the mapping (finding: DMA channel validated like the IRQ).
 */
static unsigned char
WaveDmaSelectBits(int channel)
{
    unsigned char ch = (channel >= 0 && channel <= 3) ? (unsigned char)channel : 1;
    return (unsigned char)(ch << WAVE_DMA0_SEL_SHIFT);
}

#endif /* _WAVEREG_H_ */
