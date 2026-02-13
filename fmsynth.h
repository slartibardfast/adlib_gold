/*****************************************************************************
 * fmsynth.h - Ad Lib Gold FM synth miniport private definitions
 *****************************************************************************
 *
 * OPL3 (YMF262) FM synthesis MIDI miniport.  Adapted from the Windows 2000
 * DDK fmsynth sample driver.  Hardware access is delegated to the adapter
 * common object's WriteOPL3() method which handles bank switching between
 * OPL3 array 1 and the Control Chip.
 */

#ifndef _ALGFMSYNTH_PRIVATE_H_
#define _ALGFMSYNTH_PRIVATE_H_

#include "common.h"


/*****************************************************************************
 * MIDI defines
 */
#define NUMCHANNELS                     (16)
#define NUMPATCHES                      (256)
#define DRUMCHANNEL                     (9)     /* MIDI channel 10 */

#define NUM2VOICES                      18
#define NUMOPS                          4


/*****************************************************************************
 * Utility macros
 */
#define AsULMUL(a, b)   ((DWORD)((DWORD)(a) * (DWORD)(b)))
#define AsLSHL(a, b)    ((DWORD)((DWORD)(a) << (DWORD)(b)))
#define AsULSHR(a, b)   ((DWORD)((DWORD)(a) >> (DWORD)(b)))


/*****************************************************************************
 * Indexed FM (OPL3) register addresses
 *
 * Addresses 0x000-0x0FF are bank 0 (ports base+0/1).
 * Addresses 0x100-0x1FF are bank 1 (ports base+2/3, bank-switched).
 */
#define AD_LSI                          (0x000)
#define AD_LSI2                         (0x101)
#define AD_TIMER1                       (0x001)
#define AD_TIMER2                       (0x002)
#define AD_MASK                         (0x004)
#define AD_CONNECTION                   (0x104)
#define AD_NEW                          (0x105)
#define AD_NTS                          (0x008)
#define AD_MULT                         (0x020)
#define AD_MULT2                        (0x120)
#define AD_LEVEL                        (0x040)
#define AD_LEVEL2                       (0x140)
#define AD_AD                           (0x060)
#define AD_AD2                          (0x160)
#define AD_SR                           (0x080)
#define AD_SR2                          (0x180)
#define AD_FNUMBER                      (0x0a0)
#define AD_FNUMBER2                     (0x1a0)
#define AD_BLOCK                        (0x0b0)
#define AD_BLOCK2                       (0x1b0)
#define AD_DRUM                         (0x0bd)
#define AD_FEEDBACK                     (0x0c0)
#define AD_FEEDBACK2                    (0x1c0)
#define AD_WAVE                         (0x0e0)
#define AD_WAVE2                        (0x1e0)


/*****************************************************************************
 * Patch type defines
 */
#define PATCH_1_4OP             (0)     /* use 4-operator patch */
#define PATCH_2_2OP             (1)     /* use two 2-operator patches */
#define PATCH_1_2OP             (2)     /* use one 2-operator patch */


/*****************************************************************************
 * Tuning constants
 *
 * The PITCH() macro and note frequency defines use compile-time
 * floating-point arithmetic.  The results are cast to DWORD and
 * become integer constants in the binary -- no FPU code is emitted.
 */
#define FSAMP                           (50000.0)
#define PITCH(x)                        ((DWORD)((x) * (double)(1L << 19) / FSAMP))

#define EQUAL                           (1.059463094359)

#ifdef EUROPE
#define A                               (442.0)
#else
#define A                               (440.0)
#endif

#define ASHARP                          (A * EQUAL)
#define B                               (ASHARP * EQUAL)
#define C                               (B * EQUAL / 2.0)
#define CSHARP                          (C * EQUAL)
#define D                               (CSHARP * EQUAL)
#define DSHARP                          (D * EQUAL)
#define E                               (DSHARP * EQUAL)
#define F                               (E * EQUAL)
#define FSHARP                          (F * EQUAL)
#define G                               (FSHARP * EQUAL)
#define GSHARP                          (G * EQUAL)


/*****************************************************************************
 * Operator and voice structures
 *
 * Packed to match the DDK patch data format.  Each patchStruct entry is
 * 28 bytes of raw OPL3 register values.
 */
#pragma pack(1)

typedef struct _operStruct {
    BYTE    bAt20;              /* flags sent to 0x20 on FM */
    BYTE    bAt40;              /* flags sent to 0x40       */
    BYTE    bAt60;              /* flags sent to 0x60       */
    BYTE    bAt80;              /* flags sent to 0x80       */
    BYTE    bAtE0;              /* flags sent to 0xE0       */
} operStruct;

typedef struct _noteStruct {
    operStruct op[NUMOPS];      /* operators                */
    BYTE    bAtA0[2];           /* sent to 0xA0, 0x1A0      */
    BYTE    bAtB0[2];           /* sent to 0xB0, 0x1B0      */
    BYTE    bAtC0[2];           /* sent to 0xC0, 0x1C0      */
    BYTE    bOp;                /* see PATCH_??? defines     */
    BYTE    bDummy;             /* padding                   */
} noteStruct;

typedef struct _patchStruct {
    noteStruct note;
} patchStruct;

#pragma pack()


/*****************************************************************************
 * Voice state structure (per-voice runtime data)
 */
typedef struct _voiceStruct {
    BYTE    bNote;              /* MIDI note played          */
    BYTE    bChannel;           /* MIDI channel              */
    BYTE    bPatch;             /* patch number (drums = note + 128) */
    BYTE    bOn;                /* TRUE if note is on        */
    BYTE    bVelocity;          /* velocity                  */
    BYTE    bJunk;              /* padding                   */
    DWORD   dwTime;             /* timestamp (0 = unused)    */
    DWORD   dwOrigPitch[2];     /* original pitch for bends  */
    BYTE    bBlock[2];          /* block register value       */
    BYTE    bSusHeld;           /* held by sustain pedal      */
} voiceStruct;


/*****************************************************************************
 * Channel enums
 */
enum {
    CHAN_MASTER = (-1),
    CHAN_LEFT   = 0,
    CHAN_RIGHT  = 1
};


/*****************************************************************************
 * Forward declarations
 */
class CMiniportMidiStreamFMAdLibGold;


/*****************************************************************************
 * CMiniportMidiFMAdLibGold
 *****************************************************************************
 * FM synth miniport.  Adapted from DDK CMiniportMidiFM.
 *
 * Key difference from DDK sample: all OPL3 register writes go through
 * m_AdapterCommon->WriteOPL3() instead of direct WRITE_PORT_UCHAR.
 * This handles the Ad Lib Gold's bank switching between OPL3 array 1
 * and the Control Chip automatically.
 */
class CMiniportMidiFMAdLibGold
:   public IMiniportMidi,
    public IPowerNotify,
    public CUnknown
{
private:
    PPORTMIDI       m_Port;                     /* Callback interface       */
    PADAPTERCOMMON  m_AdapterCommon;            /* Shared hardware access   */
    BOOLEAN         m_fStreamExists;            /* Only one stream allowed  */

    BYTE            m_SavedRegValues[0x200];    /* Shadow OPL3 registers    */
    POWER_STATE     m_PowerState;               /* Current power state      */
    KSPIN_LOCK      m_SpinLock;                 /* Hardware access serialize */

    /*
     * Private methods
     */
    void SoundMidiSendFM(ULONG Address, UCHAR Data);
    void Opl3_BoardReset(void);
    void MiniportMidiFMResume(void);

public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CMiniportMidiFMAdLibGold);

    ~CMiniportMidiFMAdLibGold();

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
     * IPowerNotify methods
     */
    IMP_IPowerNotify;

    /*
     * Friends
     */
    friend class CMiniportMidiStreamFMAdLibGold;
};


/*****************************************************************************
 * CMiniportMidiStreamFMAdLibGold
 *****************************************************************************
 * FM synth miniport stream.  Adapted from DDK CMiniportMidiStreamFM.
 * Removes m_PortBase (hardware access via m_Miniport->m_AdapterCommon)
 * and volume property support (topology miniport handles FM volume).
 */
class CMiniportMidiStreamFMAdLibGold
:   public IMiniportMidiStream,
    public CUnknown
{
private:
    CMiniportMidiFMAdLibGold *  m_Miniport;     /* Parent miniport          */

    /* Voice tracking */
    voiceStruct m_Voice[NUM2VOICES];
    DWORD       m_dwCurTime;

    /* Synth attenuation (always 0 -- topology handles FM volume) */
    WORD        m_wSynthAttenL;
    WORD        m_wSynthAttenR;

    /* Channel state */
    BYTE        m_bChanAtten[NUMCHANNELS];
    BYTE        m_bStereoMask[NUMCHANNELS];
    short       m_iBend[NUMCHANNELS];
    BYTE        m_bPatch[NUMCHANNELS];
    BYTE        m_bSustain[NUMCHANNELS];

    /*
     * Private methods -- Opl3 processing
     */
    void WriteMidiData(DWORD dwData);
    void Opl3_ChannelVolume(BYTE bChannel, WORD wAtten);
    void Opl3_SetPan(BYTE bChannel, BYTE bPan);
    void Opl3_PitchBend(BYTE bChannel, short iBend);
    void Opl3_NoteOn(BYTE bPatch, BYTE bNote, BYTE bChannel, BYTE bVelocity, short iBend);
    void Opl3_NoteOff(BYTE bPatch, BYTE bNote, BYTE bChannel, BYTE bSustain);
    void Opl3_AllNotesOff(void);
    void Opl3_ChannelNotesOff(BYTE bChannel);
    WORD Opl3_FindFullSlot(BYTE bNote, BYTE bChannel);
    WORD Opl3_CalcFAndB(DWORD dwPitch);
    DWORD Opl3_CalcBend(DWORD dwOrig, short iBend);
    BYTE Opl3_CalcVolume(BYTE bOrigAtten, BYTE bChannel, BYTE bVelocity, BYTE bOper, BYTE bMode);
    BYTE Opl3_CalcStereoMask(BYTE bChannel);
    WORD Opl3_FindEmptySlot(BYTE bPatch);
    void Opl3_SetVolume(BYTE bChannel);
    void Opl3_FMNote(WORD wNote, noteStruct FAR * lpSN);
    void Opl3_SetSustain(BYTE bChannel, BYTE bSusLevel);

public:
    NTSTATUS
    Init
    (
        IN      CMiniportMidiFMAdLibGold *  Miniport
    );

    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CMiniportMidiStreamFMAdLibGold);

    ~CMiniportMidiStreamFMAdLibGold();

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


#endif  /* _ALGFMSYNTH_PRIVATE_H_ */
