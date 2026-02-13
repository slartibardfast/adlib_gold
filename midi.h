/*****************************************************************************
 * midi.h - Ad Lib Gold MIDI miniport private definitions
 *****************************************************************************
 *
 * MIDI UART miniport for the YMZ263 (MMA) MIDI subsystem.  Adapted from
 * the Windows 2000 DDK uart/miniport sample driver.
 *
 * Hardware access is delegated to the adapter common object's ReadMMA()
 * and WriteMMA() methods.  The ISR in common.cpp calls ServiceMidiISR()
 * when MMA status indicates MIDI receive data is available (RRQ bit).
 */

#ifndef _ALGMIDI_PRIVATE_H_
#define _ALGMIDI_PRIVATE_H_

#include "common.h"


/*****************************************************************************
 * MMA register indices used by MIDI (YMZ263)
 *
 * Accessed via IAdapterCommon::WriteMMA / ReadMMA (MMA Channel 0).
 * MMA_REG_STATUS is also defined in algwave.h; we redefine it here
 * so midi.cpp can compile independently.
 */
#ifndef MMA_REG_STATUS
#define MMA_REG_STATUS          0x00    /* Read: status flags (auto-clear)    */
#endif
#define MMA_REG_MIDI_CTRL       0x0D    /* MIDI and interrupt control         */
#define MMA_REG_MIDI_DATA       0x0E    /* MIDI data port (R/W FIFO)          */

/*****************************************************************************
 * Register 0Dh (MIDI and Interrupt Control) bit definitions
 *
 * From SDK Chapter 7.3 — Register Reference.
 */
#define MMA_MIDI_MSK_POV        0x80    /* D7: Mask digital overrun IRQ       */
#define MMA_MIDI_MSK_MOV        0x40    /* D6: Mask MIDI overrun IRQ          */
#define MMA_MIDI_TRS_RST        0x20    /* D5: Reset MIDI transmit circuit    */
#define MMA_MIDI_MSK_TRQ        0x10    /* D4: Mask MIDI transmit FIFO IRQ   */
#define MMA_MIDI_RCV_RST        0x08    /* D3: Reset MIDI receive circuit     */
#define MMA_MIDI_MSK_RRQ        0x04    /* D2: Mask MIDI receive FIFO IRQ    */

/*
 * Default control value: mask overrun IRQs and transmit FIFO IRQ,
 * but enable receive FIFO IRQ (MSK_RRQ = 0).
 * Transmit uses polling, not interrupts.
 */
#define MMA_MIDI_CTRL_DEFAULT   (MMA_MIDI_MSK_POV | MMA_MIDI_MSK_MOV | \
                                 MMA_MIDI_MSK_TRQ)

/*****************************************************************************
 * Software FIFO for ISR-buffered MIDI input
 *
 * Must be a power of 2 for efficient modular arithmetic.
 */
#define MIDI_INPUT_BUFFER_SIZE  256


/*****************************************************************************
 * Pin identifiers
 *
 * Match the order in MiniportPins[] (defined in midi.cpp).
 * Same layout as DDK UART sample: render in, bridge out, capture out, bridge in.
 */
enum
{
    MIDI_PIN_RENDER_STREAM = 0,     /* Render streaming (MIDI data in)    */
    MIDI_PIN_RENDER_BRIDGE,         /* Render bridge (to external MIDI)   */
    MIDI_PIN_CAPTURE_STREAM,        /* Capture streaming (MIDI data out)  */
    MIDI_PIN_CAPTURE_BRIDGE         /* Capture bridge (from external MIDI)*/
};

#define kMaxMidiCaptureStreams   1
#define kMaxMidiRenderStreams    1


/*****************************************************************************
 * Forward declarations
 */
class CMiniportMidiStreamUartAdLibGold;


/*****************************************************************************
 * CMiniportMidiUartAdLibGold
 *****************************************************************************
 * MIDI UART miniport for the Ad Lib Gold YMZ263 MIDI subsystem.
 *
 * Adapted from DDK CMiniportMidiUart.  Key differences:
 *   - No direct port I/O; all access via IAdapterCommon::ReadMMA/WriteMMA
 *   - ISR callback via IMidiMiniportAdLibGold::ServiceMidiISR()
 *   - Uses shared interrupt sync from adapter common (not its own)
 *   - MIDI reset via MMA reg 0Dh instead of MPU-401 commands
 */
class CMiniportMidiUartAdLibGold
:   public IMiniportMidi,
    public IMidiMiniportAdLibGold,
    public IPowerNotify,
    public CUnknown
{
private:
    PPORTMIDI       m_Port;                 /* Callback interface            */
    PADAPTERCOMMON  m_AdapterCommon;        /* Shared hardware access        */
    PSERVICEGROUP   m_ServiceGroup;         /* Service group for capture     */

    USHORT          m_NumCaptureStreams;     /* Active capture streams        */
    USHORT          m_NumRenderStreams;      /* Active render streams         */
    KSSTATE         m_KSStateInput;         /* Capture stream state          */

    /* Software input FIFO (filled by ISR, drained by Read) */
    UCHAR           m_InputBuffer[MIDI_INPUT_BUFFER_SIZE];
    ULONG           m_InputBufferHead;      /* Consumer index (Read)         */
    ULONG           m_InputBufferTail;      /* Producer index (ISR)          */

    POWER_STATE     m_PowerState;           /* Current power state           */

public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CMiniportMidiUartAdLibGold);

    ~CMiniportMidiUartAdLibGold();

    /*
     * IMiniport methods
     */
    STDMETHODIMP
    GetDescription
    (   OUT     PPCFILTER_DESCRIPTOR *  OutFilterDescriptor
    );
    STDMETHODIMP
    DataRangeIntersection
    (   IN      ULONG           PinId
    ,   IN      PKSDATARANGE    DataRange
    ,   IN      PKSDATARANGE    MatchingDataRange
    ,   IN      ULONG           OutputBufferLength
    ,   OUT     PVOID           ResultantFormat     OPTIONAL
    ,   OUT     PULONG          ResultantFormatLength
    )
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    /*
     * IMiniportMidi methods
     */
    STDMETHODIMP Init
    (
        IN      PUNKNOWN        UnknownAdapter  OPTIONAL,
        IN      PRESOURCELIST   ResourceList,
        IN      PPORTMIDI       Port,
        OUT     PSERVICEGROUP * ServiceGroup
    );
    STDMETHODIMP NewStream
    (
        OUT     PMINIPORTMIDISTREAM *   Stream,
        IN      PUNKNOWN                OuterUnknown    OPTIONAL,
        IN      POOL_TYPE               PoolType,
        IN      ULONG                   Pin,
        IN      BOOLEAN                 Capture,
        IN      PKSDATAFORMAT           DataFormat,
        OUT     PSERVICEGROUP *         ServiceGroup
    );
    STDMETHODIMP_(void) Service
    (   void
    );

    /*
     * IMidiMiniportAdLibGold (ISR dispatch from adapter common)
     */
    STDMETHODIMP_(void) ServiceMidiISR
    (   void
    );

    /*
     * IPowerNotify methods
     */
    IMP_IPowerNotify;

    /*
     * Friends
     */
    friend class CMiniportMidiStreamUartAdLibGold;
};


/*****************************************************************************
 * CMiniportMidiStreamUartAdLibGold
 *****************************************************************************
 * MIDI UART miniport stream.  Adapted from DDK CMiniportMidiStreamUart.
 * Handles per-stream state for render (transmit) and capture (receive).
 */
class CMiniportMidiStreamUartAdLibGold
:   public IMiniportMidiStream,
    public CUnknown
{
private:
    CMiniportMidiUartAdLibGold *m_pMiniport; /* Parent miniport               */
    BOOLEAN     m_fCapture;                 /* TRUE for capture stream        */
    long        m_NumFailedTries;           /* Deadman counter for Tx errors  */

public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CMiniportMidiStreamUartAdLibGold);

    ~CMiniportMidiStreamUartAdLibGold();

    NTSTATUS
    Init
    (
        IN      CMiniportMidiUartAdLibGold *    pMiniport,
        IN      BOOLEAN                         fCapture
    );

    /*
     * IMiniportMidiStream methods
     */
    STDMETHODIMP SetFormat
    (
        IN      PKSDATAFORMAT   DataFormat
    );
    STDMETHODIMP SetState
    (
        IN      KSSTATE     State
    );
    STDMETHODIMP Read
    (
        IN      PVOID       BufferAddress,
        IN      ULONG       BufferLength,
        OUT     PULONG      BytesRead
    );
    STDMETHODIMP Write
    (
        IN      PVOID       BufferAddress,
        IN      ULONG       BytesToWrite,
        OUT     PULONG      BytesWritten
    );
};


#endif  /* _ALGMIDI_PRIVATE_H_ */
