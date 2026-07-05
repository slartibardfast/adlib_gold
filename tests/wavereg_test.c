/*
 * wavereg_test.c - userspace test for the play/format register-field derivation
 * (wavereg.h) against the manual ch07 channel semantics. No hardware needed.
 *   cc -I.. -o wavereg_test wavereg_test.c && ./wavereg_test
 *
 * Manual ch07: reg 09h D5=L D6=R enable the left/right outputs; reg 0Ch D7=ILV
 * interleaves channel 0 and channel 1 (a stereo, DMA-only feature). Mono routes to
 * both outputs with interleave off; stereo drives channel 0 to the right output and
 * channel 1 to the left with interleave on channel 0, per the validated reference
 * (plan/0008/stereo-mma-reference.md, call/0017).
 */
#include <stdio.h>
#include "../wavereg.h"

static int failures = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); failures++; } } while (0)

/* Discharges wave.allium MonoRoutesBothOutputs and StereoInterleavesChannels: the
 * channel count and MMA channel determine the programmed play/format routing. */
static void test_wave_reg(void)
{
    /* Mono routes its one FIFO (channel 0) to both outputs, interleave off. */
    CHECK(WaveMmaPlayChannelBits(1, 0) == (WAVE_PB_LEFT | WAVE_PB_RIGHT),
          "mono enables both the left and right outputs");
    CHECK(WaveMmaFormatInterleave(1) == 0,
          "mono does not interleave");

    /* Stereo drives channel 0 to the right output and channel 1 to the left, and sets
     * the interleave bit on channel 0 (the validated reference routing, call/0017). */
    CHECK(WaveMmaPlayChannelBits(2, 0) == WAVE_PB_RIGHT,
          "stereo channel 0 drives the right output");
    CHECK(WaveMmaPlayChannelBits(2, 1) == WAVE_PB_LEFT,
          "stereo channel 1 drives the left output");
    CHECK(WaveMmaFormatInterleave(2) == WAVE_FMT_ILV,
          "stereo sets the interleave bit");

    /* The two stereo channels route to different outputs, and stereo differs from mono,
     * so a regression that swaps a channel or ignores the count is caught. */
    CHECK(WaveMmaPlayChannelBits(2, 0) != WaveMmaPlayChannelBits(2, 1),
          "the two stereo channels drive different outputs");
    CHECK(WaveMmaPlayChannelBits(1, 0) != WaveMmaPlayChannelBits(2, 0),
          "the play routing depends on the channel count");
    CHECK(WaveMmaFormatInterleave(1) != WaveMmaFormatInterleave(2),
          "the interleave bit depends on the channel count");
}

/* Discharges wave.allium DmaChannelValidated: a supported DMA channel maps to its
 * select bits; an out-of-range channel falls back to DMA 1 rather than a wrong channel. */
static void test_wave_dma(void)
{
    CHECK(WaveDmaSelectBits(0) == (0 << WAVE_DMA0_SEL_SHIFT), "DMA 0 select bits");
    CHECK(WaveDmaSelectBits(1) == (1 << WAVE_DMA0_SEL_SHIFT), "DMA 1 select bits");
    CHECK(WaveDmaSelectBits(3) == (3 << WAVE_DMA0_SEL_SHIFT), "DMA 3 select bits");

    /* Out of range (and negative) falls back to DMA 1, never a masked wrong channel. */
    CHECK(WaveDmaSelectBits(5) == (1 << WAVE_DMA0_SEL_SHIFT), "DMA 5 falls back to 1");
    CHECK(WaveDmaSelectBits(7) == (1 << WAVE_DMA0_SEL_SHIFT), "DMA 7 falls back to 1");
    CHECK(WaveDmaSelectBits(-1) == (1 << WAVE_DMA0_SEL_SHIFT), "negative falls back to 1");
}

int main(void)
{
    test_wave_reg();
    test_wave_dma();
    if (failures == 0) { printf("wavereg_test: all checks passed\n"); return 0; }
    printf("wavereg_test: %d failure(s)\n", failures);
    return 1;
}
