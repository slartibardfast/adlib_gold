/*****************************************************************************
 * midi.cpp - Ad Lib Gold MIDI UART miniport implementation
 *****************************************************************************
 *
 * MIDI UART miniport for the YMZ263 (MMA) MIDI subsystem.  Adapted from
 * the Windows 2000 DDK uart/miniport.cpp and uart/mpu.cpp sample drivers.
 *
 * Key adaptations from DDK UART sample:
 *   - All hardware access through IAdapterCommon::ReadMMA/WriteMMA
 *     (no direct WRITE_PORT_UCHAR / READ_PORT_UCHAR)
 *   - ISR callback via IMidiMiniportAdLibGold::ServiceMidiISR()
 *     instead of MPUInterruptServiceRoutine
 *   - MIDI reset via MMA register 0Dh (not MPU-401 command 0xFF/0x3F)
 *   - Uses shared interrupt sync from adapter common
 *
 * Copyright (c) 1997-1999 Microsoft Corporation.  All rights reserved.
 * Adapted for Ad Lib Gold, 2026.
 */

#include "midi.h"

#define STR_MODULENAME "AdLibGoldMIDI: "


/*****************************************************************************
 * Synchronized write context
 *
 * Passed to CallSynchronizedRoutine for transmitting MIDI data at DIRQL.
 */
typedef struct
{
    CMiniportMidiUartAdLibGold  *Miniport;
    PVOID                       BufferAddress;
    ULONG                       Length;
    PULONG                      BytesWritten;
}
SYNCWRITECONTEXT, *PSYNCWRITECONTEXT;


/*****************************************************************************
 * Forward declarations
 */
NTSTATUS
SynchronizedMidiWrite
(
    IN      PINTERRUPTSYNC  InterruptSync,
    IN      PVOID           DynamicContext
);


/*****************************************************************************
 * Filter descriptor tables
 *****************************************************************************
 * Pin data ranges, pin descriptors, connections, and filter descriptor
 * for the MIDI UART miniport.  Follows the DDK UART sample layout:
 *
 *   Pin 0: Render stream  (MIDI data in from application)
 *   Pin 1: Render bridge  (to external MIDI out)
 *   Pin 2: Capture stream (MIDI data out to application)
 *   Pin 3: Capture bridge (from external MIDI in)
 *
 * Connections:
 *   Pin 0 (render stream)  -> Pin 1 (render bridge)
 *   Pin 3 (capture bridge) -> Pin 2 (capture stream)
 */

#pragma code_seg("PAGE")

static
KSDATARANGE_MUSIC PinDataRangesStream[] =
{
    {
        {
            sizeof(KSDATARANGE_MUSIC),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_MUSIC),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_MIDI),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
        },
        STATICGUIDOF(KSMUSIC_TECHNOLOGY_PORT),
        0,
        0,
        0xFFFF
    }
};

static
PKSDATARANGE PinDataRangePointersStream[] =
{
    PKSDATARANGE(&PinDataRangesStream[0])
};

static
KSDATARANGE PinDataRangesBridge[] =
{
    {
        sizeof(KSDATARANGE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_MUSIC),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_MIDI_BUS),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
    }
};

static
PKSDATARANGE PinDataRangePointersBridge[] =
{
    &PinDataRangesBridge[0]
};

static
PCPIN_DESCRIPTOR MiniportPins[] =
{
    /* Pin 0: Render stream (MIDI data in) */
    {
        kMaxMidiRenderStreams, kMaxMidiRenderStreams, 0,
        NULL,
        {
            0, NULL,
            0, NULL,
            SIZEOF_ARRAY(PinDataRangePointersStream),
            PinDataRangePointersStream,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_SINK,
            (GUID *)&KSCATEGORY_AUDIO,
            &KSAUDFNAME_MIDI,
            0
        }
    },
    /* Pin 1: Render bridge (to external MIDI out) */
    {
        0, 0, 0,
        NULL,
        {
            0, NULL,
            0, NULL,
            SIZEOF_ARRAY(PinDataRangePointersBridge),
            PinDataRangePointersBridge,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_NONE,
            (GUID *)&KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
    /* Pin 2: Capture stream (MIDI data out) */
    {
        kMaxMidiCaptureStreams, kMaxMidiCaptureStreams, 0,
        NULL,
        {
            0, NULL,
            0, NULL,
            SIZEOF_ARRAY(PinDataRangePointersStream),
            PinDataRangePointersStream,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_SINK,
            (GUID *)&KSCATEGORY_AUDIO,
            &KSAUDFNAME_MIDI,
            0
        }
    },
    /* Pin 3: Capture bridge (from external MIDI in) */
    {
        0, 0, 0,
        NULL,
        {
            0, NULL,
            0, NULL,
            SIZEOF_ARRAY(PinDataRangePointersBridge),
            PinDataRangePointersBridge,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_NONE,
            (GUID *)&KSCATEGORY_AUDIO,
            NULL,
            0
        }
    }
};

static
PCCONNECTION_DESCRIPTOR MiniportConnections[] =
{
    { PCFILTER_NODE,  0,  PCFILTER_NODE,  1 },   /* Render:  pin 0 -> pin 1 */
    { PCFILTER_NODE,  3,  PCFILTER_NODE,  2 }    /* Capture: pin 3 -> pin 2 */
};

static
PCFILTER_DESCRIPTOR MiniportFilterDescriptor =
{
    0,                                      /* Version                       */
    NULL,                                   /* AutomationTable               */
    sizeof(PCPIN_DESCRIPTOR),               /* PinSize                       */
    SIZEOF_ARRAY(MiniportPins),             /* PinCount                      */
    MiniportPins,                           /* Pins                          */
    sizeof(PCNODE_DESCRIPTOR),              /* NodeSize                      */
    0,                                      /* NodeCount                     */
    NULL,                                   /* Nodes                         */
    SIZEOF_ARRAY(MiniportConnections),      /* ConnectionCount               */
    MiniportConnections,                    /* Connections                   */
    0,                                      /* CategoryCount                 */
    NULL                                    /* Categories                    */
};


/*****************************************************************************
 * CreateMiniportMidiUartAdLibGold()
 *****************************************************************************
 * Factory function for the MIDI UART miniport.
 */
NTSTATUS
CreateMiniportMidiUartAdLibGold
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
)
{
    PAGED_CODE();

    ASSERT(Unknown);

    _DbgPrintF(DEBUGLVL_BLAB, ("CreateMiniportMidiUartAdLibGold"));

    STD_CREATE_BODY(CMiniportMidiUartAdLibGold,
                    Unknown, UnknownOuter, PoolType);
}


/*****************************************************************************
 * CMiniportMidiUartAdLibGold::GetDescription()
 *****************************************************************************
 * Returns the filter descriptor.
 */
STDMETHODIMP
CMiniportMidiUartAdLibGold::
GetDescription
(
    OUT     PPCFILTER_DESCRIPTOR *  OutFilterDescriptor
)
{
    PAGED_CODE();

    ASSERT(OutFilterDescriptor);

    _DbgPrintF(DEBUGLVL_VERBOSE, ("GetDescription"));

    *OutFilterDescriptor = &MiniportFilterDescriptor;

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CMiniportMidiUartAdLibGold::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
STDMETHODIMP
CMiniportMidiUartAdLibGold::
NonDelegatingQueryInterface
(
    REFIID  Interface,
    PVOID * Object
)
{
    PAGED_CODE();

    ASSERT(Object);

    _DbgPrintF(DEBUGLVL_BLAB, ("NonDelegatingQueryInterface"));

    if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PMINIPORTMIDI(this)));
    }
    else
    if (IsEqualGUIDAligned(Interface, IID_IMiniport))
    {
        *Object = PVOID(PMINIPORT(this));
    }
    else
    if (IsEqualGUIDAligned(Interface, IID_IMiniportMidi))
    {
        *Object = PVOID(PMINIPORTMIDI(this));
    }
    else
    if (IsEqualGUIDAligned(Interface, IID_IMidiMiniportAdLibGold))
    {
        *Object = PVOID(PMIDIMINIPORTADLIBGOLD(this));
    }
    else
    if (IsEqualGUIDAligned(Interface, IID_IPowerNotify))
    {
        *Object = PVOID(PPOWERNOTIFY(this));
    }
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}


/*****************************************************************************
 * CMiniportMidiUartAdLibGold::~CMiniportMidiUartAdLibGold()
 *****************************************************************************
 * Destructor.  Resets MIDI hardware and unregisters from adapter common.
 */
CMiniportMidiUartAdLibGold::
~CMiniportMidiUartAdLibGold
(   void
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("~CMiniportMidiUartAdLibGold"));

    ASSERT(0 == m_NumCaptureStreams);
    ASSERT(0 == m_NumRenderStreams);

    /*
     * Reset MIDI circuits and mask all MIDI interrupts.
     */
    if (m_AdapterCommon)
    {
        m_AdapterCommon->WriteMMA(MMA_REG_MIDI_CTRL,
            MMA_MIDI_MSK_POV | MMA_MIDI_MSK_MOV |
            MMA_MIDI_TRS_RST | MMA_MIDI_MSK_TRQ |
            MMA_MIDI_RCV_RST | MMA_MIDI_MSK_RRQ);

        /* Release reset after masking interrupts */
        m_AdapterCommon->WriteMMA(MMA_REG_MIDI_CTRL,
            MMA_MIDI_MSK_POV | MMA_MIDI_MSK_MOV |
            MMA_MIDI_MSK_TRQ | MMA_MIDI_MSK_RRQ);

        /* Unregister from ISR dispatch */
        m_AdapterCommon->SetMidiMiniport(NULL);

        m_AdapterCommon->Release();
        m_AdapterCommon = NULL;
    }

    if (m_ServiceGroup)
    {
        m_ServiceGroup->Release();
        m_ServiceGroup = NULL;
    }

    if (m_Port)
    {
        m_Port->Release();
        m_Port = NULL;
    }
}


/*****************************************************************************
 * CMiniportMidiUartAdLibGold::Init()
 *****************************************************************************
 * Initializes the MIDI UART miniport.
 */
STDMETHODIMP
CMiniportMidiUartAdLibGold::
Init
(
    IN      PUNKNOWN        UnknownAdapter  OPTIONAL,
    IN      PRESOURCELIST   ResourceList,
    IN      PPORTMIDI       Port,
    OUT     PSERVICEGROUP * ServiceGroup
)
{
    PAGED_CODE();

    ASSERT(ResourceList);
    ASSERT(Port);
    ASSERT(ServiceGroup);

    _DbgPrintF(DEBUGLVL_VERBOSE, ("Init"));

    NTSTATUS ntStatus = STATUS_SUCCESS;

    /*
     * Initialize member variables.
     */
    m_AdapterCommon     = NULL;
    m_ServiceGroup      = NULL;
    m_NumCaptureStreams  = 0;
    m_NumRenderStreams   = 0;
    m_KSStateInput      = KSSTATE_STOP;
    m_InputBufferHead   = 0;
    m_InputBufferTail   = 0;
    m_PowerState.DeviceState = PowerDeviceD0;

    RtlZeroMemory(m_InputBuffer, sizeof(m_InputBuffer));

    /*
     * Keep a reference to the port driver.
     */
    m_Port = Port;
    m_Port->AddRef();

    /*
     * Get the adapter common interface from the adapter object.
     */
    if (UnknownAdapter)
    {
        ntStatus = UnknownAdapter->QueryInterface(
            IID_IAdapterCommon,
            (PVOID *)&m_AdapterCommon);
    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    /*
     * Create a service group for capture notifications.
     */
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = PcNewServiceGroup(&m_ServiceGroup, NULL);
        if (NT_SUCCESS(ntStatus) && !m_ServiceGroup)
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (NT_SUCCESS(ntStatus))
    {
        *ServiceGroup = m_ServiceGroup;
        m_ServiceGroup->AddRef();

        /*
         * Register the service group early so the port is ready
         * for interrupts.
         */
        m_Port->RegisterServiceGroup(m_ServiceGroup);
    }

    /*
     * Initialize MIDI hardware on the YMZ263.
     *
     * 1. Reset both transmit and receive circuits
     * 2. Release reset
     * 3. Mask transmit FIFO and overrun IRQs (we poll for Tx)
     * 4. Enable receive FIFO IRQ (MSK_RRQ = 0)
     */
    if (NT_SUCCESS(ntStatus))
    {
        /* Assert reset for both Tx and Rx circuits */
        m_AdapterCommon->WriteMMA(MMA_REG_MIDI_CTRL,
            MMA_MIDI_MSK_POV | MMA_MIDI_MSK_MOV |
            MMA_MIDI_TRS_RST | MMA_MIDI_MSK_TRQ |
            MMA_MIDI_RCV_RST | MMA_MIDI_MSK_RRQ);

        /* Release reset, enable receive interrupt */
        m_AdapterCommon->WriteMMA(MMA_REG_MIDI_CTRL,
            MMA_MIDI_CTRL_DEFAULT);

        /*
         * Drain any stale data from the receive FIFO.
         */
        {
            ULONG drain;
            for (drain = 0; drain < 16; drain++)
            {
                UCHAR mmaStatus = m_AdapterCommon->ReadMMA(MMA_REG_STATUS);
                if (!(mmaStatus & MMA_STATUS_RRQ))
                    break;
                (void) m_AdapterCommon->ReadMMA(MMA_REG_MIDI_DATA);
            }
        }

        /*
         * Register with the adapter common for ISR dispatch.
         */
        m_AdapterCommon->SetMidiMiniport(
            (PMIDIMINIPORTADLIBGOLD)this);
    }

    /*
     * Cleanup on failure.
     */
    if (!NT_SUCCESS(ntStatus))
    {
        if (m_ServiceGroup)
        {
            m_ServiceGroup->Release();
            m_ServiceGroup = NULL;
        }
        *ServiceGroup = NULL;

        if (m_AdapterCommon)
        {
            m_AdapterCommon->Release();
            m_AdapterCommon = NULL;
        }

        m_Port->Release();
        m_Port = NULL;
    }

    return ntStatus;
}


/*****************************************************************************
 * CMiniportMidiUartAdLibGold::NewStream()
 *****************************************************************************
 * Creates a new render or capture stream.
 */
STDMETHODIMP
CMiniportMidiUartAdLibGold::
NewStream
(
    OUT     PMINIPORTMIDISTREAM *   Stream,
    IN      PUNKNOWN                OuterUnknown    OPTIONAL,
    IN      POOL_TYPE               PoolType,
    IN      ULONG                   PinID,
    IN      BOOLEAN                 Capture,
    IN      PKSDATAFORMAT           DataFormat,
    OUT     PSERVICEGROUP *         ServiceGroup
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE, ("NewStream Pin=%d Capture=%d", PinID, Capture));

    NTSTATUS ntStatus = STATUS_SUCCESS;

    /*
     * Validate stream limits.
     */
    if (Capture && (m_NumCaptureStreams >= kMaxMidiCaptureStreams))
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("NewStream: too many capture streams"));
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    if (!Capture && (m_NumRenderStreams >= kMaxMidiRenderStreams))
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("NewStream: too many render streams"));
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    /*
     * Create the stream object.
     */
    CMiniportMidiStreamUartAdLibGold *pStream =
        new(PoolType) CMiniportMidiStreamUartAdLibGold(OuterUnknown);

    if (pStream)
    {
        pStream->AddRef();

        ntStatus = pStream->Init(this, Capture);

        if (NT_SUCCESS(ntStatus))
        {
            *Stream = PMINIPORTMIDISTREAM(pStream);
            (*Stream)->AddRef();

            if (Capture)
            {
                m_NumCaptureStreams++;
                *ServiceGroup = m_ServiceGroup;
                (*ServiceGroup)->AddRef();
            }
            else
            {
                m_NumRenderStreams++;
                *ServiceGroup = NULL;
            }

            _DbgPrintF(DEBUGLVL_VERBOSE,
                ("NewStream: render=%d capture=%d",
                 m_NumRenderStreams, m_NumCaptureStreams));
        }

        pStream->Release();
    }
    else
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
}


/*****************************************************************************
 * Non-pageable code — DPC callbacks, ISR, and synchronized routines
 */
#pragma code_seg()


/*****************************************************************************
 * CMiniportMidiUartAdLibGold::Service()
 *****************************************************************************
 * DPC-mode service call from the port driver.
 * Called when the service group is signaled (after ISR puts data in FIFO).
 * Runs at DISPATCH_LEVEL — must be non-paged.
 */
STDMETHODIMP_(void)
CMiniportMidiUartAdLibGold::
Service
(   void
)
{
    _DbgPrintF(DEBUGLVL_BLAB, ("Service"));

    if (!m_NumCaptureStreams)
    {
        /*
         * No capture streams open.  Discard any buffered data.
         */
        m_InputBufferTail = m_InputBufferHead = 0;
    }
}


/*****************************************************************************
 * CMiniportMidiUartAdLibGold::ServiceMidiISR()
 *****************************************************************************
 * Called from the adapter common ISR when MMA status indicates MIDI
 * receive data is available (RRQ bit set).
 *
 * Drains the YMZ263 MIDI receive FIFO into the software ring buffer.
 * Signals the service group so the port driver's DPC will call Read().
 *
 * Runs at DIRQL — no pageable code, no blocking.
 */
STDMETHODIMP_(void)
CMiniportMidiUartAdLibGold::
ServiceMidiISR
(   void
)
{
    BOOLEAN newBytesAvailable = FALSE;
    ULONG   bytesDrained = 0;

    /*
     * Read bytes from the hardware FIFO until no more data is available
     * or we've drained a reasonable number (16 = MIDI FIFO depth).
     */
    while (bytesDrained < 16)
    {
        UCHAR mmaStatus = m_AdapterCommon->ReadMMA(MMA_REG_STATUS);
        if (!(mmaStatus & MMA_STATUS_RRQ))
        {
            break;      /* No more MIDI data available */
        }

        UCHAR dataByte = m_AdapterCommon->ReadMMA(MMA_REG_MIDI_DATA);
        bytesDrained++;

        if ((m_KSStateInput != KSSTATE_RUN) || (!m_NumCaptureStreams))
        {
            continue;   /* Discard data if not running */
        }

        /*
         * Check for buffer overflow.
         */
        ULONG nextTail = (m_InputBufferTail + 1) % MIDI_INPUT_BUFFER_SIZE;
        if (nextTail == m_InputBufferHead)
        {
            _DbgPrintF(DEBUGLVL_TERSE,
                ("ServiceMidiISR: input buffer overflow"));
            continue;   /* Drop byte on overflow */
        }

        m_InputBuffer[m_InputBufferTail] = dataByte;
        m_InputBufferTail = nextTail;
        newBytesAvailable = TRUE;
    }

    /*
     * Notify the port driver that data is available.
     */
    if (newBytesAvailable && m_Port)
    {
        m_Port->Notify(m_ServiceGroup);
    }
}


/*****************************************************************************
 * CMiniportMidiUartAdLibGold::PowerChangeState()
 *****************************************************************************
 * Handle power state changes.
 */
STDMETHODIMP_(void)
CMiniportMidiUartAdLibGold::
PowerChangeState
(
    IN      POWER_STATE     NewState
)
{
    _DbgPrintF(DEBUGLVL_VERBOSE,
        ("PowerChangeState: D%d -> D%d",
         m_PowerState.DeviceState - PowerDeviceD0,
         NewState.DeviceState - PowerDeviceD0));

    if (NewState.DeviceState == PowerDeviceD0)
    {
        /*
         * Resuming from low-power state.
         * Re-initialize MIDI hardware.
         */
        if (m_PowerState.DeviceState != PowerDeviceD0)
        {
            /* Reset Tx and Rx circuits */
            m_AdapterCommon->WriteMMA(MMA_REG_MIDI_CTRL,
                MMA_MIDI_MSK_POV | MMA_MIDI_MSK_MOV |
                MMA_MIDI_TRS_RST | MMA_MIDI_MSK_TRQ |
                MMA_MIDI_RCV_RST | MMA_MIDI_MSK_RRQ);

            /* Release reset, restore default control */
            m_AdapterCommon->WriteMMA(MMA_REG_MIDI_CTRL,
                MMA_MIDI_CTRL_DEFAULT);
        }
    }
    else
    {
        /*
         * Entering low-power state.
         * Mask all MIDI interrupts.
         */
        if (m_PowerState.DeviceState == PowerDeviceD0)
        {
            m_AdapterCommon->WriteMMA(MMA_REG_MIDI_CTRL,
                MMA_MIDI_MSK_POV | MMA_MIDI_MSK_MOV |
                MMA_MIDI_MSK_TRQ | MMA_MIDI_MSK_RRQ);
        }
    }

    m_PowerState = NewState;
}


/*****************************************************************************
 * SynchronizedMidiWrite()
 *****************************************************************************
 * Synchronized routine to transmit MIDI data.
 * Writes bytes to the YMZ263 MIDI data register (0Eh) via WriteMMA.
 *
 * Called via InterruptSync->CallSynchronizedRoutine() to serialize
 * with the ISR.
 */
NTSTATUS
SynchronizedMidiWrite
(
    IN      PINTERRUPTSYNC  InterruptSync,
    IN      PVOID           DynamicContext
)
{
    PSYNCWRITECONTEXT context = (PSYNCWRITECONTEXT)DynamicContext;

    ASSERT(context->Miniport);
    ASSERT(context->BufferAddress);
    ASSERT(context->BytesWritten);

    PUCHAR  pMidiData = PUCHAR(context->BufferAddress);
    ULONG   count = 0;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    /*
     * Write bytes one at a time to the MIDI transmit FIFO.
     * The YMZ263 has a 16-byte transmit FIFO.  At 31.25 kbaud,
     * each byte takes ~320us to transmit, giving ~5ms of buffer.
     *
     * We write up to 16 bytes per call (FIFO depth).  If the caller
     * has more data (e.g., SysEx), the port driver will retry.
     */
    while (count < context->Length && count < 16)
    {
        context->Miniport->m_AdapterCommon->WriteMMA(
            MMA_REG_MIDI_DATA, pMidiData[count]);
        count++;
    }

    *(context->BytesWritten) = count;

    return ntStatus;
}


/*****************************************************************************
 * Pageable code — stream methods
 */
#pragma code_seg("PAGE")


/*****************************************************************************
 * CMiniportMidiStreamUartAdLibGold::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
STDMETHODIMP
CMiniportMidiStreamUartAdLibGold::
NonDelegatingQueryInterface
(
    REFIID  Interface,
    PVOID * Object
)
{
    PAGED_CODE();

    ASSERT(Object);

    _DbgPrintF(DEBUGLVL_BLAB, ("Stream::NonDelegatingQueryInterface"));

    if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(this));
    }
    else
    if (IsEqualGUIDAligned(Interface, IID_IMiniportMidiStream))
    {
        *Object = PVOID(PMINIPORTMIDISTREAM(this));
    }
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}


/*****************************************************************************
 * CMiniportMidiStreamUartAdLibGold::~CMiniportMidiStreamUartAdLibGold()
 *****************************************************************************
 * Destructor.  Decrements the parent miniport's stream count.
 */
CMiniportMidiStreamUartAdLibGold::
~CMiniportMidiStreamUartAdLibGold
(   void
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("~CMiniportMidiStreamUartAdLibGold"));

    if (m_pMiniport)
    {
        if (m_fCapture)
        {
            m_pMiniport->m_NumCaptureStreams--;
        }
        else
        {
            m_pMiniport->m_NumRenderStreams--;
        }

        _DbgPrintF(DEBUGLVL_VERBOSE,
            ("~Stream: render=%d capture=%d",
             m_pMiniport->m_NumRenderStreams,
             m_pMiniport->m_NumCaptureStreams));

        m_pMiniport->Release();
    }
}


/*****************************************************************************
 * CMiniportMidiStreamUartAdLibGold::Init()
 *****************************************************************************
 * Initializes a stream.
 */
STDMETHODIMP
CMiniportMidiStreamUartAdLibGold::
Init
(
    IN      CMiniportMidiUartAdLibGold *    pMiniport,
    IN      BOOLEAN                         fCapture
)
{
    PAGED_CODE();

    ASSERT(pMiniport);

    _DbgPrintF(DEBUGLVL_VERBOSE,
        ("Stream::Init capture=%d", fCapture));

    m_NumFailedTries = 0;
    m_pMiniport = pMiniport;
    m_pMiniport->AddRef();

    m_fCapture = fCapture;

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CMiniportMidiStreamUartAdLibGold::SetFormat()
 *****************************************************************************
 * Sets the format.  MIDI has only one format, so this is a no-op.
 */
STDMETHODIMP
CMiniportMidiStreamUartAdLibGold::
SetFormat
(
    IN      PKSDATAFORMAT   Format
)
{
    PAGED_CODE();

    ASSERT(Format);

    _DbgPrintF(DEBUGLVL_VERBOSE, ("Stream::SetFormat"));

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * Non-pageable stream methods (called at DISPATCH_LEVEL or higher)
 */
#pragma code_seg()


/*****************************************************************************
 * CMiniportMidiStreamUartAdLibGold::SetState()
 *****************************************************************************
 * Sets the state of the stream.
 */
STDMETHODIMP
CMiniportMidiStreamUartAdLibGold::
SetState
(
    IN      KSSTATE     NewState
)
{
    _DbgPrintF(DEBUGLVL_VERBOSE, ("Stream::SetState %d", NewState));

    if (m_fCapture)
    {
        m_pMiniport->m_KSStateInput = NewState;

        if (NewState == KSSTATE_STOP)
        {
            /*
             * Discard all buffered data on stop.
             */
            m_pMiniport->m_InputBufferHead = 0;
            m_pMiniport->m_InputBufferTail = 0;
        }
    }

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CMiniportMidiStreamUartAdLibGold::Read()
 *****************************************************************************
 * Reads incoming MIDI data from the software ring buffer.
 *
 * The ISR (ServiceMidiISR) has already read the hardware FIFO and placed
 * bytes into the software buffer.  This method drains the software buffer
 * into the caller's buffer.
 */
STDMETHODIMP
CMiniportMidiStreamUartAdLibGold::
Read
(
    IN      PVOID   BufferAddress,
    IN      ULONG   Length,
    OUT     PULONG  BytesRead
)
{
    ASSERT(BufferAddress);
    ASSERT(BytesRead);

    *BytesRead = 0;

    if (!m_fCapture)
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    PUCHAR pDest = PUCHAR(BufferAddress);
    ULONG  count = 0;

    while ((m_pMiniport->m_InputBufferHead != m_pMiniport->m_InputBufferTail)
           && (count < Length))
    {
        *pDest = m_pMiniport->m_InputBuffer[m_pMiniport->m_InputBufferHead];
        pDest++;
        count++;

        m_pMiniport->m_InputBufferHead =
            (m_pMiniport->m_InputBufferHead + 1) % MIDI_INPUT_BUFFER_SIZE;
    }

    *BytesRead = count;

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CMiniportMidiStreamUartAdLibGold::Write()
 *****************************************************************************
 * Writes outgoing MIDI data to the YMZ263 MIDI transmit FIFO.
 *
 * Uses a synchronized routine to serialize with the ISR.
 */
STDMETHODIMP
CMiniportMidiStreamUartAdLibGold::
Write
(
    IN      PVOID       BufferAddress,
    IN      ULONG       Length,
    OUT     PULONG      BytesWritten
)
{
    _DbgPrintF(DEBUGLVL_BLAB, ("Stream::Write len=%d", Length));

    ASSERT(BytesWritten);

    if (!BufferAddress)
    {
        Length = 0;
    }

    if (m_fCapture)
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG count = 0;

    if (Length)
    {
        SYNCWRITECONTEXT context;
        context.Miniport        = m_pMiniport;
        context.BufferAddress   = BufferAddress;
        context.Length           = Length;
        context.BytesWritten    = &count;

        PINTERRUPTSYNC pInterruptSync =
            m_pMiniport->m_AdapterCommon->GetInterruptSync();

        if (pInterruptSync)
        {
            ntStatus = pInterruptSync->CallSynchronizedRoutine(
                SynchronizedMidiWrite, PVOID(&context));
        }
        else
        {
            ntStatus = SynchronizedMidiWrite(NULL, PVOID(&context));
        }

        if (count == 0)
        {
            m_NumFailedTries++;
            if (m_NumFailedTries >= 100)
            {
                ntStatus = STATUS_IO_DEVICE_ERROR;
                m_NumFailedTries = 0;
            }
        }
        else
        {
            m_NumFailedTries = 0;
        }
    }

    *BytesWritten = count;

    return ntStatus;
}


#pragma code_seg()
