/*****************************************************************************
 * wavesrc.h - the WaveCyclic sample-rate converter (call/0011): band-limit an
 * off-rate 16-bit source, then resample it to the nearest hardware rate.
 *
 * Pure, fixed-point, no-FPU (kernel-safe) and no 64-bit, so it compiles into the
 * driver and into a userspace test that discharges the wave.allium ResampleOffRate
 * obligation. Two pieces, per call/0011:
 *   WaveSrcHalfBand - a precomputed Q15 windowed-sinc low-pass (unity DC gain)
 *     that band-limits to ~Fs/4 ahead of a /2 decimation (the clean integer chain
 *     44100 -> 22050 -> 11025), so aliasing stays out of the audible band.
 *   WaveSrcResample - a fixed-point phase-accumulator resampler with linear
 *     interpolation (the low-cost fallback call/0011 names) for an arbitrary ratio.
 * The nearest hardware rate is chosen by NearestSupportedRate (wavedsp.h); the DAC
 * bit-depth reduction is WaveDspDither (wavedsp.h).
 *****************************************************************************/
#ifndef _WAVESRC_H_
#define _WAVESRC_H_

/*
 * Half-band anti-alias low-pass, 15-tap Hamming-windowed sinc at cutoff Fs/4,
 * Q15, normalized so the taps sum to 32768 (unity DC gain). Precomputed once.
 */
#define WAVESRC_FIR_TAPS 15
static const short WaveSrcFir[WAVESRC_FIR_TAPS] =
    { -120, 0, 530, 0, -2242, 0, 9993, 16446, 9993, 0, -2242, 0, 530, 0, -120 };

/*
 * Apply the anti-alias low-pass to `count` samples, writing `count` filtered
 * samples to out (zero-padded at the edges). Unity DC gain, so a constant signal
 * passes unchanged; content above ~Fs/4 is strongly attenuated.
 */
static void
WaveSrcHalfBand(const short *in, int count, short *out)
{
    int  n, k, j;
    long acc, s;

    for (n = 0; n < count; n++)
    {
        acc = 0;
        for (k = 0; k < WAVESRC_FIR_TAPS; k++)
        {
            j = n + k - (WAVESRC_FIR_TAPS / 2);
            s = (j >= 0 && j < count) ? (long)in[j] : 0;
            acc += s * (long)WaveSrcFir[k];
        }
        acc >>= 15;
        if (acc > 32767)  acc = 32767;
        if (acc < -32768) acc = -32768;
        out[n] = (short)acc;
    }
}

/*
 * Linearly interpolate between neighbouring samples s0 and s1 at the Q16 fraction
 * f (0..0xFFFF); f == 0 returns s0. The fraction is taken to Q15 first so the
 * product stays inside a signed 32-bit long (16-bit sample difference times a
 * 15-bit fraction). This is the per-sample step both the block resampler and the
 * driver's streaming PIO fill use, so the arithmetic has one tested definition.
 */
static short
WaveSrcInterp(short s0, short s1, unsigned long f)
{
    long f15 = (long)((f & 0xFFFF) >> 1);
    long v   = (long)s0 + ((((long)s1 - (long)s0) * f15) >> 15);
    if (v > 32767)  v = 32767;
    if (v < -32768) v = -32768;
    return (short)v;
}

/*
 * Advance a byte offset within a cyclic DMA buffer by `advance` bytes, wrapping at
 * the buffer end. The 16-bit PIO service loop steps its read/write position by whole
 * samples on each FIFO service and wraps here, so the reported position always stays
 * within [0, bufSize) and never reads past the buffer (spec ServiceAdvancesPosition).
 * bufSize == 0 (unsized buffer) leaves the position at 0.
 */
static unsigned long
WaveWrapPosition(unsigned long pos, unsigned long advance, unsigned long bufSize)
{
    if (bufSize == 0)
        return 0;
    pos += advance;
    while (pos >= bufSize)
        pos -= bufSize;
    return pos;
}

/*
 * Resample `inCount` samples from inRate to outRate, writing at most outCap
 * samples to out; returns the count written. A phase accumulator steps through
 * the input at inRate/outRate samples per output sample (Q16), linearly
 * interpolating between neighbours. Equal rates copy through. Integer-only and
 * overflow-safe for rates up to 65535 Hz (inRate << 16 fits in 32 bits, and the
 * Q15 interpolation product stays inside a signed 32-bit long).
 */
static int
WaveSrcResample(const short *in, int inCount, unsigned long inRate,
                unsigned long outRate, short *out, int outCap)
{
    unsigned long step, phase;
    int   n, idx;
    short s0, s1;

    if (inCount <= 0 || inRate == 0 || outRate == 0 || outCap <= 0)
        return 0;

    if (inRate == outRate)                       /* identity: copy through */
    {
        n = (inCount < outCap) ? inCount : outCap;
        for (idx = 0; idx < n; idx++)
            out[idx] = in[idx];
        return n;
    }

    step  = (inRate << 16) / outRate;            /* input samples per output, Q16 */
    phase = 0;
    n = 0;
    while (n < outCap)
    {
        idx = (int)(phase >> 16);
        if (idx >= inCount)
            break;

        s0  = in[idx];
        s1  = (idx + 1 < inCount) ? in[idx + 1] : (short)s0;
        out[n++] = WaveSrcInterp(s0, s1, phase & 0xFFFF);

        phase += step;
    }
    return n;
}

#endif /* _WAVESRC_H_ */
