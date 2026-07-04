/*
 * wavesrc_test.c - userspace test for the WaveCyclic sample-rate converter
 * (wavesrc.h), the off-rate resample path (call/0011, wave.allium ResampleOffRate).
 * Checks the resampler and the anti-alias FIR without hardware.
 *   cc -I.. -o wavesrc_test wavesrc_test.c && ./wavesrc_test
 */
#include <stdio.h>
#include <stdlib.h>
#include "../wavesrc.h"

static int failures = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); failures++; } } while (0)

/* The anti-alias FIR: unity DC gain, strong high-frequency rejection. */
static void test_wavesrc_fir(void)
{
    short in[64], out[64];
    long sum;
    int i;

    /* DC preservation: a constant passes through unchanged (unity gain). */
    for (i = 0; i < 64; i++) in[i] = 1000;
    WaveSrcHalfBand(in, 64, out);
    for (i = 16; i < 48; i++)                 /* interior, away from zero-padded edges */
        CHECK(out[i] == 1000, "FIR passes DC unchanged");

    /* Nyquist rejection: an alternating +/- full-scale tone is strongly attenuated. */
    for (i = 0; i < 64; i++) in[i] = (i & 1) ? 20000 : -20000;
    WaveSrcHalfBand(in, 64, out);
    sum = 0;
    for (i = 24; i < 40; i++) sum += (out[i] < 0) ? -out[i] : out[i];
    CHECK((sum / 16) < 400, "FIR rejects a Nyquist-rate tone");
}

/* The shared interpolation primitive: endpoints exact, midpoint the average. */
static void test_wavesrc_interp(void)
{
    CHECK(WaveSrcInterp(1000, 2000, 0x0000) == 1000, "fraction 0 returns s0");
    CHECK(WaveSrcInterp(1000, 2000, 0x8000) == 1500, "fraction 1/2 returns the midpoint");
    CHECK(WaveSrcInterp(-32768, 32767, 0x0000) == -32768, "extreme endpoint exact");
    CHECK(WaveSrcInterp(500, 500, 0x4000) == 500, "equal neighbours interpolate flat");
}

static void test_wavesrc_resample(void)
{
    short in[200], out[200];
    int i, n;

    for (i = 0; i < 200; i++) in[i] = (short)(-30000 + 300 * i);   /* a linear ramp */

    /* Identity: equal rates copy through exactly. */
    n = WaveSrcResample(in, 100, 22050, 22050, out, 200);
    CHECK(n == 100, "identity keeps the sample count");
    for (i = 0; i < 100; i++) CHECK(out[i] == in[i], "identity copies exactly");

    /* Downsample 44100 -> 22050 (ratio 2): count halves, and because the step is
     * exactly 2.0 the output is every other input sample (linear interp is exact
     * on a ramp, and lands on integer indices). */
    n = WaveSrcResample(in, 100, 44100, 22050, out, 200);
    CHECK(n == 50, "44100->22050 halves the count");
    for (i = 0; i < 50; i++) CHECK(out[i] == in[2 * i], "/2 decimation picks every other sample");

    /* Arbitrary ratio 48000 -> 44100: count scales, output stays in range and the
     * ramp stays monotonic non-decreasing (a linear source resamples to a line). */
    n = WaveSrcResample(in, 160, 48000, 44100, out, 200);
    CHECK(n >= 145 && n <= 148, "48000->44100 scales the count (~147)");
    for (i = 1; i < n; i++)
        CHECK(out[i] >= out[i - 1], "a ramp resamples to a monotonic ramp");
    for (i = 0; i < n; i++)
        CHECK(out[i] >= -32768 && out[i] <= 32767, "output stays in 16-bit range");

    /* DC preservation across a resample: a constant resamples to the same constant. */
    for (i = 0; i < 100; i++) in[i] = 5000;
    n = WaveSrcResample(in, 100, 44100, 11025, out, 200);
    for (i = 0; i < n; i++) CHECK(out[i] == 5000, "resample preserves DC");

    /* outCap is honoured: never write past the destination. */
    n = WaveSrcResample(in, 100, 44100, 22050, out, 10);
    CHECK(n <= 10, "resample honours the output capacity");
}

/* The cyclic position advance the 16-bit PIO service loop uses (spec
 * ServiceAdvancesPosition): moves forward, wraps at the buffer end, stays in range. */
static void test_wave_wrap(void)
{
    /* A plain advance that does not reach the end. */
    CHECK(WaveWrapPosition(0, 2, 256) == 2, "advance without wrap");
    CHECK(WaveWrapPosition(100, 96, 256) == 196, "advance stays below bufSize");

    /* Advancing to exactly the end wraps to 0. */
    CHECK(WaveWrapPosition(254, 2, 256) == 0, "reaching the end wraps to 0");

    /* Advancing past the end wraps within range. */
    CHECK(WaveWrapPosition(250, 12, 256) == 6, "past-end wraps by one buffer");

    /* The result is always a valid in-range offset, over a full sweep. */
    {
        unsigned long p = 0;
        int i;
        for (i = 0; i < 1000; i++)
        {
            p = WaveWrapPosition(p, 2, 200);
            CHECK(p < 200, "position stays within [0, bufSize)");
        }
    }

    /* An unsized buffer parks the position at 0 rather than dividing by zero. */
    CHECK(WaveWrapPosition(50, 2, 0) == 0, "unsized buffer stays at 0");
}

/* The block-size guard (spec ResampleBoundedBlock): a block larger than
 * WAVESRC_MAX_BLOCK is bounded to a safe size so the Q16 phase accumulator cannot wrap
 * past 65536 and corrupt the read index. The /2 downsample steps by exactly two input
 * samples per output, so the guard caps the consumed input at WAVESRC_MAX_BLOCK and the
 * output is exactly half that; without the guard the wrap would run the loop out to
 * outCap on garbage indices. */
static void test_wavesrc_bound(void)
{
    const int over = 70000;                        /* > 65536, past the wrap point */
    short *in  = (short *)malloc(over * sizeof(short));
    short *out = (short *)malloc(over * sizeof(short));
    int i, n;

    if (!in || !out) { printf("FAIL: bound-test allocation\n"); failures++; free(in); free(out); return; }

    for (i = 0; i < over; i++) in[i] = (short)((i & 0x7FFF) - 16384);  /* in-range sawtooth */

    n = WaveSrcResample(in, over, 44100, 22050, out, over);
    CHECK(n == WAVESRC_MAX_BLOCK / 2, "over-large block is bounded to WAVESRC_MAX_BLOCK");
    for (i = 0; i < n; i++)
        CHECK(out[i] >= -32768 && out[i] <= 32767, "bounded output stays in 16-bit range");

    free(in);
    free(out);
}

int main(void)
{
    test_wavesrc_fir();
    test_wavesrc_interp();
    test_wavesrc_resample();
    test_wave_wrap();
    test_wavesrc_bound();
    if (failures == 0) { printf("wavesrc_test: all checks passed\n"); return 0; }
    printf("wavesrc_test: %d failure(s)\n", failures);
    return 1;
}
