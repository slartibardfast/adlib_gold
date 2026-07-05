/*
 * midififo_test.c - userspace test for the pure MIDI UART FIFO decisions
 * (midififo.h): the transmit flow-control count and the receive ring admission.
 *   cc -I.. -o midififo_test midififo_test.c && ./midififo_test
 */
#include <stdio.h>
#include "../midififo.h"

static int failures = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); failures++; } } while (0)

/* Discharges midi.allium TransmitOnlyWithSpace: never write more than the FIFO holds,
 * and write nothing while the FIFO is not empty. */
static void test_midi_tx(void)
{
    /* Empty FIFO: write the whole message when it fits the depth. */
    CHECK(MidiTxCount(1, 0) == 0,  "empty FIFO, zero-length writes nothing");
    CHECK(MidiTxCount(1, 3) == 3,  "empty FIFO writes a short message whole");
    CHECK(MidiTxCount(1, 16) == 16, "empty FIFO writes a full FIFO");

    /* Never more than the FIFO depth, even for a long message (port retries the rest). */
    CHECK(MidiTxCount(1, 17) == MIDI_TX_FIFO_DEPTH, "long message is capped at the depth");
    CHECK(MidiTxCount(1, 1000) == MIDI_TX_FIFO_DEPTH, "SysEx is capped at the depth");

    /* Not empty: write nothing, so a burst cannot overrun the FIFO. */
    CHECK(MidiTxCount(0, 3) == 0,   "non-empty FIFO writes nothing");
    CHECK(MidiTxCount(0, 1000) == 0, "non-empty FIFO writes nothing for a long message");
}

/* Discharges midi.allium ReceiveKeepsUntilFull: a byte fits until the ring is full. */
static void test_midi_ring(void)
{
    /* Empty ring accepts. */
    CHECK(MidiRingFits(0, 0, 8), "empty ring accepts a byte");

    /* Fill to size-1 (one slot kept free); each push fits until the last free slot. */
    CHECK(MidiRingFits(0, 6, 8), "ring with a free slot accepts");

    /* Full: advancing tail would meet head, so the byte does not fit (dropped, not lost
     * silently before that). */
    CHECK(!MidiRingFits(0, 7, 8), "full ring (tail+1 == head) rejects");
    CHECK(!MidiRingFits(3, 2, 8), "full ring wraps: tail+1 == head rejects");

    /* Head ahead of tail (drained some): accepts again. */
    CHECK(MidiRingFits(5, 2, 8), "ring with room after draining accepts");
}

int main(void)
{
    test_midi_tx();
    test_midi_ring();
    if (failures == 0) { printf("midififo_test: all checks passed\n"); return 0; }
    printf("midififo_test: %d failure(s)\n", failures);
    return 1;
}
