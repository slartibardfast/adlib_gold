/*
 * wavedsp_test.c - userspace test for the pure WaveCyclic DSP decisions
 * (wavedsp.h). Discharges the wave.allium behavioural obligations that do not
 * need hardware: the nearest-rate mapping (ResampleOffRate) and the dither
 * (DitherWideFormat). Build and run with any C compiler:
 *   cc -I.. -o wavedsp_test wavedsp_test.c && ./wavedsp_test
 */
#include <stdio.h>
#include "../wavedsp.h"

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s\n", msg); failures++; } } while (0)

/* ResampleOffRate: a hardware rate maps to itself; an off-rate maps to the nearest. */
static void test_nearest_rate(void)
{
    CHECK(NearestSupportedRate(44100) == 44100, "44100 -> 44100");
    CHECK(NearestSupportedRate(22050) == 22050, "22050 -> 22050");
    CHECK(NearestSupportedRate(11025) == 11025, "11025 -> 11025");
    CHECK(NearestSupportedRate(7350)  == 7350,  "7350 -> 7350");
    CHECK(NearestSupportedRate(48000) == 44100, "48000 -> nearest 44100");
    CHECK(NearestSupportedRate(32000) == 22050, "32000 -> nearest 22050");
    CHECK(NearestSupportedRate(16000) == 11025, "16000 -> nearest 11025");
    CHECK(NearestSupportedRate(8000)  == 7350,  "8000 -> nearest 7350");
    CHECK(NearestSupportedRate(4000)  == 7350,  "4000 -> nearest 7350");
}

/* DitherWideFormat: the dithered 16-bit sample has its low 4 bits cleared (12-bit),
 * stays in range, and lands within the +/- 1 LSB dither window of the source. */
static void test_dither(void)
{
    unsigned short lfsr = 0xACE1;
    int s;
    for (s = -32000; s <= 32000; s += 137)
    {
        short out = WaveDspDither((short)s, &lfsr);
        long diff = (long)out - (long)s;
        CHECK((out & 0x0F) == 0, "dither clears low 4 bits (12-bit)");
        CHECK(out >= -32768 && out <= 32767, "dither in 16-bit range");
        CHECK(diff <= 16 && diff >= -32, "dither within the LSB+truncation window");
    }
    /* the LFSR never collapses to zero from a non-zero state (maximal length). */
    {
        unsigned short x = 0x0001;
        int i, hitZero = 0;
        for (i = 0; i < 65535; i++) { x = WaveDspLfsrNext(x); if (x == 0) hitZero = 1; }
        CHECK(!hitZero, "LFSR never reaches zero");
    }
}

int main(void)
{
    test_nearest_rate();
    test_dither();
    if (failures == 0) { printf("wavedsp_test: all checks passed\n"); return 0; }
    printf("wavedsp_test: %d failure(s)\n", failures);
    return 1;
}
