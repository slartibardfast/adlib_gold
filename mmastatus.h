/*****************************************************************************
 * mmastatus.h - pure decode of the YMZ263 (MMA) status register, so the bit
 * semantics are one tested source of truth (wave.allium).
 *
 * The status is the DIRECT read of the register-select port at base+4 (38CH).
 * It is NOT reached through the register-select/data protocol: writing an index
 * to base+4 and reading base+5 returns that register's data, never the status
 * (manual ch07, "Status Register"). CAdapterCommon::ReadMMAStatus reads the port;
 * the ISR and the MIDI service path decode the byte through the helpers here.
 *
 * Read at 38CH (manual ch07):
 *   D7  D6  D5  D4  D3   D2   D1    D0
 *   OV  T2  T1  T0  TRQ  RRQ  FIF1  FIF0
 * The flags are level-sensitive: the register-08h/0Dh mask bits gate only the
 * hardware IRQ line, never these status bits (manual ch07). A flag stays set
 * while its condition holds, so a read does not clear it.
 *****************************************************************************/
#ifndef _MMASTATUS_H_
#define _MMASTATUS_H_

#define MMA_STATUS_FIF0     0x01    /* PCM/ADPCM FIFO channel 0 at its FIFO-INT level */
#define MMA_STATUS_FIF1     0x02    /* PCM/ADPCM FIFO channel 1 at its FIFO-INT level */
#define MMA_STATUS_RRQ      0x04    /* MIDI receive FIFO has data                     */
#define MMA_STATUS_TRQ      0x08    /* MIDI transmit FIFO empty (ready for a byte)    */
#define MMA_STATUS_T0       0x10    /* Timer-0 overflow                              */
#define MMA_STATUS_T1       0x20    /* Timer-1 overflow                              */
#define MMA_STATUS_T2       0x40    /* Timer-2 overflow                              */
#define MMA_STATUS_OV       0x80    /* FIFO overrun (data lost)                      */

/* The channel-0 playback FIFO has reached its interrupt level and needs a refill
 * (spec PlaybackFifoServicesRender): this drives the wave render service. */
static int
MmaStatusPlaybackReady(unsigned char status)
{
    return (status & MMA_STATUS_FIF0) != 0;
}

/* The channel-1 FIFO needs service (the capture / second-channel path). */
static int
MmaStatusCaptureReady(unsigned char status)
{
    return (status & MMA_STATUS_FIF1) != 0;
}

/* A MIDI byte has arrived in the receive FIFO (spec: the ISR drains it). */
static int
MmaStatusMidiRxReady(unsigned char status)
{
    return (status & MMA_STATUS_RRQ) != 0;
}

/* The MIDI transmit FIFO is empty, so a byte may be written (spec: TX flow control). */
static int
MmaStatusMidiTxReady(unsigned char status)
{
    return (status & MMA_STATUS_TRQ) != 0;
}

/* A FIFO overran and data was lost; the caller resets the affected FIFO. */
static int
MmaStatusOverrun(unsigned char status)
{
    return (status & MMA_STATUS_OV) != 0;
}

#endif /* _MMASTATUS_H_ */
