/*****************************************************************************
 * wavedsp.h - pure DSP decisions for the WaveCyclic path (call/0011).
 *
 * The nearest-supported-rate mapping (the downsampling decision) and the
 * triangular-PDF dither that reduces 16-bit samples to the 12-bit DAC. These
 * are the pure, fixed-point, no-FPU parts of the path, kept free of DDK types
 * so they compile into the driver and also into a userspace test that
 * discharges the wave.allium obligations. See spec/wave.allium.
 *****************************************************************************/
#ifndef _WAVEDSP_H_
#define _WAVEDSP_H_

/* The discrete sample rates the MMA clocks, in Hz (algwave.h, the SDK). */
#define WAVEDSP_NUM_RATES 4
static const unsigned long WaveDspRates[WAVEDSP_NUM_RATES] =
    { 44100, 22050, 11025, 7350 };

/*
 * Map a requested rate to the nearest supported hardware rate. A rate that is
 * already a hardware rate maps to itself; any other rate resamples to the
 * closest one (call/0011). Integer-only.
 */
static unsigned long
NearestSupportedRate(unsigned long requested)
{
    unsigned long best = WaveDspRates[0];
    unsigned long bestDist =
        (requested > best) ? (requested - best) : (best - requested);
    int i;
    for (i = 1; i < WAVEDSP_NUM_RATES; i++)
    {
        unsigned long r = WaveDspRates[i];
        unsigned long d = (requested > r) ? (requested - r) : (r - requested);
        if (d < bestDist) { bestDist = d; best = r; }
    }
    return best;
}

/* Advance a 16-bit Galois LFSR by one step (maximal length). */
static unsigned short
WaveDspLfsrNext(unsigned short s)
{
    unsigned short bit =
        (unsigned short)(((s >> 0) ^ (s >> 2) ^ (s >> 3) ^ (s >> 5)) & 1);
    return (unsigned short)((s >> 1) | (bit << 15));
}

/*
 * Apply triangular-PDF dither to a 16-bit sample and truncate to 12-bit (the
 * low four bits cleared). Two uniform values in [-8,+7] sum to a triangular PDF
 * of +/- 1 LSB at 12-bit scale, so quantization error is noise, not distortion
 * (call/0011).
 */
static short
WaveDspDither(short sample16, unsigned short *pLfsr)
{
    long r1 = (long)(*pLfsr & 0x0F) - 8;
    long r2;
    long dithered;
    *pLfsr = WaveDspLfsrNext(*pLfsr);
    r2 = (long)(*pLfsr & 0x0F) - 8;
    *pLfsr = WaveDspLfsrNext(*pLfsr);
    dithered = (long)sample16 + r1 + r2;
    if (dithered > 32767)  dithered = 32767;
    if (dithered < -32768) dithered = -32768;
    return (short)(dithered & 0xFFF0);
}

#endif /* _WAVEDSP_H_ */
