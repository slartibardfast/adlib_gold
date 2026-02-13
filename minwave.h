/*****************************************************************************
 * minwave.h - Ad Lib Gold wave miniport private definitions
 *****************************************************************************
 *
 * WaveCyclic miniport for YMZ263 (MMA) digital audio.  Supports 8-bit
 * PCM via DMA and 16-bit PCM via PIO with TPDF dithering for clean
 * 16-bit to 12-bit conversion.
 */

#ifndef _ALGWAVE_PRIVATE_H_
#define _ALGWAVE_PRIVATE_H_

#include "common.h"


/*****************************************************************************
 * MMA register indices (YMZ263 — used by wave miniport only)
 *
 * Accessed via IAdapterCommon::WriteMMA / ReadMMA (MMA Channel 0).
 */
#define MMA_REG_STATUS      0x00    /* Read: status flags (auto-clear)       */
#define MMA_REG_PLAYBACK    0x09    /* Write: playback/record control        */
#define MMA_REG_VOLUME      0x0A    /* Write: output volume (0x00-0xFF)      */
#define MMA_REG_PCM_DATA    0x0B    /* R/W: FIFO data port                   */
#define MMA_REG_FORMAT      0x0C    /* Write: format/DMA/FIFO control        */

/*****************************************************************************
 * Register 0x09 (Playback/Record Control) bit definitions
 */
#define MMA_PB_RST          0x80    /* D7: Reset (write alone, then config)  */
#define MMA_PB_RIGHT        0x40    /* D6: Enable right channel              */
#define MMA_PB_LEFT         0x20    /* D5: Enable left channel               */
#define MMA_PB_FREQ_SHIFT   3       /* D4-D3: Frequency select               */
#define MMA_PB_FREQ_MASK    0x18
#define MMA_PB_PCM          0x04    /* D2: 1=PCM, 0=ADPCM                   */
#define MMA_PB_PLAYBACK     0x02    /* D1: 1=Playback, 0=Record             */
#define MMA_PB_GO           0x01    /* D0: Start                             */

/*
 * FREQ field values -> PCM sample rates
 *   0 = 44100 Hz
 *   1 = 22050 Hz
 *   2 = 11025 Hz
 *   3 = 7350  Hz
 */
#define MMA_FREQ_44100      0
#define MMA_FREQ_22050      1
#define MMA_FREQ_11025      2
#define MMA_FREQ_7350       3

/*****************************************************************************
 * Register 0x0C (Format/DMA/FIFO Control) bit definitions
 */
#define MMA_FMT_ILV         0x80    /* D7: Interleave (ch0 only)             */
#define MMA_FMT_DATA_SHIFT  5       /* D6-D5: Data format                    */
#define MMA_FMT_DATA_MASK   0x60
#define MMA_FMT_FIFO_SHIFT  2       /* D4-D2: FIFO interrupt threshold       */
#define MMA_FMT_FIFO_MASK   0x1C
#define MMA_FMT_MSK         0x02    /* D1: 1=mask FIFO IRQ, 0=enable         */
#define MMA_FMT_ENB         0x01    /* D0: DMA mode enable                   */

/*
 * DATA FMT values
 *   0 = 8-bit (1 byte, MSB only)
 *   1 = 12-bit (2-byte format 1)
 *   2 = 12-bit (2-byte format 2 — LE-compatible with 16-bit PCM)
 *   3 = invalid
 */
#define MMA_DATA_FMT_8BIT   0
#define MMA_DATA_FMT_12B_1  1
#define MMA_DATA_FMT_12B_2  2       /* Use this for 16-bit Windows PCM       */

/*
 * FIFO threshold values (bytes remaining before interrupt)
 *   5 = 32 bytes  (good balance of latency vs. overhead)
 */
#define MMA_FIFO_THR_112    0
#define MMA_FIFO_THR_96     1
#define MMA_FIFO_THR_80     2
#define MMA_FIFO_THR_64     3
#define MMA_FIFO_THR_48     4
#define MMA_FIFO_THR_32     5
#define MMA_FIFO_THR_16     6

/* Default threshold: 32 bytes — enough to avoid underrun at DPC latency */
#define MMA_FIFO_THR_DEFAULT    MMA_FIFO_THR_32

/* FIFO size in bytes */
#define MMA_FIFO_SIZE           128


/*****************************************************************************
 * TPDF dither helpers (integer only — no FPU in kernel mode)
 *
 * 16-bit Galois LFSR for pseudo-random generation.
 * Triangular PDF dither at +/- 1 LSB in 12-bit scale.
 */

/*
 * Advance a 16-bit Galois LFSR by one step.
 * Polynomial: x^16 + x^14 + x^13 + x^11 + 1 (maximal-length).
 */
static USHORT
LfsrNext
(
    IN      USHORT  State
)
{
    USHORT bit = ((State >> 0) ^ (State >> 2) ^
                  (State >> 3) ^ (State >> 5)) & 1;
    return (USHORT)((State >> 1) | (bit << 15));
}

/*
 * Apply TPDF dither to a 16-bit signed sample and truncate to 12-bit.
 * Returns a 16-bit value with the lower 4 bits cleared.
 */
static SHORT
DitherSample
(
    IN      SHORT       Sample16,
    IN OUT  USHORT *    pLfsr
)
{
    /*
     * Two uniform random values in [-8, +7] summed give triangular
     * PDF in [-16, +14].  One LSB at 12-bit resolution = 16 at
     * 16-bit resolution, so this is +/- 1 LSB dither.
     */
    LONG r1 = (LONG)(*pLfsr & 0x0F) - 8;
    *pLfsr = LfsrNext(*pLfsr);
    LONG r2 = (LONG)(*pLfsr & 0x0F) - 8;
    *pLfsr = LfsrNext(*pLfsr);

    LONG dithered = (LONG)Sample16 + r1 + r2;

    /* Clamp to signed 16-bit range */
    if (dithered > 32767)  dithered = 32767;
    if (dithered < -32768) dithered = -32768;

    /* Truncate: clear lower 4 bits */
    return (SHORT)(dithered & 0xFFF0);
}


/*****************************************************************************
 * Pin identifiers
 *
 * Match the order in MiniportPins[] (defined in minwave.cpp).
 * Same layout as SB16 DDK sample.
 */
enum
{
    WAVE_PIN_CAPTURE_STREAM = 0,    /* Capture streaming (data out)          */
    WAVE_PIN_CAPTURE_BRIDGE,        /* Capture bridge (from topology)        */
    WAVE_PIN_RENDER_STREAM,         /* Render streaming (data in)            */
    WAVE_PIN_RENDER_BRIDGE          /* Render bridge (to topology)           */
};


/*****************************************************************************
 * Forward declarations
 */
class CMiniportWaveCyclicStreamAdLibGold;


/*****************************************************************************
 * CMiniportWaveCyclicAdLibGold
 *****************************************************************************
 * WaveCyclic miniport for the Ad Lib Gold YMZ263 digital audio subsystem.
 *
 * 8-bit PCM: DMA mode (ISA DMA transfers to FIFO directly).
 * 16-bit PCM: PIO mode with TPDF dithering (software fills FIFO in DPC).
 */
class CMiniportWaveCyclicAdLibGold
:   public IMiniportWaveCyclic,
    public IWaveMiniportAdLibGold,
    public IPowerNotify,
    public CUnknown
{
private:
    PADAPTERCOMMON      m_AdapterCommon;        /* Shared HW access          */
    PPORTWAVECYCLIC     m_Port;                 /* Callback interface        */
    PSERVICEGROUP       m_ServiceGroup;         /* Notification service group */
    PDMACHANNELSLAVE    m_DmaChannel;           /* Slave DMA channel         */

    BOOLEAN             m_CaptureAllocated;     /* Capture stream active     */
    BOOLEAN             m_RenderAllocated;      /* Render stream active      */
    ULONG               m_SamplingFrequency;    /* Current sample rate       */
    ULONG               m_NotificationInterval; /* ms between notifications  */

    POWER_STATE         m_PowerState;           /* Current device power      */

    /*
     * Private methods
     */
    NTSTATUS ProcessResources
    (
        IN      PRESOURCELIST   ResourceList
    );

    void ConfigureDmaAndIrq
    (
        IN      PRESOURCELIST   ResourceList
    );

public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CMiniportWaveCyclicAdLibGold);

    ~CMiniportWaveCyclicAdLibGold();

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
    ,   IN      PKSDATARANGE    ClientDataRange
    ,   IN      PKSDATARANGE    MyDataRange
    ,   IN      ULONG           OutputBufferLength
    ,   OUT     PVOID           ResultantFormat     OPTIONAL
    ,   OUT     PULONG          ResultantFormatLength
    );

    /*
     * IMiniportWaveCyclic methods
     */
    STDMETHODIMP Init
    (
        IN      PUNKNOWN        UnknownAdapter,
        IN      PRESOURCELIST   ResourceList,
        IN      PPORTWAVECYCLIC Port
    );
    STDMETHODIMP NewStream
    (
        OUT     PMINIPORTWAVECYCLICSTREAM * OutStream,
        IN      PUNKNOWN                    OuterUnknown    OPTIONAL,
        IN      POOL_TYPE                   PoolType,
        IN      ULONG                       Pin,
        IN      BOOLEAN                     Capture,
        IN      PKSDATAFORMAT               DataFormat,
        OUT     PDMACHANNEL *               OutDmaChannel,
        OUT     PSERVICEGROUP *             OutServiceGroup
    );

    /*
     * IWaveMiniportAdLibGold (ISR dispatch from adapter common)
     */
    STDMETHODIMP_(void) ServiceWaveISR
    (   void
    );

    /*
     * IPowerNotify
     */
    IMP_IPowerNotify;

    /*
     * Helpers accessible by stream
     */
    NTSTATUS ValidateFormat
    (
        IN      PKSDATAFORMAT   Format
    );

    static BYTE SampleRateToFreqBits
    (
        IN      ULONG           SampleRate
    );

    /*
     * Friends
     */
    friend class CMiniportWaveCyclicStreamAdLibGold;
};


/*****************************************************************************
 * CMiniportWaveCyclicStreamAdLibGold
 *****************************************************************************
 * WaveCyclic stream for a single playback or capture instance.
 *
 * For 8-bit formats, uses hardware DMA (ENB=1 in reg 0Ch).
 * For 16-bit formats, uses PIO with TPDF dithering (ENB=0, FIFO interrupt).
 */
class CMiniportWaveCyclicStreamAdLibGold
:   public IMiniportWaveCyclicStream,
    public CUnknown
{
private:
    CMiniportWaveCyclicAdLibGold *   m_Miniport;    /* Parent miniport       */
    BOOLEAN     m_Capture;              /* TRUE for record, FALSE for play   */
    BOOLEAN     m_16Bit;                /* TRUE for 16-bit (PIO+dither)      */
    BOOLEAN     m_Stereo;               /* TRUE for stereo                   */
    KSSTATE     m_State;                /* Current stream state              */

    /* PIO mode state (16-bit only) */
    ULONG       m_SoftwarePosition;     /* Read/write position in DMA buffer */
    ULONG       m_DmaBufferSize;        /* Size of allocated DMA buffer      */
    USHORT      m_LfsrState;            /* LFSR state for dither generation  */

    /*
     * Private methods
     */
    void FillFifo(void);                /* 16-bit playback: dither + write   */
    void DrainFifo(void);               /* 16-bit capture: read + zero-pad   */

    void ProgramMmaStart(void);         /* Write regs 0Ch + 09h to start    */
    void ProgramMmaStop(void);          /* Reset reg 09h, mask FIFO IRQ     */

public:
    NTSTATUS
    Init
    (
        IN      CMiniportWaveCyclicAdLibGold *  Miniport,
        IN      BOOLEAN                         Capture,
        IN      PKSDATAFORMAT                   DataFormat
    );

    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CMiniportWaveCyclicStreamAdLibGold);

    ~CMiniportWaveCyclicStreamAdLibGold();

    /*
     * IMiniportWaveCyclicStream methods
     */
    STDMETHODIMP SetFormat
    (
        IN      PKSDATAFORMAT   DataFormat
    );
    STDMETHODIMP SetState
    (
        IN      KSSTATE         NewState
    );
    STDMETHODIMP GetPosition
    (
        OUT     PULONG          Position
    );
    STDMETHODIMP NormalizePhysicalPosition
    (
        IN OUT  PLONGLONG       PhysicalPosition
    );
    STDMETHODIMP_(ULONG) SetNotificationFreq
    (
        IN      ULONG           Interval,
        OUT     PULONG          FramingSize
    );
    STDMETHODIMP_(void) Silence
    (
        IN      PVOID           Buffer,
        IN      ULONG           ByteCount
    );
};


#endif  /* _ALGWAVE_PRIVATE_H_ */
