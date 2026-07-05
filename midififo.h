/*****************************************************************************
 * midififo.h - pure decisions for the YMZ263 MIDI UART FIFO handling, so the
 * transmit flow control and the receive ring-buffer admission are the single
 * tested source of truth (spec/midi.allium). No hardware or kernel dependency,
 * so it compiles into the driver and into a userspace test.
 *****************************************************************************/
#ifndef _MIDIFIFO_H_
#define _MIDIFIFO_H_

#define MIDI_TX_FIFO_DEPTH   16     /* YMZ263 transmit FIFO depth (spec tx_fifo_depth) */

/*
 * How many bytes to write to the transmit FIFO now. The FIFO holds MIDI_TX_FIFO_DEPTH
 * bytes and TRQ signals it is empty (manual ch07), so a burst is safe only when the FIFO
 * is empty: write min(length, depth) then, and otherwise write nothing and let the caller
 * retry once it drains. Never returns more than the FIFO can hold, so a write cannot
 * overrun it (spec TransmitOnlyWithSpace).
 */
static unsigned long
MidiTxCount(int fifo_empty, unsigned long length)
{
    if (!fifo_empty)
        return 0;
    return (length < MIDI_TX_FIFO_DEPTH) ? length : MIDI_TX_FIFO_DEPTH;
}

/*
 * Whether a received byte fits the software receive ring of `size` slots at the given head
 * and tail. A byte is enqueued at tail and tail advances; the ring is full when advancing
 * tail would meet head (one slot is kept free to tell full from empty). Returns nonzero if
 * the byte fits, so no byte is lost until the ring is genuinely full (spec
 * ReceiveKeepsUntilFull).
 */
static int
MidiRingFits(unsigned long head, unsigned long tail, unsigned long size)
{
    unsigned long next = (tail + 1) % size;
    return next != head;
}

#endif /* _MIDIFIFO_H_ */
