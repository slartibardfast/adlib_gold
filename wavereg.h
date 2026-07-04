/*****************************************************************************
 * wavereg.h - pure derivation of the YMZ263 play/format register fields that a
 * WaveCyclic stream's channel count controls (finding: channel count never
 * programmed; call/0016). Kept free of DDK headers so a userspace test pins the
 * mapping the driver's ProgramMmaStart uses.
 *
 * Grounded in the manual (ch07):
 *   Reg 09h (play/record), D5 = L, D6 = R: "Setting L or R enables output from the
 *     left or right channel respectively." A mono stream drives its one FIFO to both
 *     outputs; a stereo stream drives channel 0 to the left output and channel 1 to
 *     the right.
 *   Reg 0Ch (format), D7 = ILV: stereo is interleaving, "Channel 0 initiates the
 *     transfer ... ENB must be 1 for both channels", and ENB is the DMA-mode bit. So
 *     the interleave bit is set only for stereo, and it takes effect only on the DMA
 *     path, never on the 16-bit PIO fill.
 *
 * Digital-audio output is mono today (call/0016): the driver rejects a stereo format
 * before a stream reaches ProgramMmaStart, so it only ever uses the mono result here.
 * The stereo mapping is defined and tested as the target for the deferred dual-channel
 * path, which also needs channel-1 programming the driver does not have yet.
 *****************************************************************************/
#ifndef _WAVEREG_H_
#define _WAVEREG_H_

/* Reg 09h (play/record control) output-enable bits. */
#define WAVE_PB_LEFT    0x20    /* D5 L: enable the left output   */
#define WAVE_PB_RIGHT   0x40    /* D6 R: enable the right output  */

/* Reg 0Ch (format/control) interleave bit. */
#define WAVE_FMT_ILV    0x80    /* D7 ILV: interleave channel 0 and channel 1 */

/*
 * The reg-09h output-enable bits for a stream of `channels` channels. Mono routes its
 * one FIFO to both outputs (L and R); stereo drives channel 0 to the left output alone
 * (channel 1 carries the right, programmed separately on the deferred path, call/0016).
 */
static unsigned char
WaveMmaPlayChannelBits(int channels)
{
    return (unsigned char)((channels >= 2) ? WAVE_PB_LEFT
                                           : (WAVE_PB_LEFT | WAVE_PB_RIGHT));
}

/*
 * The reg-0Ch interleave bit for a stream of `channels` channels. Stereo interleaves
 * the two channels (ILV set); mono does not. Interleave takes effect only with the DMA
 * engine (ENB = 1), which is why the deferred stereo path (call/0016) is the 8-bit DMA
 * path, not the 16-bit PIO fill.
 */
static unsigned char
WaveMmaFormatInterleave(int channels)
{
    return (unsigned char)((channels >= 2) ? WAVE_FMT_ILV : 0);
}

#endif /* _WAVEREG_H_ */
