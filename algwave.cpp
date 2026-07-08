/*****************************************************************************
 * algwave.cpp - Ad Lib Gold WaveCyclic miniport implementation.
 *****************************************************************************
 *
 * YMZ263 (MMA) digital audio for the Ad Lib Gold sound card.
 *
 * Two transfer modes depending on bit depth:
 *   8-bit PCM  -> ISA DMA to FIFO  (hardware transfer, low CPU)
 *   16-bit PCM -> PIO with TPDF dithering  (software transfer in DPC)
 *
 * Adapted from the Windows 2000 DDK SB16 wave miniport sample.
 */

#define STR_MODULENAME "ALGWave: "

#include "algwave.h"


/*
 * 100-nanosecond units per second, used by NormalizePhysicalPosition.
 */
#define _100NS_UNITS_PER_SECOND     10000000L


/*****************************************************************************
 * Supported sample rates (discrete — MMA hardware constraint)
 */
static const ULONG SupportedSampleRates[] =
{
    44100, 22050, 11025, 7350
};

#define NUM_SUPPORTED_RATES     SIZEOF_ARRAY(SupportedSampleRates)


/*****************************************************************************
 * Data range: PCM audio for streaming pins
 */
static
KSDATARANGE_AUDIO PinDataRangesStream[] =
{
    {
        {
            sizeof(KSDATARANGE_AUDIO),
            0,
            0,
            0,
            STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
            STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
            STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
        },
        2,          /* MaximumChannels (mono, plus stereo over DMA interleave; call/0017) */
        8,          /* MinimumBitsPerSample */
        16,         /* MaximumBitsPerSample */
        7350,       /* MinimumSampleFrequency */
        44100       /* MaximumSampleFrequency */
    }
};

static
PKSDATARANGE PinDataRangePointersStream[] =
{
    PKSDATARANGE(&PinDataRangesStream[0])
};

/*****************************************************************************
 * Data range: analog bridge pins
 */
static
KSDATARANGE PinDataRangesBridge[] =
{
    {
        sizeof(KSDATARANGE),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_ANALOG),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
    }
};

static
PKSDATARANGE PinDataRangePointersBridge[] =
{
    PKSDATARANGE(&PinDataRangesBridge[0])
};


/*****************************************************************************
 * Pin descriptors
 *
 * Pin 0: Capture streaming  (data flows OUT from filter to client)
 * Pin 1: Capture bridge     (data flows IN from topology)
 * Pin 2: Render streaming   (data flows IN from client to filter)
 * Pin 3: Render bridge      (data flows OUT to topology)
 */
static
PCPIN_DESCRIPTOR MiniportPins[] =
{
    /* Pin 0 — Wave capture streaming */
    {
        1,1,0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(PinDataRangePointersStream),
            PinDataRangePointersStream,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_SINK,
            (GUID *) &PINNAME_CAPTURE,
            &KSAUDFNAME_RECORDING_CONTROL,
            0
        }
    },
    /* Pin 1 — Wave capture bridge (from topology) */
    {
        0,0,0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(PinDataRangePointersBridge),
            PinDataRangePointersBridge,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_NONE,
            (GUID *) &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
    /* Pin 2 — Wave render streaming */
    {
        1,1,0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(PinDataRangePointersStream),
            PinDataRangePointersStream,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_SINK,
            (GUID *) &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    },
    /* Pin 3 — Wave render bridge (to topology) */
    {
        0,0,0,
        NULL,
        {
            0,
            NULL,
            0,
            NULL,
            SIZEOF_ARRAY(PinDataRangePointersBridge),
            PinDataRangePointersBridge,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_NONE,
            (GUID *) &KSCATEGORY_AUDIO,
            NULL,
            0
        }
    }
};


/*****************************************************************************
 * Node descriptors: ADC and DAC
 */
static
PCNODE_DESCRIPTOR MiniportNodes[] =
{
    {
        0,                          /* Flags            */
        NULL,                       /* AutomationTable  */
        &KSNODETYPE_ADC,            /* Type             */
        NULL                        /* Name             */
    },
    {
        0,
        NULL,
        &KSNODETYPE_DAC,
        NULL
    }
};


/*****************************************************************************
 * Connection descriptors
 *
 *   Bridge in (pin 1) -> ADC (node 0) -> Capture stream (pin 0)
 *   Render stream (pin 2) -> DAC (node 1) -> Bridge out (pin 3)
 */
static
PCCONNECTION_DESCRIPTOR MiniportConnections[] =
{
    { PCFILTER_NODE,  1,    0,              1 },    /* Bridge in -> ADC      */
    { 0,              0,    PCFILTER_NODE,  0 },    /* ADC -> Capture stream */
    { PCFILTER_NODE,  2,    1,              1 },    /* Render stream -> DAC  */
    { 1,              0,    PCFILTER_NODE,  3 }     /* DAC -> Bridge out     */
};


/*****************************************************************************
 * Filter descriptor
 */
static
PCFILTER_DESCRIPTOR MiniportFilterDescriptor =
{
    0,                                      /* Version          */
    NULL,                                   /* AutomationTable  */
    sizeof(PCPIN_DESCRIPTOR),               /* PinSize          */
    SIZEOF_ARRAY(MiniportPins),             /* PinCount         */
    MiniportPins,                           /* Pins             */
    sizeof(PCNODE_DESCRIPTOR),              /* NodeSize         */
    SIZEOF_ARRAY(MiniportNodes),            /* NodeCount        */
    MiniportNodes,                          /* Nodes            */
    SIZEOF_ARRAY(MiniportConnections),      /* ConnectionCount  */
    MiniportConnections,                    /* Connections      */
    0,                                      /* CategoryCount    */
    NULL                                    /* Categories       */
};


#pragma code_seg("PAGE")

/*****************************************************************************
 * CreateMiniportWaveCyclicAdLibGold()
 *****************************************************************************
 * Factory function for adapter.cpp to instantiate the wave miniport.
 */
NTSTATUS
CreateMiniportWaveCyclicAdLibGold
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
)
{
    PAGED_CODE();

    ASSERT(Unknown);

    STD_CREATE_BODY_(CMiniportWaveCyclicAdLibGold, Unknown, UnknownOuter, PoolType, PMINIPORTWAVECYCLIC);
}


/*****************************************************************************
 * CMiniportWaveCyclicAdLibGold non-delegating IUnknown
 */
STDMETHODIMP
CMiniportWaveCyclicAdLibGold::
NonDelegatingQueryInterface
(
    IN      REFIID  Interface,
    OUT     PVOID * Object
)
{
    PAGED_CODE();

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = PVOID((PUNKNOWN)(IMiniportWaveCyclic *)this);
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniport))
    {
        *Object = PVOID((PMINIPORT)(IMiniportWaveCyclic *)this);
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniportWaveCyclic))
    {
        *Object = PVOID((PMINIPORTWAVECYCLIC)this);
    }
    else if (IsEqualGUIDAligned(Interface, IID_IWaveMiniportAdLibGold))
    {
        *Object = PVOID((PWAVEMINIPORTADLIBGOLD)this);
    }
    else if (IsEqualGUIDAligned(Interface, IID_IPowerNotify))
    {
        *Object = PVOID((PPOWERNOTIFY)this);
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
 * CMiniportWaveCyclicAdLibGold::~CMiniportWaveCyclicAdLibGold()
 */
CMiniportWaveCyclicAdLibGold::
~CMiniportWaveCyclicAdLibGold
(   void
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE, ("[~CMiniportWaveCyclicAdLibGold]"));

    if (m_AdapterCommon)
    {
        m_AdapterCommon->SetWaveMiniport(NULL);
        m_AdapterCommon->Release();
        m_AdapterCommon = NULL;
    }

    if (m_DmaChannel)
    {
        m_DmaChannel->Release();
        m_DmaChannel = NULL;
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
 * CMiniportWaveCyclicAdLibGold::Init()
 *****************************************************************************
 * Initialize the wave miniport.  Called by PortCls after port->Init().
 */
STDMETHODIMP
CMiniportWaveCyclicAdLibGold::
Init
(
    IN      PUNKNOWN        UnknownAdapter,
    IN      PRESOURCELIST   ResourceList,
    IN      PPORTWAVECYCLIC Port_
)
{
    PAGED_CODE();

    ASSERT(UnknownAdapter);
    ASSERT(ResourceList);
    ASSERT(Port_);

    _DbgPrintF(DEBUGLVL_VERBOSE, ("[CMiniportWaveCyclicAdLibGold::Init]"));

    m_Port = Port_;
    m_Port->AddRef();

    m_CaptureAllocated  = FALSE;
    m_RenderAllocated   = FALSE;
    m_SamplingFrequency = 44100;
    m_NotificationInterval = 0;
    m_pActiveStream     = NULL;
    m_PowerState.DeviceState = PowerDeviceD0;
#if (DBG)
    m_IsrCount          = 0;
#endif

    /* Zero the interface pointers before any can fail: the DDK placement new does
     * not zero pool, and the destructor releases each under an `if (ptr)` guard, so
     * an early QueryInterface/PcNewServiceGroup failure must leave them NULL rather
     * than as heap garbage that the guard would take for a live pointer. */
    m_AdapterCommon     = NULL;
    m_ServiceGroup      = NULL;
    m_DmaChannel        = NULL;

    NTSTATUS ntStatus =
        UnknownAdapter->QueryInterface(IID_IAdapterCommon,
                                        (PVOID *)&m_AdapterCommon);

    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = PcNewServiceGroup(&m_ServiceGroup, NULL);
    }

    if (NT_SUCCESS(ntStatus))
    {
        m_AdapterCommon->SetWaveMiniport((PWAVEMINIPORTADLIBGOLD)this);
        ntStatus = ProcessResources(ResourceList);
    }

    if (!NT_SUCCESS(ntStatus))
    {
        /* Clean up on failure */
        if (m_AdapterCommon)
        {
            m_AdapterCommon->SetWaveMiniport(NULL);

            if (m_ServiceGroup)
            {
                m_ServiceGroup->Release();
                m_ServiceGroup = NULL;
            }

            m_AdapterCommon->Release();
            m_AdapterCommon = NULL;
        }

        m_Port->Release();
        m_Port = NULL;
    }

    return ntStatus;
}


/*****************************************************************************
 * CMiniportWaveCyclicAdLibGold::ProcessResources()
 *****************************************************************************
 * Allocate DMA channel and configure Control Chip IRQ/DMA registers.
 */
NTSTATUS
CMiniportWaveCyclicAdLibGold::
ProcessResources
(
    IN      PRESOURCELIST   ResourceList
)
{
    PAGED_CODE();

    ASSERT(ResourceList);

    _DbgPrintF(DEBUGLVL_VERBOSE, ("[CMiniportWaveCyclicAdLibGold::ProcessResources]"));

    ULONG countIO  = ResourceList->NumberOfPorts();
    ULONG countIRQ = ResourceList->NumberOfInterrupts();
    ULONG countDMA = ResourceList->NumberOfDmas();

    if ((countIO < 1) || (countIRQ < 1) || (countDMA < 1))
    {
        _DbgPrintF(DEBUGLVL_TERSE,
            ("ProcessResources: need ports+IRQ+DMA (got %d/%d/%d)",
             countIO, countIRQ, countDMA));
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    /*
     * Create slave DMA channel.  Used directly for 8-bit mode;
     * also provides the cyclic buffer that PortCls fills for 16-bit mode.
     */
    NTSTATUS ntStatus =
        m_Port->NewSlaveDmaChannel(
            &m_DmaChannel,
            NULL,                       /* OuterUnknown      */
            ResourceList,
            0,                          /* DMA resource index */
            MAXLEN_DMA_BUFFER,
            FALSE,                      /* DemandMode        */
            Compatible);                /* DMA speed         */

    if (NT_SUCCESS(ntStatus))
    {
        /*
         * Allocate DMA buffer with fallback to smaller sizes.
         */
        ULONG bufferLength = MAXLEN_DMA_BUFFER;

        do
        {
            ntStatus = m_DmaChannel->AllocateBuffer(bufferLength, NULL);
            bufferLength >>= 1;
        }
        while (!NT_SUCCESS(ntStatus) && (bufferLength >= (PAGE_SIZE / 2)));
    }

    /*
     * Configure Control Chip registers 13h/14h for IRQ and DMA. Mask both MMA
     * channels' FIFO interrupts first: the MMA's power-on mask state is
     * undocumented and the FIFO flags are level-sensitive, so an unmasked idle
     * FIFO could hold the line asserted the moment reg 13h sets AEN (call/0029).
     * Stream start programs the real format values; stop re-masks.
     */
    if (NT_SUCCESS(ntStatus))
    {
        m_AdapterCommon->WriteMMA (MMA_REG_FORMAT, MMA_FMT_MSK);
        m_AdapterCommon->WriteMMA1(MMA_REG_FORMAT, MMA_FMT_MSK);
        /* The timer masks (reg 08h) are the remaining undocumented-at-power-on
         * interrupt source on the same line; mask all three and stop the
         * timers and base counter before AEN goes live (call/0032). */
        m_AdapterCommon->WriteMMA (MMA_REG_TIMER_CTRL, MMA_TIMER_MASK_ALL);
        ConfigureDmaAndIrq(ResourceList);
    }

    if (!NT_SUCCESS(ntStatus))
    {
        if (m_DmaChannel)
        {
            m_DmaChannel->Release();
            m_DmaChannel = NULL;
        }
    }

    return ntStatus;
}


/*****************************************************************************
 * CMiniportWaveCyclicAdLibGold::ConfigureDmaAndIrq()
 *****************************************************************************
 * Program Control Chip registers 13h (IRQ + DMA ch0) and 14h (DMA ch1).
 * Maps PnP-assigned IRQ and DMA numbers to the hardware's select fields.
 */
void
CMiniportWaveCyclicAdLibGold::
ConfigureDmaAndIrq
(
    IN      PRESOURCELIST   ResourceList
)
{
    PAGED_CODE();

    /*
     * Get the PnP-assigned IRQ and DMA channel numbers.
     */
    PCM_PARTIAL_RESOURCE_DESCRIPTOR irqDesc =
        ResourceList->FindUntranslatedInterrupt(0);
    PCM_PARTIAL_RESOURCE_DESCRIPTOR dmaDesc =
        ResourceList->FindUntranslatedDma(0);

    ULONG irqLine  = irqDesc->u.Interrupt.Level;
    ULONG dmaChan  = dmaDesc->u.Dma.Channel;

    /*
     * Map IRQ number to Control Chip register 13h IRQ select field.
     */
    BYTE irqSel;
    switch (irqLine)
    {
    case 3:  irqSel = CTRL_IRQ_SEL_3;  break;
    case 4:  irqSel = CTRL_IRQ_SEL_4;  break;
    case 5:  irqSel = CTRL_IRQ_SEL_5;  break;
    case 7:  irqSel = CTRL_IRQ_SEL_7;  break;
    case 10: irqSel = CTRL_IRQ_SEL_10; break;
    case 11: irqSel = CTRL_IRQ_SEL_11; break;
    case 12: irqSel = CTRL_IRQ_SEL_12; break;
    case 15: irqSel = CTRL_IRQ_SEL_15; break;
    default:
        _DbgPrintF(DEBUGLVL_TERSE, ("ConfigureDmaAndIrq: unexpected IRQ %d", irqLine));
        irqSel = CTRL_IRQ_SEL_7;       /* Safe fallback */
        break;
    }

    /*
     * Map DMA channel number to DMA select bits (D6-D5 of reg 13h), validated like the
     * IRQ: an out-of-range channel is reported and falls back to DMA 1 rather than
     * silently masking to a wrong channel.
     */
    if (dmaChan > 3)
    {
        _DbgPrintF(DEBUGLVL_TERSE,
            ("ConfigureDmaAndIrq: unexpected DMA %d, using 1", dmaChan));
    }
    BYTE dmaSel = WaveDmaSelectBits((int)dmaChan);

    /*
     * Register 13h: DMA ch0 enable + DMA select + IRQ enable + IRQ select
     */
    BYTE reg13 = CTRL_DMA0_ENABLE | dmaSel | CTRL_IRQ_ENABLE | irqSel;
    m_AdapterCommon->ControlRegWrite(CTRL_REG_IRQ_DMA0, reg13);

    _DbgPrintF(DEBUGLVL_VERBOSE,
        ("ConfigureDmaAndIrq: IRQ=%d DMA=%d reg13=0x%02X",
         irqLine, dmaChan, (ULONG)reg13));
}


/*****************************************************************************
 * CMiniportWaveCyclicAdLibGold::GetDescription()
 */
STDMETHODIMP
CMiniportWaveCyclicAdLibGold::
GetDescription
(
    OUT     PPCFILTER_DESCRIPTOR *  OutFilterDescriptor
)
{
    PAGED_CODE();

    ASSERT(OutFilterDescriptor);

    *OutFilterDescriptor = &MiniportFilterDescriptor;

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CMiniportWaveCyclicAdLibGold::ValidateFormat()
 *****************************************************************************
 * Check that a KSDATAFORMAT represents a PCM format we can handle.
 */
NTSTATUS
CMiniportWaveCyclicAdLibGold::
ValidateFormat
(
    IN      PKSDATAFORMAT   Format
)
{
    PAGED_CODE();

    ASSERT(Format);

    _DbgPrintF(DEBUGLVL_VERBOSE, ("[CMiniportWaveCyclicAdLibGold::ValidateFormat]"));

    /*
     * Must be audio/PCM with WAVEFORMATEX specifier.
     */
    if ((Format->FormatSize < sizeof(KSDATAFORMAT_WAVEFORMATEX)) ||
        !IsEqualGUIDAligned(Format->MajorFormat, KSDATAFORMAT_TYPE_AUDIO) ||
        !IsEqualGUIDAligned(Format->SubFormat,   KSDATAFORMAT_SUBTYPE_PCM) ||
        !IsEqualGUIDAligned(Format->Specifier,   KSDATAFORMAT_SPECIFIER_WAVEFORMATEX))
    {
        return STATUS_INVALID_PARAMETER;
    }

    PWAVEFORMATEX wfx = PWAVEFORMATEX(Format + 1);

    if (wfx->wFormatTag != WAVE_FORMAT_PCM)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Channels: mono or stereo. Stereo runs the dual-channel interleaved DMA path
     * (call/0017); the pin advertises MaximumChannels = 2. */
    if (wfx->nChannels != 1 && wfx->nChannels != 2)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Bit depth: 8-bit or 16-bit (16 maps to 12-bit hardware via dithering) */
    if ((wfx->wBitsPerSample != 8) && (wfx->wBitsPerSample != 16))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /*
     * Sample rate. The 16-bit path plays through the software PIO fill, which can
     * resample an off-rate source to the nearest hardware rate (call/0011), so it
     * accepts any rate in the audio range. The 8-bit ISA DMA path clocks the hardware
     * directly, so it must be one of the four discrete hardware rates.
     */
    if (wfx->wBitsPerSample == 16 && wfx->nChannels == 1)
    {
        if (wfx->nSamplesPerSec < 4000 || wfx->nSamplesPerSec > 48000)
            return STATUS_INVALID_PARAMETER;
        return STATUS_SUCCESS;
    }

    BOOLEAN rateValid = FALSE;
    ULONG i;
    for (i = 0; i < NUM_SUPPORTED_RATES; i++)
    {
        if (wfx->nSamplesPerSec == SupportedSampleRates[i])
        {
            rateValid = TRUE;
            break;
        }
    }

    if (!rateValid)
    {
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CMiniportWaveCyclicAdLibGold::SampleRateToFreqBits()
 *****************************************************************************
 * Map a sample rate to the MMA register 09h FREQ field value.
 */
BYTE
CMiniportWaveCyclicAdLibGold::
SampleRateToFreqBits
(
    IN      ULONG   SampleRate
)
{
    switch (SampleRate)
    {
    case 44100: return MMA_FREQ_44100;
    case 22050: return MMA_FREQ_22050;
    case 11025: return MMA_FREQ_11025;
    case 7350:  return MMA_FREQ_7350;
    default:    return MMA_FREQ_22050;  /* Safe fallback */
    }
}


/*****************************************************************************
 * CMiniportWaveCyclicAdLibGold::DataRangeIntersection()
 *****************************************************************************
 * Negotiate format between client data range and our data range.
 * Adapted from SB16 DDK sample with discrete sample rate handling.
 */
STDMETHODIMP
CMiniportWaveCyclicAdLibGold::
DataRangeIntersection
(
    IN      ULONG           PinId,
    IN      PKSDATARANGE    ClientDataRange,
    IN      PKSDATARANGE    MyDataRange,
    IN      ULONG           OutputBufferLength,
    OUT     PVOID           ResultantFormat     OPTIONAL,
    OUT     PULONG          ResultantFormatLength
)
{
    PAGED_CODE();

    BOOLEAN     DigitalAudio;
    ULONG       RequiredSize;

    /*
     * Determine format type and required buffer size.
     */
    if (!IsEqualGUIDAligned(ClientDataRange->Specifier,
                            KSDATAFORMAT_SPECIFIER_NONE))
    {
        if (!IsEqualGUIDAligned(ClientDataRange->MajorFormat,
                                KSDATAFORMAT_TYPE_AUDIO) ||
            !IsEqualGUIDAligned(ClientDataRange->SubFormat,
                                KSDATAFORMAT_SUBTYPE_PCM))
        {
            return STATUS_INVALID_PARAMETER;
        }

        DigitalAudio = TRUE;

        if (IsEqualGUIDAligned(ClientDataRange->Specifier,
                               KSDATAFORMAT_SPECIFIER_DSOUND))
        {
            RequiredSize = sizeof(KSDATAFORMAT_DSOUND);
        }
        else
        {
            RequiredSize = sizeof(KSDATAFORMAT_WAVEFORMATEX);
        }
    }
    else
    {
        DigitalAudio = FALSE;
        RequiredSize = sizeof(KSDATAFORMAT);
    }

    /*
     * Handle size query.
     */
    if (!OutputBufferLength)
    {
        *ResultantFormatLength = RequiredSize;
        return STATUS_BUFFER_OVERFLOW;
    }
    else if (OutputBufferLength < RequiredSize)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (DigitalAudio)
    {
        PKSDATARANGE_AUDIO  AudioRange = (PKSDATARANGE_AUDIO)MyDataRange;
        PWAVEFORMATEX       WaveFormatEx;

        /*
         * Fill the appropriate output structure.
         */
        if (IsEqualGUIDAligned(ClientDataRange->Specifier,
                               KSDATAFORMAT_SPECIFIER_DSOUND))
        {
            PKSDATAFORMAT_DSOUND DSoundFormat =
                (PKSDATAFORMAT_DSOUND)ResultantFormat;

            DSoundFormat->BufferDesc.Flags   = 0;
            DSoundFormat->BufferDesc.Control = 0;
            DSoundFormat->DataFormat = *ClientDataRange;
            DSoundFormat->DataFormat.Specifier  = KSDATAFORMAT_SPECIFIER_DSOUND;
            DSoundFormat->DataFormat.FormatSize = RequiredSize;

            WaveFormatEx = &DSoundFormat->BufferDesc.WaveFormatEx;
        }
        else
        {
            PKSDATAFORMAT_WAVEFORMATEX WaveFormat =
                (PKSDATAFORMAT_WAVEFORMATEX)ResultantFormat;

            WaveFormat->DataFormat = *ClientDataRange;
            WaveFormat->DataFormat.Specifier  = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
            WaveFormat->DataFormat.FormatSize = RequiredSize;

            WaveFormatEx = &WaveFormat->WaveFormatEx;
        }

        *ResultantFormatLength = RequiredSize;

        WaveFormatEx->wFormatTag = WAVE_FORMAT_PCM;
        WaveFormatEx->nChannels =
            (USHORT)min(AudioRange->MaximumChannels,
                        ((PKSDATARANGE_AUDIO)ClientDataRange)->MaximumChannels);

        /*
         * Sample rate: if a stream is already active, force the same rate
         * (full-duplex constraint).  Otherwise pick the highest supported
         * rate within the client's range.
         */
        if (m_CaptureAllocated || m_RenderAllocated)
        {
            if ((m_SamplingFrequency >
                    ((PKSDATARANGE_AUDIO)ClientDataRange)->MaximumSampleFrequency) ||
                (m_SamplingFrequency <
                    ((PKSDATARANGE_AUDIO)ClientDataRange)->MinimumSampleFrequency))
            {
                return STATUS_NO_MATCH;
            }
            WaveFormatEx->nSamplesPerSec = m_SamplingFrequency;
        }
        else
        {
            /*
             * Find the highest discrete hardware rate within the
             * intersection of client and device ranges.
             */
            ULONG clientMax =
                ((PKSDATARANGE_AUDIO)ClientDataRange)->MaximumSampleFrequency;
            ULONG clientMin =
                ((PKSDATARANGE_AUDIO)ClientDataRange)->MinimumSampleFrequency;

            ULONG bestRate = 0;
            ULONG i;
            for (i = 0; i < NUM_SUPPORTED_RATES; i++)
            {
                if ((SupportedSampleRates[i] <= clientMax) &&
                    (SupportedSampleRates[i] >= clientMin))
                {
                    bestRate = SupportedSampleRates[i];
                    break;  /* Array is sorted descending, first hit = highest */
                }
            }

            if (bestRate == 0)
            {
                return STATUS_NO_MATCH;
            }

            WaveFormatEx->nSamplesPerSec = bestRate;
        }

        /*
         * Bit depth: prefer 16-bit within range.
         */
        USHORT BitsPerSample =
            (USHORT)min(AudioRange->MaximumBitsPerSample,
                        ((PKSDATARANGE_AUDIO)ClientDataRange)->MaximumBitsPerSample);

        if (BitsPerSample >= 16)
        {
            BitsPerSample = 16;
        }
        else if (BitsPerSample >= 8)
        {
            BitsPerSample = 8;
        }
        else
        {
            return STATUS_NO_MATCH;
        }

        WaveFormatEx->wBitsPerSample  = BitsPerSample;
        WaveFormatEx->nBlockAlign     =
            (WaveFormatEx->wBitsPerSample * WaveFormatEx->nChannels) / 8;
        WaveFormatEx->nAvgBytesPerSec =
            WaveFormatEx->nSamplesPerSec * WaveFormatEx->nBlockAlign;
        WaveFormatEx->cbSize = 0;

        ((PKSDATAFORMAT)ResultantFormat)->SampleSize = WaveFormatEx->nBlockAlign;
    }
    else
    {
        RtlCopyMemory(ResultantFormat, ClientDataRange, sizeof(KSDATAFORMAT));
        *ResultantFormatLength = sizeof(KSDATAFORMAT);
    }

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CMiniportWaveCyclicAdLibGold::NewStream()
 *****************************************************************************
 * Create a new playback or capture stream.
 */
STDMETHODIMP
CMiniportWaveCyclicAdLibGold::
NewStream
(
    OUT     PMINIPORTWAVECYCLICSTREAM * OutStream,
    IN      PUNKNOWN                    OuterUnknown    OPTIONAL,
    IN      POOL_TYPE                   PoolType,
    IN      ULONG                       Pin,
    IN      BOOLEAN                     Capture,
    IN      PKSDATAFORMAT               DataFormat,
    OUT     PDMACHANNEL *               OutDmaChannel,
    OUT     PSERVICEGROUP *             OutServiceGroup
)
{
    PAGED_CODE();

    ASSERT(OutStream);
    ASSERT(DataFormat);
    ASSERT(OutDmaChannel);
    ASSERT(OutServiceGroup);

    _DbgPrintF(DEBUGLVL_VERBOSE, ("[CMiniportWaveCyclicAdLibGold::NewStream]"));

    NTSTATUS ntStatus = STATUS_SUCCESS;

    /*
     * Only one capture and one render stream at a time.
     */
    if (Capture)
    {
        if (m_CaptureAllocated)
        {
            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        }
    }
    else
    {
        if (m_RenderAllocated)
        {
            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    /*
     * Validate format.
     */
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = ValidateFormat(DataFormat);
    }

    /*
     * Reject full duplex until independent per-direction channels exist (plan/0008).
     * The driver has one DMA channel and one buffer, and a stereo stream already uses both
     * MMA channels for its two audio channels (call/0017), not for two directions, so a
     * render and a capture stream would silently share the transport. Allow one direction
     * at a time.
     */
    if (NT_SUCCESS(ntStatus))
    {
        BOOLEAN otherAllocated = Capture ? m_RenderAllocated : m_CaptureAllocated;
        if (otherAllocated)
        {
            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    if (NT_SUCCESS(ntStatus))
    {
        CMiniportWaveCyclicStreamAdLibGold *stream =
            new(PoolType) CMiniportWaveCyclicStreamAdLibGold(OuterUnknown);

        if (stream)
        {
            stream->AddRef();

            ntStatus = stream->Init(this, Capture, DataFormat);

            if (NT_SUCCESS(ntStatus))
            {
                if (Capture)
                {
                    m_CaptureAllocated = TRUE;
                }
                else
                {
                    m_RenderAllocated = TRUE;
                }

                /*
                 * Register the stream the ISR services. Full duplex is rejected above,
                 * so exactly one stream is active; the destructor clears this under the
                 * interrupt lock. The stream is not RUN yet, and the ISR gates on RUN.
                 */
                m_pActiveStream = stream;

                /* Resolve the hardware rate the same way SetFormat does, so
                 * m_SamplingFrequency always means the rate the hardware clocks -- never
                 * the raw source rate on one path and the resolved rate on another.
                 * Only mono 16-bit resamples an off-rate source; every other path clocks
                 * the source rate directly. */
                PWAVEFORMATEX wfx = PWAVEFORMATEX(DataFormat + 1);
                BOOLEAN mono16 =
                    (wfx->wBitsPerSample == 16 && wfx->nChannels == 1);
                m_SamplingFrequency = mono16
                    ? NearestSupportedRate(wfx->nSamplesPerSec)
                    : wfx->nSamplesPerSec;

                *OutStream = PMINIPORTWAVECYCLICSTREAM(stream);
                stream->AddRef();

                *OutDmaChannel = PDMACHANNEL(m_DmaChannel);
                m_DmaChannel->AddRef();

                *OutServiceGroup = m_ServiceGroup;
                m_ServiceGroup->AddRef();
            }

            stream->Release();
        }
        else
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return ntStatus;
}


/*****************************************************************************
 * SyncClearActiveStream()
 *****************************************************************************
 * Clear the miniport's active-stream pointer under the interrupt lock, so the ISR
 * can never dereference a stream that is being torn down. Runs at DIRQL via
 * CallSynchronizedRoutine; clears the pointer only if it still names this stream.
 */
#pragma code_seg()

typedef struct _WAVE_SYNC_CONTEXT
{
    CMiniportWaveCyclicAdLibGold *       Miniport;
    CMiniportWaveCyclicStreamAdLibGold * Stream;
} WAVE_SYNC_CONTEXT, *PWAVE_SYNC_CONTEXT;

NTSTATUS
SyncClearActiveStream
(
    IN      PINTERRUPTSYNC  InterruptSync,
    IN      PVOID           Context
)
{
    PWAVE_SYNC_CONTEXT ctx = (PWAVE_SYNC_CONTEXT)Context;

    if (ctx->Miniport->m_pActiveStream == ctx->Stream)
    {
        ctx->Miniport->m_pActiveStream = NULL;
    }

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CMiniportWaveCyclicAdLibGold::ServiceWaveISR()
 *****************************************************************************
 * Called at DIRQL from the adapter ISR when a sampling interrupt occurs, with the
 * MMA status byte. When the channel-0 PCM FIFO signals (FIF0) and the active 16-bit
 * PIO stream is running, service its FIFO — refill for render, drain for capture —
 * so a 16-bit stream keeps playing/recording instead of underrunning after the
 * pre-fill (plan/0008). The 8-bit DMA path has no active stream registered, so it
 * only notifies. Notify advances the port's position query either way.
 */
STDMETHODIMP_(void)
CMiniportWaveCyclicAdLibGold::
ServiceWaveISR
(
    IN      BYTE    MmaStatus
)
{
#if (DBG)
    m_IsrCount++;
#endif

    CMiniportWaveCyclicStreamAdLibGold *stream = m_pActiveStream;

    if (stream &&
        stream->m_State == KSSTATE_RUN &&
        stream->m_16Bit && !stream->m_Stereo &&
        MmaStatusChannel0Ready(MmaStatus))
    {
        /* Only 16-bit mono runs the software PIO FIFO fill; 8-bit mono and stereo both
         * use DMA, where the ISR just advances the buffer position via Notify (call/0017). */
        if (stream->m_Capture)
        {
            stream->DrainFifo();
        }
        else
        {
            stream->FillFifo(TRUE);     /* ISR context: raw MMA access */
        }
    }

    if (m_Port && m_ServiceGroup)
    {
        m_Port->Notify(m_ServiceGroup);
    }
}

#pragma code_seg("PAGE")


/*****************************************************************************
 * CMiniportWaveCyclicAdLibGold::PowerChangeNotify()
 *****************************************************************************
 * Handle power state transitions.
 */
STDMETHODIMP_(void)
CMiniportWaveCyclicAdLibGold::
PowerChangeNotify
(
    IN      POWER_STATE     NewState
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE,
        ("[CMiniportWaveCyclicAdLibGold::PowerChangeNotify D%d]",
         NewState.DeviceState - PowerDeviceD0));

    m_PowerState = NewState;
}


/*****************************************************************************
 *
 *  Stream implementation
 *
 *****************************************************************************/


/*****************************************************************************
 * CMiniportWaveCyclicStreamAdLibGold non-delegating IUnknown
 */
STDMETHODIMP
CMiniportWaveCyclicStreamAdLibGold::
NonDelegatingQueryInterface
(
    IN      REFIID  Interface,
    OUT     PVOID * Object
)
{
    PAGED_CODE();

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = PVOID((PUNKNOWN)(IMiniportWaveCyclicStream *)this);
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniportWaveCyclicStream))
    {
        *Object = PVOID((PMINIPORTWAVECYCLICSTREAM)this);
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
 * CMiniportWaveCyclicStreamAdLibGold::~CMiniportWaveCyclicStreamAdLibGold()
 */
CMiniportWaveCyclicStreamAdLibGold::
~CMiniportWaveCyclicStreamAdLibGold
(   void
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE, ("[~CMiniportWaveCyclicStreamAdLibGold]"));

    if (m_State != KSSTATE_STOP)
    {
        ProgramMmaStop();
    }

    if (m_Miniport)
    {
        /*
         * Clear the ISR's active-stream pointer under the interrupt lock, so the ISR
         * can never dereference this stream after it is freed (plan/0008).
         * GetInterruptSync returns a borrowed pointer (no AddRef). Before the interrupt
         * is connected there is no armed ISR, so clear it directly.
         */
        PINTERRUPTSYNC intSync = m_Miniport->m_AdapterCommon
            ? m_Miniport->m_AdapterCommon->GetInterruptSync()
            : NULL;
        WAVE_SYNC_CONTEXT ctx;
        ctx.Miniport = m_Miniport;
        ctx.Stream   = this;

        if (intSync)
        {
            intSync->CallSynchronizedRoutine(SyncClearActiveStream, PVOID(&ctx));
        }
        else if (m_Miniport->m_pActiveStream == this)
        {
            m_Miniport->m_pActiveStream = NULL;
        }

        if (m_Capture)
        {
            m_Miniport->m_CaptureAllocated = FALSE;
        }
        else
        {
            m_Miniport->m_RenderAllocated = FALSE;
        }

        m_Miniport->Release();
        m_Miniport = NULL;
    }
}


/*****************************************************************************
 * CMiniportWaveCyclicStreamAdLibGold::Init()
 *****************************************************************************
 * Initialize a stream instance.
 */
NTSTATUS
CMiniportWaveCyclicStreamAdLibGold::
Init
(
    IN      CMiniportWaveCyclicAdLibGold *  Miniport,
    IN      BOOLEAN                         Capture,
    IN      PKSDATAFORMAT                   DataFormat
)
{
    PAGED_CODE();

    ASSERT(Miniport);
    ASSERT(DataFormat);

    _DbgPrintF(DEBUGLVL_VERBOSE, ("[CMiniportWaveCyclicStreamAdLibGold::Init]"));

    m_Miniport = Miniport;
    m_Miniport->AddRef();

    PWAVEFORMATEX wfx = PWAVEFORMATEX(DataFormat + 1);

    m_Capture   = Capture;
    m_16Bit     = (wfx->wBitsPerSample == 16);
    m_Stereo    = (wfx->nChannels == 2);
    m_State     = KSSTATE_STOP;

    /* PIO state */
    m_SoftwarePosition = 0;
    m_DmaBufferSize    = m_Miniport->m_DmaChannel->BufferSize();
    m_LfsrState        = 0xACE1;    /* Non-zero seed */

    /* Resampler: identity until SetFormat sizes it to the source rate. */
    m_SourceRate       = wfx->nSamplesPerSec;
    m_ResampleStep     = 0x10000;
    m_ResamplePhase    = 0;

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CMiniportWaveCyclicStreamAdLibGold::SetFormat()
 *****************************************************************************
 * Set the data format for this stream.  Only allowed when not running.
 */
STDMETHODIMP
CMiniportWaveCyclicStreamAdLibGold::
SetFormat
(
    IN      PKSDATAFORMAT   Format
)
{
    PAGED_CODE();

    ASSERT(Format);

    _DbgPrintF(DEBUGLVL_VERBOSE, ("[CMiniportWaveCyclicStreamAdLibGold::SetFormat]"));

    NTSTATUS ntStatus = m_Miniport->ValidateFormat(Format);

    if (NT_SUCCESS(ntStatus))
    {
        PWAVEFORMATEX wfx = PWAVEFORMATEX(Format + 1);

        /* Mono 16-bit resamples an off-rate source to the nearest hardware rate;
         * every other path runs the hardware at the source rate directly. */
        ULONG   source = wfx->nSamplesPerSec;
        BOOLEAN mono16 = (wfx->wBitsPerSample == 16 && wfx->nChannels == 1);
        ULONG   hwRate = mono16 ? NearestSupportedRate(source) : source;

        /*
         * Capture does not resample: DrainFifo stores samples at the hardware rate, so
         * an off-rate 16-bit capture would record at the wrong speed. Require a discrete
         * hardware rate for a capture stream; only render resamples an off-rate source.
         */
        if (m_Capture)
        {
            BOOLEAN discreteRate = FALSE;
            ULONG   r;
            for (r = 0; r < NUM_SUPPORTED_RATES; r++)
            {
                if (source == SupportedSampleRates[r])
                {
                    discreteRate = TRUE;
                    break;
                }
            }
            if (!discreteRate)
                return STATUS_INVALID_PARAMETER;
        }

        /*
         * Full-duplex constraint: the two directions share the hardware clock,
         * so their hardware rates must match.
         */
        if ((m_Miniport->m_CaptureAllocated && m_Miniport->m_RenderAllocated) &&
            (m_Miniport->m_SamplingFrequency != hwRate))
        {
            return STATUS_INVALID_PARAMETER;
        }

        m_16Bit  = (wfx->wBitsPerSample == 16);
        m_Stereo = (wfx->nChannels == 2);
        m_SourceRate = source;
        m_Miniport->m_SamplingFrequency = hwRate;

        /* Q16 input-samples-per-output; 0x10000 (identity) unless we resample. */
        m_ResampleStep  = (source << 16) / hwRate;
        m_ResamplePhase = 0;
    }

    return ntStatus;
}


/*****************************************************************************
 * CMiniportWaveCyclicStreamAdLibGold::SetState()
 *****************************************************************************
 * Handle stream state transitions.
 *
 *   STOP -> ACQUIRE -> PAUSE -> RUN -> PAUSE -> STOP
 */
STDMETHODIMP
CMiniportWaveCyclicStreamAdLibGold::
SetState
(
    IN      KSSTATE     NewState
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE,
        ("[CMiniportWaveCyclicStreamAdLibGold::SetState %d -> %d]",
         m_State, NewState));

    NTSTATUS ntStatus = STATUS_SUCCESS;

    /*
     * ACQUIRE is treated as PAUSE for our hardware.
     */
    if (NewState == KSSTATE_ACQUIRE)
    {
        NewState = KSSTATE_PAUSE;
    }

    if (m_State != NewState)
    {
        switch (NewState)
        {
        case KSSTATE_PAUSE:
            if (m_State == KSSTATE_RUN)
            {
#if (DBG)
                /* Retest stop line (call/0029), read before the hardware stops:
                 * isr=0 with dmapos=0 means no DRQ; dmapos moving with isr=0
                 * means IRQ routing is dead; both moving localizes a silent
                 * stream to the analog path. */
                {
                    ULONG dbgPos = 0;
                    if ((!m_16Bit || m_Stereo) && m_Miniport->m_DmaChannel)
                    {
                        ULONG dbgTc = m_Miniport->m_DmaChannel->TransferCount();
                        if (dbgTc)
                        {
                            ULONG dbgCounter =
                                m_Miniport->m_DmaChannel->ReadCounter();
                            dbgPos = (dbgCounter != 0) ? (dbgTc - dbgCounter) : 0;
                        }
                    }
                    _DbgPrintF(DEBUGLVL_TERSE,
                        ("SetState RUN->PAUSE: isr=%d dmapos=%d mma=0x%02X",
                         m_Miniport->m_IsrCount, dbgPos,
                         (ULONG)m_Miniport->m_AdapterCommon->ReadMMAStatus()));
                }
#endif

                /*
                 * Stop playback/recording.  Clear GO bit in reg 09h.
                 */
                m_Miniport->m_AdapterCommon->WriteMMA(MMA_REG_PLAYBACK, 0x00);
                if (m_Stereo)
                {
                    /* Stereo drives channel 1 too (call/0017): clear its GO as well. */
                    m_Miniport->m_AdapterCommon->WriteMMA1(MMA_REG_PLAYBACK, 0x00);
                }

                if (!m_16Bit || m_Stereo)
                {
                    /* DMA mode (8-bit mono or any stereo): stop the DMA channel */
                    m_Miniport->m_DmaChannel->Stop();
                }

                /*
                 * Mask FIFO interrupt to avoid spurious interrupts while paused.
                 */
                BYTE fmtReg = (BYTE)(
                    ((m_16Bit ? MMA_DATA_FMT_12B_2 : MMA_DATA_FMT_8BIT)
                        << MMA_FMT_DATA_SHIFT) |
                    (MMA_FIFO_THR_DEFAULT << MMA_FMT_FIFO_SHIFT) |
                    MMA_FMT_MSK);   /* Mask FIFO IRQ */

                if (!m_16Bit)
                {
                    fmtReg |= MMA_FMT_ENB;     /* Keep DMA mode flag */
                }

                m_Miniport->m_AdapterCommon->WriteMMA(MMA_REG_FORMAT, fmtReg);
            }
            break;

        case KSSTATE_RUN:
#if (DBG)
            m_Miniport->m_IsrCount = 0;     /* per-run count for the stop line */
#endif
            ProgramMmaStart();
            break;

        case KSSTATE_STOP:
            ProgramMmaStop();
            break;
        }

        m_State = NewState;
    }

    return ntStatus;
}


/*****************************************************************************
 * CMiniportWaveCyclicStreamAdLibGold::ProgramMmaStart()
 *****************************************************************************
 * Program MMA registers and start playback or recording.
 */
void
CMiniportWaveCyclicStreamAdLibGold::
ProgramMmaStart
(   void
)
{
    PADAPTERCOMMON ac = m_Miniport->m_AdapterCommon;

    /*
     * Stereo runs a separate dual-channel interleaved DMA path (call/0017); everything
     * below this point is the mono path (m_Stereo is FALSE from here on).
     */
    if (m_Stereo)
    {
        ProgramMmaStartStereo();
        return;
    }

    /*
     * Reset the MMA playback/record engine.
     */
    ac->WriteMMA(MMA_REG_PLAYBACK, MMA_PB_RST);
    KeStallExecutionProcessor(1);
    ac->WriteMMA(MMA_REG_PLAYBACK, 0x00);

    /*
     * Prime the FIFO with four zero bytes on the 8-bit DMA render path, directly
     * after the reset, as the validated AIL sequence does before GO
     * (plan/0008/stereo-mma-reference.md :961-964, call/0029). The 16-bit PIO
     * path pre-fills through FillFifo below instead.
     */
    if (!m_16Bit && !m_Capture)
    {
        ac->WriteMMA(MMA_REG_PCM_DATA, 0x00);
        ac->WriteMMA(MMA_REG_PCM_DATA, 0x00);
        ac->WriteMMA(MMA_REG_PCM_DATA, 0x00);
        ac->WriteMMA(MMA_REG_PCM_DATA, 0x00);
    }

    /*
     * Set the MMA digital-audio output volume to full (reg 0Ah). The power-on default
     * is undocumented and left the sampling path silent (call/0025).
     */
    ac->WriteMMA(MMA_REG_VOLUME, 0xFF);

    /*
     * Program register 0Ch (format, FIFO threshold, DMA/PIO mode).
     */
    BYTE fmtReg = (BYTE)(
        (MMA_FIFO_THR_DEFAULT << MMA_FMT_FIFO_SHIFT));

    /* Mono path: interleave is always off (stereo branched out above; call/0017). */
    fmtReg |= WaveMmaFormatInterleave(1);

    if (m_16Bit)
    {
        /* 16-bit: PIO mode — software fills FIFO with dithered data */
        fmtReg |= (MMA_DATA_FMT_12B_2 << MMA_FMT_DATA_SHIFT);
        /* ENB=0 (PIO), MSK=0 (FIFO interrupt enabled) */
    }
    else
    {
        /* 8-bit: DMA mode — hardware transfers directly. Leave the FIFO interrupt
         * unmasked (no MMA_FMT_MSK): the threshold interrupt is the only thing that
         * runs the ISR, and ServiceWaveISR gates the PIO fill on m_16Bit, so for an
         * 8-bit stream it just calls Port->Notify, which is what advances the cyclic
         * buffer position. Masking it froze 8-bit render and capture. */
        fmtReg |= (MMA_DATA_FMT_8BIT << MMA_FMT_DATA_SHIFT);
        fmtReg |= MMA_FMT_ENB;         /* DMA enabled */
    }

    ac->WriteMMA(MMA_REG_FORMAT, fmtReg);

    /*
     * For 16-bit PIO mode: reset software position and pre-fill FIFO.
     */
    if (m_16Bit)
    {
        m_SoftwarePosition = 0;
        m_ResamplePhase    = 0;     /* restart the resampler cleanly */
        m_DmaBufferSize    = m_Miniport->m_DmaChannel->BufferSize();

        if (!m_Capture)
        {
            FillFifo(FALSE);    /* PASSIVE pre-fill: synchronized MMA access */
        }
    }

    /*
     * For 8-bit DMA mode: start ISA DMA transfer.
     * Second parameter: TRUE = write-to-device (playback),
     *                   FALSE = read-from-device (capture).
     */
    if (!m_16Bit)
    {
        m_Miniport->m_DmaChannel->Start(
            m_Miniport->m_DmaChannel->BufferSize(),
            !m_Capture);
    }

    /*
     * Program register 09h to start playback or recording.
     */
    BYTE pbReg = MMA_PB_PCM | MMA_PB_GO;   /* PCM mode, start */

    /* Mono path: channel 0 drives both outputs (stereo branched out above; call/0017).
     * Derived in the pure wavereg.h helper so a test pins the mapping. */
    pbReg |= WaveMmaPlayChannelBits(1, 0);

    /* Frequency select */
    BYTE freqBits = CMiniportWaveCyclicAdLibGold::SampleRateToFreqBits(
        m_Miniport->m_SamplingFrequency);
    pbReg |= (freqBits << MMA_PB_FREQ_SHIFT);

    /* Playback vs. record */
    if (!m_Capture)
    {
        pbReg |= MMA_PB_PLAYBACK;
    }

    ac->WriteMMA(MMA_REG_PLAYBACK, pbReg);

    _DbgPrintF(DEBUGLVL_VERBOSE,
        ("ProgramMmaStart: fmt=0x%02X pb=0x%02X rate=%d %s %s",
         (ULONG)fmtReg, (ULONG)pbReg,
         m_Miniport->m_SamplingFrequency,
         m_16Bit ? "16bit-PIO" : "8bit-DMA",
         m_Capture ? "capture" : "render"));
}


/*****************************************************************************
 * CMiniportWaveCyclicStreamAdLibGold::ProgramMmaStartStereo()
 *****************************************************************************
 * Program both YMZ263 channels for stereo over the DMA interleave path and start it,
 * following the validated reference sequence (plan/0008/stereo-mma-reference.md,
 * call/0017): reset both FIFOs, program each channel's play (reg 9, GO deferred) and
 * format (reg 12) registers, arm the single interleaved DMA stream the chip
 * de-interleaves into the two FIFOs, then set GO on channel 0 (which governs GO for both
 * in interleave). Channel 0 routes to the right output with interleave set; channel 1 to
 * the left with its FIFO interrupt masked so channel 0 drives the shared interrupt.
 *
 * The reference's PIO pre-fill of channel 0 (4 bytes to reg 11) is deliberately omitted:
 * PortCls owns the ISA DMA setup here, as it does on the working 8-bit mono DMA path,
 * which likewise primes nothing. The channel-to-output mapping is attested on hardware
 * (call/0017). Reached only via ProgramMmaStart when m_Stereo.
 */
void
CMiniportWaveCyclicStreamAdLibGold::
ProgramMmaStartStereo
(   void
)
{
    PADAPTERCOMMON ac = m_Miniport->m_AdapterCommon;

    BYTE dataFmt   = (BYTE)(m_16Bit ? MMA_DATA_FMT_12B_2 : MMA_DATA_FMT_8BIT);
    BYTE freqBits  = CMiniportWaveCyclicAdLibGold::SampleRateToFreqBits(
                         m_Miniport->m_SamplingFrequency);
    BYTE freqField = (BYTE)(freqBits << MMA_PB_FREQ_SHIFT);
    BYTE prDir     = (BYTE)(m_Capture ? 0 : MMA_PB_PLAYBACK);

    /*
     * Reset both channels' FIFOs (write RST alone to reg 9), then clear.
     */
    ac->WriteMMA (MMA_REG_PLAYBACK, MMA_PB_RST);
    ac->WriteMMA1(MMA_REG_PLAYBACK, MMA_PB_RST);
    KeStallExecutionProcessor(1);
    ac->WriteMMA (MMA_REG_PLAYBACK, 0x00);
    ac->WriteMMA1(MMA_REG_PLAYBACK, 0x00);

    /*
     * Prime channel 0's FIFO with four zero bytes on render, directly after the
     * reset, exactly as the validated reference does before GO
     * (plan/0008/stereo-mma-reference.md :961-964, call/0029). The earlier
     * omission was justified from the 8-bit mono path, which had never actually
     * been proven on hardware.
     */
    if (!m_Capture)
    {
        ac->WriteMMA(MMA_REG_PCM_DATA, 0x00);
        ac->WriteMMA(MMA_REG_PCM_DATA, 0x00);
        ac->WriteMMA(MMA_REG_PCM_DATA, 0x00);
        ac->WriteMMA(MMA_REG_PCM_DATA, 0x00);
    }

    /*
     * Set both channels' MMA digital-audio output volume to full (reg 0Ah); the
     * power-on default is undocumented and left PCM silent (call/0025).
     */
    ac->WriteMMA (MMA_REG_VOLUME, 0xFF);
    ac->WriteMMA1(MMA_REG_VOLUME, 0xFF);

    /*
     * Program each channel's play/rate register (reg 9), GO deferred. Channel 0 -> right
     * output, channel 1 -> left. Values decode to the reference's PRC_0=0x46, PRC_1=0x26.
     */
    BYTE pb0 = (BYTE)(MMA_PB_PCM | prDir | freqField |
                      WaveMmaPlayChannelBits(2, 0));   /* R output */
    BYTE pb1 = (BYTE)(MMA_PB_PCM | prDir | freqField |
                      WaveMmaPlayChannelBits(2, 1));   /* L output */
    ac->WriteMMA (MMA_REG_PLAYBACK, pb0);
    ac->WriteMMA1(MMA_REG_PLAYBACK, pb1);

    /*
     * Program each channel's format register (reg 12). Channel 0: interleave on, FIFO
     * interrupt at 96 bytes, interrupt enabled, DMA. Channel 1: no interleave, FIFO
     * interrupt at 112 bytes, interrupt masked, DMA. At 16-bit these are the validated
     * index-5 values SFC_0=0xC5 and SFC_1=0x43, built from the driver's field constants.
     */
    BYTE fmt0 = (BYTE)((dataFmt << MMA_FMT_DATA_SHIFT) |
                       (MMA_FIFO_THR_96 << MMA_FMT_FIFO_SHIFT) |
                       WaveMmaFormatInterleave(2) |     /* ILV on channel 0 */
                       MMA_FMT_ENB);                    /* DMA, interrupt enabled */
    BYTE fmt1 = (BYTE)((dataFmt << MMA_FMT_DATA_SHIFT) |
                       (MMA_FIFO_THR_112 << MMA_FMT_FIFO_SHIFT) |
                       MMA_FMT_MSK | MMA_FMT_ENB);      /* interrupt masked, DMA */
    ac->WriteMMA (MMA_REG_FORMAT, fmt0);
    ac->WriteMMA1(MMA_REG_FORMAT, fmt1);

    /*
     * Arm the single interleaved DMA stream; the chip de-interleaves it into the two
     * FIFOs. TRUE = write-to-device (playback), FALSE = read-from-device (capture).
     */
    m_Miniport->m_DmaChannel->Start(
        m_Miniport->m_DmaChannel->BufferSize(), !m_Capture);

    /*
     * Set GO on channel 0 to start both channels.
     */
    ac->WriteMMA(MMA_REG_PLAYBACK, (BYTE)(pb0 | MMA_PB_GO));

    _DbgPrintF(DEBUGLVL_VERBOSE,
        ("ProgramMmaStartStereo: fmt0=0x%02X fmt1=0x%02X pb0=0x%02X pb1=0x%02X rate=%d %s",
         (ULONG)fmt0, (ULONG)fmt1, (ULONG)pb0, (ULONG)pb1,
         m_Miniport->m_SamplingFrequency,
         m_Capture ? "capture" : "render"));
}


/*****************************************************************************
 * CMiniportWaveCyclicStreamAdLibGold::ProgramMmaStop()
 *****************************************************************************
 * Reset MMA and stop all transfers.
 */
void
CMiniportWaveCyclicStreamAdLibGold::
ProgramMmaStop
(   void
)
{
    PADAPTERCOMMON ac = m_Miniport->m_AdapterCommon;

    /* Reset the MMA engine */
    ac->WriteMMA(MMA_REG_PLAYBACK, MMA_PB_RST);
    KeStallExecutionProcessor(1);
    ac->WriteMMA(MMA_REG_PLAYBACK, 0x00);

    /* Mask FIFO interrupt and disable DMA */
    ac->WriteMMA(MMA_REG_FORMAT, MMA_FMT_MSK);

    if (m_Stereo)
    {
        /* Stereo also drives channel 1 (call/0017): reset and mask it too. */
        ac->WriteMMA1(MMA_REG_PLAYBACK, MMA_PB_RST);
        KeStallExecutionProcessor(1);
        ac->WriteMMA1(MMA_REG_PLAYBACK, 0x00);
        ac->WriteMMA1(MMA_REG_FORMAT, MMA_FMT_MSK);
    }

    /* Stop DMA channel if it was running (8-bit mono or any stereo) */
    if (!m_16Bit || m_Stereo)
    {
        m_Miniport->m_DmaChannel->Stop();
    }

    m_SoftwarePosition = 0;
}


/*****************************************************************************
 * CMiniportWaveCyclicStreamAdLibGold::FillFifo()
 *****************************************************************************
 * 16-bit playback PIO: read samples from the DMA buffer, resample an off-rate
 * mono source to the hardware rate (call/0011), apply TPDF dither, truncate to
 * 12-bit, and write byte pairs to the FIFO.
 *
 * The read position advances by m_ResampleStep (Q16 input samples per output);
 * when it is 0x10000 (on-rate, stereo, or 8-bit) the fill is the plain 1:1 copy.
 * Called from the DPC (via PortCls notification) or during pre-fill. Writes up to
 * MMA_FIFO_SIZE bytes to the FIFO.
 */
#pragma code_seg()

void
CMiniportWaveCyclicStreamAdLibGold::
FillFifo
(   IN      BOOLEAN AtDirql
)
{
    PADAPTERCOMMON ac = m_Miniport->m_AdapterCommon;
    PUCHAR pBuffer = (PUCHAR)m_Miniport->m_DmaChannel->SystemAddress();

    if (!pBuffer)
        return;

    /*
     * Refill the bytes the hardware drained since the interrupt fired at the FIFO INT
     * level: FIFO_SIZE - MMA_FIFO_INT_BYTES = 96 bytes (manual ch07). Writing the full
     * 128 would overflow the FIFO and set the OV error. Each 16-bit sample writes a
     * 2-byte format-2 pair.
     */
    ULONG bytesToWrite = MMA_FIFO_SIZE - MMA_FIFO_INT_BYTES;
    ULONG bytesWritten = 0;

    while (bytesWritten < bytesToWrite)
    {
        /* Read this sample and its neighbour, then interpolate at the fractional
         * read position (identity when m_ResampleStep == 0x10000). */
        SHORT s0 = *((SHORT *)(pBuffer + m_SoftwarePosition));
        ULONG nextPos = WaveWrapPosition(m_SoftwarePosition, 2, m_DmaBufferSize);
        SHORT s1 = *((SHORT *)(pBuffer + nextPos));

        SHORT sample = WaveSrcInterp(s0, s1, m_ResamplePhase);

        /* Apply TPDF dither and truncate to 12-bit */
        SHORT dithered = DitherSample(sample, &m_LfsrState);

        /* Write in format 2 byte order: low byte first, high byte second. Pick the MMA
         * accessor by context: from the ISR (AtDirql) the raw WriteMMALocked, since the
         * public form would re-enter the interrupt lock; from the PASSIVE pre-fill the
         * synchronized WriteMMA, so the index/data pair stays atomic against the ISR. */
        if (AtDirql)
        {
            ac->WriteMMALocked(MMA_REG_PCM_DATA, (BYTE)(dithered & 0xFF));
            ac->WriteMMALocked(MMA_REG_PCM_DATA, (BYTE)((dithered >> 8) & 0xFF));
        }
        else
        {
            ac->WriteMMA(MMA_REG_PCM_DATA, (BYTE)(dithered & 0xFF));
            ac->WriteMMA(MMA_REG_PCM_DATA, (BYTE)((dithered >> 8) & 0xFF));
        }

        bytesWritten += 2;

        /* Advance the fractional read position, stepping over whole input samples.
         * step < 0x10000 upsamples (reuses a sample), step > 0x10000 downsamples. */
        m_ResamplePhase += m_ResampleStep;
        while (m_ResamplePhase >= 0x10000)
        {
            m_ResamplePhase -= 0x10000;
            m_SoftwarePosition =            /* 2 bytes per 16-bit sample */
                WaveWrapPosition(m_SoftwarePosition, 2, m_DmaBufferSize);
        }
    }
}


/*****************************************************************************
 * CMiniportWaveCyclicStreamAdLibGold::DrainFifo()
 *****************************************************************************
 * 16-bit capture PIO: read byte pairs from FIFO, store as 16-bit in buffer.
 * Lower 4 bits are zero (12-bit hardware resolution).
 */
void
CMiniportWaveCyclicStreamAdLibGold::
DrainFifo
(   void
)
{
    PADAPTERCOMMON ac = m_Miniport->m_AdapterCommon;
    PUCHAR pBuffer = (PUCHAR)m_Miniport->m_DmaChannel->SystemAddress();

    if (!pBuffer)
        return;

    /*
     * Drain only the bytes guaranteed present at the FIFO INT level (manual ch07):
     * MMA_FIFO_INT_BYTES. Reading a full FIFO would underflow and store stale bytes.
     * Continuous capture depends on the record FIFO tuning and is confirmed on hardware
     * (plan/0008 attested); this moves the guaranteed data without underflow.
     */
    ULONG bytesToRead = MMA_FIFO_INT_BYTES;
    ULONG bytesRead = 0;

    while (bytesRead < bytesToRead)
    {
        /* Read format 2 byte pair from FIFO */
        /* ReadMMALocked: DrainFifo runs at DIRQL inside the ISR's interrupt-sync. */
        BYTE lo = ac->ReadMMALocked(MMA_REG_PCM_DATA);
        BYTE hi = ac->ReadMMALocked(MMA_REG_PCM_DATA);

        /* Store as 16-bit in the cyclic buffer */
        pBuffer[m_SoftwarePosition]     = lo;
        pBuffer[m_SoftwarePosition + 1] = hi;

        bytesRead += 2;
        m_SoftwarePosition =
            WaveWrapPosition(m_SoftwarePosition, 2, m_DmaBufferSize);
    }
}


/*****************************************************************************
 * CMiniportWaveCyclicStreamAdLibGold::GetPosition()
 *****************************************************************************
 * Return the current byte position in the DMA buffer.
 * Called at DISPATCH_LEVEL — must be non-paged.
 *
 * 8-bit DMA mode: read from ISA DMA counter.
 * 16-bit PIO mode: return software-tracked position.
 */
STDMETHODIMP
CMiniportWaveCyclicStreamAdLibGold::
GetPosition
(
    OUT     PULONG  Position
)
{
    ASSERT(Position);

    if (m_16Bit && !m_Stereo)
    {
        /*
         * 16-bit mono PIO mode: software position tracks where we've read/written.
         * Stereo (and 8-bit) run over DMA, where the software position never advances,
         * so they must use the DMA counter below (call/0017).
         */
        *Position = m_SoftwarePosition;
    }
    else
    {
        /*
         * DMA mode: position = BufferSize - bytes remaining.
         */
        if (m_Miniport->m_DmaChannel)
        {
            ULONG transferCount = m_Miniport->m_DmaChannel->TransferCount();

            if (transferCount)
            {
                ULONG counter = m_Miniport->m_DmaChannel->ReadCounter();
                *Position = (counter != 0) ? (transferCount - counter) : 0;
            }
            else
            {
                *Position = 0;
            }
        }
        else
        {
            *Position = 0;
        }
    }

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CMiniportWaveCyclicStreamAdLibGold::NormalizePhysicalPosition()
 *****************************************************************************
 * Convert a byte offset in the DMA buffer to a time in 100ns units.
 * Called at DISPATCH_LEVEL — must be non-paged.
 */
STDMETHODIMP
CMiniportWaveCyclicStreamAdLibGold::
NormalizePhysicalPosition
(
    IN OUT  PLONGLONG   PhysicalPosition
)
{
    ULONG bytesPerFrame = (1 << (m_Stereo + m_16Bit));

    *PhysicalPosition =
        (_100NS_UNITS_PER_SECOND / bytesPerFrame * *PhysicalPosition) /
            m_Miniport->m_SamplingFrequency;

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CMiniportWaveCyclicStreamAdLibGold::Silence()
 *****************************************************************************
 * Fill a buffer region with silence.
 * Called at DISPATCH_LEVEL — must be non-paged.
 * 8-bit unsigned PCM: 0x80 is silence.
 * 16-bit signed PCM: 0x00 is silence.
 */
STDMETHODIMP_(void)
CMiniportWaveCyclicStreamAdLibGold::
Silence
(
    IN      PVOID   Buffer,
    IN      ULONG   ByteCount
)
{
    RtlFillMemory(Buffer, ByteCount, m_16Bit ? 0 : 0x80);
}


#pragma code_seg("PAGE")

/*****************************************************************************
 * CMiniportWaveCyclicStreamAdLibGold::SetNotificationFreq()
 *****************************************************************************
 * Set the notification interval and return the framing size.
 */
STDMETHODIMP_(ULONG)
CMiniportWaveCyclicStreamAdLibGold::
SetNotificationFreq
(
    IN      ULONG   Interval,
    OUT     PULONG  FramingSize
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE,
        ("[CMiniportWaveCyclicStreamAdLibGold::SetNotificationFreq %d ms]",
         Interval));

    m_Miniport->m_NotificationInterval = Interval;

    ULONG bytesPerFrame = (1 << (m_Stereo + m_16Bit));

    *FramingSize =
        bytesPerFrame *
        (m_Miniport->m_SamplingFrequency * Interval / 1000);

    return m_Miniport->m_NotificationInterval;
}
