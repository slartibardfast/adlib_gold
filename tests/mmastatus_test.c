/*
 * mmastatus_test.c - userspace test for the YMZ263 MMA status-register decode
 * (mmastatus.h) against the manual ch07 bit layout. No hardware needed.
 *   cc -I.. -o mmastatus_test mmastatus_test.c && ./mmastatus_test
 *
 * Manual ch07, status read at 38CH:  D7 OV  D6 T2  D5 T1  D4 T0
 *                                     D3 TRQ D2 RRQ D1 FIF1 D0 FIF0
 */
#include <stdio.h>
#include "../mmastatus.h"

static int failures = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); failures++; } } while (0)

/* Discharges wave.allium PlaybackFifoServicesRender (and the sibling flag decodes):
 * exercises MmaStatusChannel0Ready and the other status predicates. */
static void test_mma_status(void)
{
    /* Each flag decodes from exactly its documented bit. */
    CHECK(MmaStatusChannel0Ready(0x01), "FIF0 (0x01) is the channel-0 FIFO request");
    CHECK(MmaStatusChannel1Ready(0x02), "FIF1 (0x02) is the channel-1 FIFO request");
    CHECK(MmaStatusMidiRxReady(0x04),   "RRQ (0x04) is MIDI receive");
    CHECK(MmaStatusMidiTxReady(0x08),   "TRQ (0x08) is MIDI transmit-empty");
    CHECK(MmaStatusTimer0Elapsed(0x10), "T0 (0x10) is the Timer-0 elapse");
    CHECK(MmaStatusOverrun(0x80),       "OV (0x80) is FIFO overrun");

    /* Each decoder ignores the other bits (no cross-talk between flags). */
    CHECK(!MmaStatusChannel0Ready(0xFE), "channel 0 ignores every bit but FIF0");
    CHECK(!MmaStatusChannel1Ready(0xFD), "channel 1 ignores every bit but FIF1");
    CHECK(!MmaStatusMidiRxReady(0xFB),   "MIDI rx ignores every bit but RRQ");
    CHECK(!MmaStatusMidiTxReady(0xF7),   "MIDI tx ignores every bit but TRQ");
    CHECK(!MmaStatusTimer0Elapsed(0xEF), "timer 0 ignores every bit but T0");
    CHECK(!MmaStatusOverrun(0x7F),       "overrun ignores every bit but OV");

    /* The old code read FIF0/FIF1 as TRQ/PRQ and TRQ as a timer bit; guard the
     * corrected assignment so a regression to the mislabeled values is caught. */
    CHECK(MMA_STATUS_FIF0 == 0x01 && MMA_STATUS_FIF1 == 0x02, "playback FIFO bits");
    CHECK(MMA_STATUS_RRQ == 0x04 && MMA_STATUS_TRQ == 0x08,   "MIDI rx/tx bits");

    /* A cleared status requests nothing. */
    CHECK(!MmaStatusChannel0Ready(0x00) && !MmaStatusMidiRxReady(0x00),
          "an idle status requests no service");
}

int main(void)
{
    test_mma_status();
    if (failures == 0) { printf("mmastatus_test: all checks passed\n"); return 0; }
    printf("mmastatus_test: %d failure(s)\n", failures);
    return 1;
}
