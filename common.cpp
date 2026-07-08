/*****************************************************************************
 * common.cpp - Common code used by all the Ad Lib Gold miniports.
 *****************************************************************************
 *
 * Implementation of the adapter common object.  Handles Control Chip
 * register access with bank switching, interrupt dispatch, mixer shadow
 * cache with registry persistence, and power management.
 */

#include "common.h"
#include "sp2.h"        /* SP2 bit-serial encoder (call/0012), unit-tested */
#include "chiptiming.h"  /* calibrated chip-write delays (call/0013), unit-tested */

#define STR_MODULENAME "AdLibGold: "


/*****************************************************************************
 * CAdapterCommon
 *****************************************************************************
 * Adapter common object.
 */
/*
 * Synchronized shared-port access (plan/0008, spec/BankAccess.tla). The Control Chip
 * and OPL3 array 1 share the base+2/base+3 ports; every access is an
 * enable/write/restore sequence that the DIRQL ISR must not interleave. Each write to
 * those ports runs inside the one interrupt-sync lock through these routines. The
 * context carries the register (or OPL3 address) and value into the locked body.
 */
class CAdapterCommon;

typedef struct _SYNC_PORT_CONTEXT
{
    CAdapterCommon *That;
    ULONG           Address;
    BYTE            Value;
} SYNC_PORT_CONTEXT, *PSYNC_PORT_CONTEXT;

NTSTATUS SynchronizedControlRegWrite(IN PINTERRUPTSYNC, IN PVOID);
NTSTATUS SynchronizedWriteOPL3Bank1(IN PINTERRUPTSYNC, IN PVOID);
NTSTATUS SynchronizedWriteMMA(IN PINTERRUPTSYNC, IN PVOID);
NTSTATUS SynchronizedReadMMA(IN PINTERRUPTSYNC, IN PVOID);

class CAdapterCommon
:   public IAdapterCommon,
    public IAdapterPowerManagement,
    public CUnknown
{
private:
    PINTERRUPTSYNC          m_pInterruptSync;
    PUCHAR                  m_pPortBase;
    PDEVICE_OBJECT          m_pDeviceObject;
    DEVICE_POWER_STATE      m_PowerState;
    BYTE                    m_ControlRegs[CTRL_REG_MAX];
    BYTE                    m_CardModel;
    BYTE                    m_CardOptions;
    BOOLEAN                 m_EepromPersist;    /* Registry-gated card-EEPROM save (call/0019) */
    PWAVEMINIPORTADLIBGOLD  m_pWaveMiniport;
    PMIDIMINIPORTADLIBGOLD  m_pMidiMiniport;

    BOOLEAN WaitForReady(void);

    /* Locked bodies run at DIRQL inside the interrupt-sync routine (plan/0008). */
    void ControlRegWriteLocked(IN BYTE Register, IN BYTE Value);
    void WriteOPL3Bank1Locked(IN ULONG Address, IN BYTE Data);

    friend NTSTATUS SynchronizedControlRegWrite(IN PINTERRUPTSYNC, IN PVOID);
    friend NTSTATUS SynchronizedWriteOPL3Bank1(IN PINTERRUPTSYNC, IN PVOID);

public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CAdapterCommon);
    ~CAdapterCommon();

    /*****************************************************************************
     * IAdapterCommon methods
     */
    STDMETHODIMP_(NTSTATUS) Init
    (
        IN      PRESOURCELIST   ResourceList,
        IN      PDEVICE_OBJECT  DeviceObject
    );
    STDMETHODIMP_(PINTERRUPTSYNC) GetInterruptSync
    (   void
    );
    STDMETHODIMP_(void) ControlRegWrite
    (
        IN      BYTE    Register,
        IN      BYTE    Value
    );
    STDMETHODIMP_(BYTE) ControlRegRead
    (
        IN      BYTE    Register
    );
    STDMETHODIMP_(void) ControlRegReset
    (   void
    );
    STDMETHODIMP_(void) EnableControlBank
    (   void
    );
    STDMETHODIMP_(void) EnableOPL3Bank1
    (   void
    );
    STDMETHODIMP_(void) WriteSurroundReg
    (   BYTE    Register,
        BYTE    Value
    );
    STDMETHODIMP_(void) WriteOPL3
    (
        IN      ULONG   Address,
        IN      UCHAR   Data
    );
    STDMETHODIMP_(void) WriteMMA
    (
        IN      BYTE    Register,
        IN      BYTE    Value
    );
    STDMETHODIMP_(BYTE) ReadMMA
    (
        IN      BYTE    Register
    );
    STDMETHODIMP_(void) WriteMMALocked
    (
        IN      BYTE    Register,
        IN      BYTE    Value
    );
    STDMETHODIMP_(BYTE) ReadMMALocked
    (
        IN      BYTE    Register
    );
    STDMETHODIMP_(BYTE) ReadMMAStatus
    (   void
    );
    STDMETHODIMP_(void) WriteMMA1
    (
        IN      BYTE    Register,
        IN      BYTE    Value
    );
    STDMETHODIMP_(void) SetWaveMiniport(IN PWAVEMINIPORTADLIBGOLD Miniport);
    STDMETHODIMP_(void) SetMidiMiniport(IN PMIDIMINIPORTADLIBGOLD Miniport);
    STDMETHODIMP_(NTSTATUS) RestoreMixerSettingsFromRegistry
    (   void
    );
    STDMETHODIMP_(NTSTATUS) SaveMixerSettingsToRegistry
    (   void
    );
    STDMETHODIMP_(NTSTATUS) SaveToEEPROM
    (   void
    );
    STDMETHODIMP_(NTSTATUS) RestoreFromEEPROM
    (   void
    );
    STDMETHODIMP_(BYTE) GetCardModel
    (   void
    );

    /*************************************************************************
     * IAdapterPowerManagement implementation
     *
     * This macro is from PORTCLS.H.  It lists all the interface's functions.
     */
    IMP_IAdapterPowerManagement;

    friend
    NTSTATUS
    InterruptServiceRoutine
    (
        IN      PINTERRUPTSYNC  InterruptSync,
        IN      PVOID           DynamicContext
    );
};


/*****************************************************************************
 * Default mixer settings for registry persistence
 *
 * Covers Control Chip registers 0x04-0x0F (all volume/tone/mode controls).
 * Values chosen for safe mid-range defaults.
 */
static
MIXERSETTING DefaultMixerSettings[] =
{
    /*                               Reg    Default                         */
    { L"LeftMasterVol",  CTRL_REG_MASTER_VOL_L,  0xFC },  /* 0dB. Reg 04/05 is a dB code (audible 0xDC-0xFF); 0xD8 was -80dB OFF (call/0025) */
    { L"RightMasterVol", CTRL_REG_MASTER_VOL_R,  0xFC },
    { L"Bass",           CTRL_REG_BASS,           0xF6 },  /* 0dB flat, D7-D4 set */
    { L"Treble",         CTRL_REG_TREBLE,         0xF6 },  /* 0dB flat, D7-D4 set */
    { L"OutputMode",     CTRL_REG_OUTPUT_MODE,    0xCE },  /* Linear stereo, both ch, unmuted (call/0029) */
    { L"LeftFMVol",      CTRL_REG_FM_VOL_L,       0xC0 },  /* Mid-range (192 of 128-255) */
    { L"RightFMVol",     CTRL_REG_FM_VOL_R,       0xC0 },
    { L"LeftSampVol",    CTRL_REG_SAMP_VOL_L,     0xC0 },
    { L"RightSampVol",   CTRL_REG_SAMP_VOL_R,     0xC0 },
    { L"LeftAuxVol",     CTRL_REG_AUX_VOL_L,      0xC0 },
    { L"RightAuxVol",    CTRL_REG_AUX_VOL_R,      0xC0 },
    { L"MicVol",         CTRL_REG_MIC_VOL,         0x80 },  /* Silent */
    { L"LeftRecordGain", CTRL_REG_GAIN_L,          0x80 },  /* ADC record level, mid */
    { L"RightRecordGain",CTRL_REG_GAIN_R,          0x80 },
};


/*****************************************************************************
 * Pageable code
 */
#pragma code_seg("PAGE")

/*****************************************************************************
 * NewAdapterCommon()
 *****************************************************************************
 * Create a new adapter common object.
 */
NTSTATUS
NewAdapterCommon
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
)
{
    PAGED_CODE();

    ASSERT(Unknown);

    STD_CREATE_BODY_
    (
        CAdapterCommon,
        Unknown,
        UnknownOuter,
        PoolType,
        PADAPTERCOMMON
    );
}


/*****************************************************************************
 * CAdapterCommon::Init()
 *****************************************************************************
 * Initialize the adapter common object.  Detects the card, sets up the
 * interrupt sync object, and initializes the Control Chip to a known state.
 */
NTSTATUS
CAdapterCommon::
Init
(
    IN      PRESOURCELIST   ResourceList,
    IN      PDEVICE_OBJECT  DeviceObject
)
{
    PAGED_CODE();

    ASSERT(ResourceList);
    ASSERT(DeviceObject);

    /*
     * Validate resources: need at least one I/O port range and one IRQ.
     */
    if ((ResourceList->NumberOfPorts() < 1) ||
        (ResourceList->NumberOfInterrupts() < 1))
    {
        _DbgPrintF(DEBUGLVL_TERSE,
            ("Init: insufficient resources (ports=%d, IRQs=%d)",
             ResourceList->NumberOfPorts(),
             ResourceList->NumberOfInterrupts()));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    m_pDeviceObject     = DeviceObject;
    m_pWaveMiniport     = NULL;
    m_pMidiMiniport     = NULL;
    m_pInterruptSync    = NULL;

    /*
     * Get the base I/O address from the resource list.
     */
    ASSERT(ResourceList->FindTranslatedPort(0));
    m_pPortBase = PUCHAR(ResourceList->FindTranslatedPort(0)->u.Port.Start.LowPart);

    /*
     * Set initial power state.
     */
    m_PowerState = PowerDeviceD0;

    /* Card-EEPROM save is off until the registry flag turns it on (call/0019). */
    m_EepromPersist = FALSE;

    /*
     * Clear shadow cache.
     */
    RtlZeroMemory(m_ControlRegs, sizeof(m_ControlRegs));

    /*
     * Detect card via Control Chip register 0 (model ID).
     *
     * 1. Write 0xFF to base+2 to enable control bank
     * 2. Poll SB/RB until ready
     * 3. Write register index 0x00 to base+2
     * 4. Read model/options from base+3
     * 5. Write 0xFE to base+2 to restore OPL3 bank
     */
    NTSTATUS ntStatus = STATUS_SUCCESS;

    WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_ADDR, ALG_BANK_CONTROL);

    if (!WaitForReady())
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("Init: card not responding (busy timeout)"));
        ntStatus = STATUS_DEVICE_DOES_NOT_EXIST;
    }

    if (NT_SUCCESS(ntStatus))
    {
        WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_ADDR, CTRL_REG_CONTROL_ID);
        BYTE idByte = READ_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_DATA);
        WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_ADDR, ALG_BANK_OPL3);

        m_CardModel   = idByte & CTRL_ID_MODEL_MASK;
        m_CardOptions = idByte;
        m_ControlRegs[CTRL_REG_CONTROL_ID] = idByte;

        if (m_CardModel > ALG_MODEL_GOLD2000MC)
        {
            _DbgPrintF(DEBUGLVL_TERSE,
                ("Init: unknown card model 0x%X", (ULONG)m_CardModel));
            ntStatus = STATUS_DEVICE_DOES_NOT_EXIST;
        }
        else
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,
                ("Init: detected Ad Lib Gold model %d", (ULONG)m_CardModel));
            _DbgPrintF(DEBUGLVL_VERBOSE,
                ("Init: options=0x%02X TEL=%s SUR=%s SCSI=%s",
                 (ULONG)m_CardOptions,
                 (m_CardOptions & CTRL_ID_OPT_TEL) ? "no" : "yes",
                 (m_CardOptions & CTRL_ID_OPT_SURROUND) ? "no" : "yes",
                 (m_CardOptions & CTRL_ID_OPT_SCSI) ? "no" : "yes"));
        }
    }

    /*
     * Set up interrupt synchronization.
     */
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = PcNewInterruptSync(
            &m_pInterruptSync,
            NULL,                           /* OuterUnknown              */
            ResourceList,                   /* Gets IRQ from list        */
            0,                              /* Resource index            */
            InterruptSyncModeNormal         /* Run ISRs until SUCCESS    */
        );

        if (NT_SUCCESS(ntStatus) && m_pInterruptSync)
        {
            ntStatus = m_pInterruptSync->RegisterServiceRoutine(
                InterruptServiceRoutine,
                PVOID(this),
                FALSE                       /* Run first                 */
            );

            if (NT_SUCCESS(ntStatus))
            {
                ntStatus = m_pInterruptSync->Connect();
            }

            if (!NT_SUCCESS(ntStatus))
            {
                m_pInterruptSync->Release();
                m_pInterruptSync = NULL;
            }
        }
    }

    /*
     * Initialize Control Chip mixer registers.
     */
    if (NT_SUCCESS(ntStatus))
    {
        ControlRegReset();
    }

    return ntStatus;
}


/*****************************************************************************
 * CAdapterCommon::~CAdapterCommon()
 *****************************************************************************
 * Destructor.
 */
CAdapterCommon::
~CAdapterCommon
(   void
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE, ("[CAdapterCommon::~CAdapterCommon]"));

    if (m_pInterruptSync)
    {
        m_pInterruptSync->Disconnect();
        m_pInterruptSync->Release();
        m_pInterruptSync = NULL;
    }
}


/*****************************************************************************
 * CAdapterCommon::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
STDMETHODIMP
CAdapterCommon::
NonDelegatingQueryInterface
(
    REFIID  Interface,
    PVOID * Object
)
{
    PAGED_CODE();

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PADAPTERCOMMON(this)));
    }
    else
    if (IsEqualGUIDAligned(Interface, IID_IAdapterCommon))
    {
        *Object = PVOID(PADAPTERCOMMON(this));
    }
    else
    if (IsEqualGUIDAligned(Interface, IID_IAdapterPowerManagment))
    {
        *Object = PVOID(PADAPTERPOWERMANAGMENT(this));
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
 * CAdapterCommon::ControlRegReset()
 *****************************************************************************
 * Reset mixer registers to defaults (from registry or hardcoded).
 */
STDMETHODIMP_(void)
CAdapterCommon::
ControlRegReset
(   void
)
{
    PAGED_CODE();

    ASSERT(m_pPortBase);

    NTSTATUS ntStatus = RestoreMixerSettingsFromRegistry();
    if (!NT_SUCCESS(ntStatus))
    {
        for (ULONG i = 0; i < SIZEOF_ARRAY(DefaultMixerSettings); i++)
        {
            ControlRegWrite(DefaultMixerSettings[i].RegisterIndex,
                            DefaultMixerSettings[i].RegisterSetting);
        }
    }

    /* Ensure reserved register is zero */
    ControlRegWrite(CTRL_REG_RESERVED, 0x00);
}


/*****************************************************************************
 * CAdapterCommon::GetCardModel()
 *****************************************************************************
 * Returns the detected card model identifier.
 */
STDMETHODIMP_(BYTE)
CAdapterCommon::
GetCardModel
(   void
)
{
    PAGED_CODE();

    return m_CardModel;
}


/*****************************************************************************
 * Non-pageable code
 *
 * Everything below runs at DISPATCH_LEVEL or DIRQL and must not be paged out.
 */
#pragma code_seg()


/*****************************************************************************
 * CAdapterCommon::GetInterruptSync()
 *****************************************************************************
 * Returns the borrowed interrupt-sync pointer. Non-paged: the MIDI transmit path
 * calls this at DISPATCH_LEVEL, where a fetch from a trimmed pageable page would be
 * an unrecoverable ring-0 page fault.
 */
STDMETHODIMP_(PINTERRUPTSYNC)
CAdapterCommon::
GetInterruptSync
(   void
)
{
    return m_pInterruptSync;
}


/*****************************************************************************
 * SyncClearBackPointer()
 *****************************************************************************
 * Runs at DIRQL under the interrupt lock; clears an ISR back-pointer so the ISR
 * cannot dispatch through a pointer that is being torn down.
 */
typedef struct _CLEAR_PTR_CONTEXT
{
    PVOID * Target;
} CLEAR_PTR_CONTEXT, *PCLEAR_PTR_CONTEXT;

static NTSTATUS
SyncClearBackPointer
(
    IN      PINTERRUPTSYNC  InterruptSync,
    IN      PVOID           Context
)
{
    *(((PCLEAR_PTR_CONTEXT)Context)->Target) = NULL;
    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAdapterCommon::SetWaveMiniport()
 *****************************************************************************
 * Publish or clear the wave miniport back-pointer the ISR dispatches through. A
 * clear is serialized against the ISR through the interrupt lock; a set is a plain
 * aligned store the ISR reads atomically.
 */
STDMETHODIMP_(void)
CAdapterCommon::
SetWaveMiniport
(
    IN      PWAVEMINIPORTADLIBGOLD  Miniport
)
{
    if (Miniport == NULL && m_pInterruptSync)
    {
        CLEAR_PTR_CONTEXT ctx;
        ctx.Target = (PVOID *)&m_pWaveMiniport;
        m_pInterruptSync->CallSynchronizedRoutine(SyncClearBackPointer, PVOID(&ctx));
    }
    else
    {
        m_pWaveMiniport = Miniport;
    }
}


/*****************************************************************************
 * CAdapterCommon::SetMidiMiniport()
 *****************************************************************************
 * Publish or clear the MIDI miniport back-pointer; see SetWaveMiniport.
 */
STDMETHODIMP_(void)
CAdapterCommon::
SetMidiMiniport
(
    IN      PMIDIMINIPORTADLIBGOLD  Miniport
)
{
    if (Miniport == NULL && m_pInterruptSync)
    {
        CLEAR_PTR_CONTEXT ctx;
        ctx.Target = (PVOID *)&m_pMidiMiniport;
        m_pInterruptSync->CallSynchronizedRoutine(SyncClearBackPointer, PVOID(&ctx));
    }
    else
    {
        m_pMidiMiniport = Miniport;
    }
}


/*****************************************************************************
 * CAdapterCommon::WaitForReady()
 *****************************************************************************
 * Poll the SB and RB status bits until both clear.
 * Must be called with the Control Chip bank enabled (0xFF written to base+2).
 * Returns TRUE if ready, FALSE on timeout.
 */
BOOLEAN
CAdapterCommon::
WaitForReady
(   void
)
{
    ULONG   timeout = 1000;
    UCHAR   status;

    do
    {
        status = READ_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_ADDR);
    } while ((status & ALG_STATUS_BUSY_MASK) && --timeout);

    return (timeout > 0);
}


/*****************************************************************************
 * CAdapterCommon::ControlRegWrite()
 *****************************************************************************
 * Write a value to a Control Chip register.
 *
 * Performs the full bank-switch sequence:
 *   1. Enable control bank (write 0xFF)
 *   2. Poll SB/RB until ready
 *   3. Write register index
 *   4. Write data value
 *   5. Apply timing delay (register-dependent)
 *   6. Restore OPL3 bank (write 0xFE)
 *
 * Always updates the shadow cache, even if the hardware write is skipped
 * due to power state.
 *
 * The enable/write/restore sequence runs inside the interrupt-sync lock so the
 * DIRQL ISR can never interleave it and flip the bank mid-sequence (plan/0008,
 * spec/BankAccess.tla). Before the interrupt is connected (Init), the locked body
 * runs directly, since no ISR is armed yet.
 */
STDMETHODIMP_(void)
CAdapterCommon::
ControlRegWrite
(
    IN      BYTE    Register,
    IN      BYTE    Value
)
{
    SYNC_PORT_CONTEXT ctx;
    ctx.That    = this;
    ctx.Address = Register;
    ctx.Value   = Value;

    if (m_pInterruptSync)
    {
        m_pInterruptSync->CallSynchronizedRoutine(
            SynchronizedControlRegWrite, PVOID(&ctx));
    }
    else
    {
        ControlRegWriteLocked(Register, Value);
    }
}


/*****************************************************************************
 * SynchronizedControlRegWrite()
 *****************************************************************************
 * Interrupt-sync routine (DIRQL) that wraps the Control Chip register write, so
 * the shared bank-switch sequence is atomic against the ISR (plan/0008).
 */
NTSTATUS
SynchronizedControlRegWrite
(
    IN      PINTERRUPTSYNC  InterruptSync,
    IN      PVOID           Context
)
{
    PSYNC_PORT_CONTEXT ctx = (PSYNC_PORT_CONTEXT)Context;
    ctx->That->ControlRegWriteLocked((BYTE)ctx->Address, ctx->Value);
    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAdapterCommon::ControlRegWriteLocked()
 *****************************************************************************
 * The bank-switch enable/write/restore sequence. Runs at DIRQL under the
 * interrupt-sync lock (or directly during Init). Always updates the shadow cache.
 */
void
CAdapterCommon::
ControlRegWriteLocked
(
    IN      BYTE    Register,
    IN      BYTE    Value
)
{
    ASSERT(m_pPortBase);

    /* Only hit hardware if in an acceptable power state */
    if (m_PowerState <= PowerDeviceD1)
    {
        /* 1. Enable control bank */
        WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_ADDR, ALG_BANK_CONTROL);

        /* 2. Poll until not busy; report a stuck chip rather than writing anyway */
        if (!WaitForReady())
            _DbgPrintF(DEBUGLVL_TERSE,
                ("ControlRegWrite: chip busy timeout before reg 0x%02X", (ULONG)Register));

        /* 3. Write register index */
        WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_ADDR, Register);

        /* 4. Write data value */
        WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_DATA, Value);

        /* 5. Apply required delay based on register number */
        if (Register >= 0x04 && Register <= 0x08)
        {
            /* Registers 4-8: ~450us — poll SB/RB for completion */
            if (!WaitForReady())
                _DbgPrintF(DEBUGLVL_TERSE,
                    ("ControlRegWrite: chip busy timeout after reg 0x%02X", (ULONG)Register));
        }
        else if (Register >= 0x09 && Register <= 0x16)
        {
            /* Registers 9-16h: 5us delay */
            KeStallExecutionProcessor(ChipWriteDelayUs(CHIP_CONTROL, Register));
        }
        else
        {
            /* Every other control register still honours its tabulated settle rather
             * than none: the SP2 serial port (0x17+) and reg 0 (EEPROM, 2500us) get
             * their delay from the same table, so no control write is left unpaced. */
            KeStallExecutionProcessor(ChipWriteDelayUs(CHIP_CONTROL, Register));
        }

        /* 6. Restore OPL3 bank 1 access */
        WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_ADDR, ALG_BANK_OPL3);
    }

    /* Always update shadow cache */
    if (Register < CTRL_REG_MAX)
    {
        m_ControlRegs[Register] = Value;
    }

    /* Diagnostic: every control-chip write, to see exactly what reaches the mixer.
     * Compiles out of the free build (checked build only). */
    _DbgPrintF(DEBUGLVL_VERBOSE,
        ("CtrlWr r0x%02X = 0x%02X (pwr=%d)", (ULONG)Register, (ULONG)Value, (ULONG)m_PowerState));
}


/*****************************************************************************
 * CAdapterCommon::ControlRegRead()
 *****************************************************************************
 * Read a Control Chip register value from the shadow cache.
 */
STDMETHODIMP_(BYTE)
CAdapterCommon::
ControlRegRead
(
    IN      BYTE    Register
)
{
    if (Register < CTRL_REG_MAX)
    {
        return m_ControlRegs[Register];
    }

    return 0;
}


/*****************************************************************************
 * CAdapterCommon::EnableControlBank()
 *****************************************************************************
 * Switch base+2/3 to Control Chip register access.
 * Must be called within an InterruptSync synchronized routine.
 */
STDMETHODIMP_(void)
CAdapterCommon::
EnableControlBank
(   void
)
{
    ASSERT(m_pPortBase);
    WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_ADDR, ALG_BANK_CONTROL);
}


/*****************************************************************************
 * CAdapterCommon::EnableOPL3Bank1()
 *****************************************************************************
 * Switch base+2/3 to OPL3 array 1 register access.
 * Must be called within an InterruptSync synchronized routine.
 */
STDMETHODIMP_(void)
CAdapterCommon::
EnableOPL3Bank1
(   void
)
{
    ASSERT(m_pPortBase);
    WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_ADDR, ALG_BANK_OPL3);
}


/*****************************************************************************
 * CAdapterCommon::WriteOPL3()
 *****************************************************************************
 * Write to an OPL3 register with bank coordination.
 *
 * Address < 0x100: Bank 0 (ports base+0/1) — no conflict with Control Chip.
 * Address >= 0x100: Bank 1 (ports base+2/3) — ensure OPL3 mode first.
 */
STDMETHODIMP_(void)
CAdapterCommon::
WriteOPL3
(
    IN      ULONG   Address,
    IN      UCHAR   Data
)
{
    ASSERT(m_pPortBase);

    if (m_PowerState > PowerDeviceD1)
        return;

    if (Address < 0x100)
    {
        /* Bank 0 (ports base+0/1): not shared with the Control Chip, no sync needed */
        WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM0_ADDR, (UCHAR)Address);
        KeStallExecutionProcessor(ChipWriteDelayUs(CHIP_OPL3, 0));
        WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM0_DATA, Data);
        KeStallExecutionProcessor(ChipWriteDelayUs(CHIP_OPL3, 0));
    }
    else
    {
        /* Bank 1 (ports base+2/3): shared with the Control Chip. Serialize the write
         * against the ISR through the interrupt-sync lock (plan/0008). */
        SYNC_PORT_CONTEXT ctx;
        ctx.That    = this;
        ctx.Address = Address;
        ctx.Value   = Data;

        if (m_pInterruptSync)
        {
            m_pInterruptSync->CallSynchronizedRoutine(
                SynchronizedWriteOPL3Bank1, PVOID(&ctx));
        }
        else
        {
            WriteOPL3Bank1Locked(Address, Data);
        }
    }
}


/*****************************************************************************
 * SynchronizedWriteOPL3Bank1()
 *****************************************************************************
 * Interrupt-sync routine (DIRQL) wrapping the OPL3 array-1 write, so it is
 * atomic against the ISR on the shared base+2/base+3 ports (plan/0008).
 */
NTSTATUS
SynchronizedWriteOPL3Bank1
(
    IN      PINTERRUPTSYNC  InterruptSync,
    IN      PVOID           Context
)
{
    PSYNC_PORT_CONTEXT ctx = (PSYNC_PORT_CONTEXT)Context;
    ctx->That->WriteOPL3Bank1Locked(ctx->Address, ctx->Value);
    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAdapterCommon::WriteOPL3Bank1Locked()
 *****************************************************************************
 * Write an OPL3 array-1 register on the shared ports. Runs at DIRQL under the
 * interrupt-sync lock (or directly during Init).
 */
void
CAdapterCommon::
WriteOPL3Bank1Locked
(
    IN      ULONG   Address,
    IN      BYTE    Data
)
{
    WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_ADDR, ALG_BANK_OPL3);
    WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_ADDR, (UCHAR)(Address & 0xFF));
    KeStallExecutionProcessor(ChipWriteDelayUs(CHIP_OPL3, 0));
    WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_DATA, Data);
    KeStallExecutionProcessor(ChipWriteDelayUs(CHIP_OPL3, 0));
}


/*****************************************************************************
 * CAdapterCommon::WriteMMA()
 *****************************************************************************
 * Write to a YMZ263 MMA register (Channel 0). The base+4 index / base+5 data pair
 * is one latch, so it runs inside the interrupt-sync lock: the DIRQL FIFO service
 * also drives base+4/base+5, and without this a state-transition write could be
 * preempted between its index and its data write and land in the wrong register
 * (plan/0008). Before the interrupt is connected (Init), the locked body runs directly.
 */
STDMETHODIMP_(void)
CAdapterCommon::
WriteMMA
(
    IN      BYTE    Register,
    IN      BYTE    Value
)
{
    SYNC_PORT_CONTEXT ctx;
    ctx.That    = this;
    ctx.Address = Register;
    ctx.Value   = Value;

    if (m_pInterruptSync)
    {
        m_pInterruptSync->CallSynchronizedRoutine(SynchronizedWriteMMA, PVOID(&ctx));
    }
    else
    {
        WriteMMALocked(Register, Value);
    }
}


/*****************************************************************************
 * SynchronizedWriteMMA()
 *****************************************************************************
 * Interrupt-sync routine (DIRQL) wrapping the MMA index/data write.
 */
NTSTATUS
SynchronizedWriteMMA
(
    IN      PINTERRUPTSYNC  InterruptSync,
    IN      PVOID           Context
)
{
    PSYNC_PORT_CONTEXT ctx = (PSYNC_PORT_CONTEXT)Context;
    ctx->That->WriteMMALocked((BYTE)ctx->Address, ctx->Value);
    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAdapterCommon::WriteMMALocked()
 *****************************************************************************
 * The raw base+4 index / base+5 data write. Runs at DIRQL inside the interrupt-sync
 * (or directly during Init); ISR-context callers (the FIFO service, MIDI transmit)
 * use it directly to avoid re-entering the lock.
 */
STDMETHODIMP_(void)
CAdapterCommon::
WriteMMALocked
(
    IN      BYTE    Register,
    IN      BYTE    Value
)
{
    ASSERT(m_pPortBase);

    if (m_PowerState > PowerDeviceD1)
        return;

    WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_MMA0_ADDR, Register);
    KeStallExecutionProcessor(ChipWriteDelayUs(CHIP_MMA, 0));
    WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_MMA0_DATA, Value);
    KeStallExecutionProcessor(ChipWriteDelayUs(CHIP_MMA, 0));
}


/*****************************************************************************
 * CAdapterCommon::ReadMMA()
 *****************************************************************************
 * Read from a YMZ263 MMA register (Channel 0), serialized against the ISR like
 * WriteMMA. The read byte is returned through the sync context.
 */
STDMETHODIMP_(BYTE)
CAdapterCommon::
ReadMMA
(
    IN      BYTE    Register
)
{
    SYNC_PORT_CONTEXT ctx;
    ctx.That    = this;
    ctx.Address = Register;
    ctx.Value   = 0;

    if (m_pInterruptSync)
    {
        m_pInterruptSync->CallSynchronizedRoutine(SynchronizedReadMMA, PVOID(&ctx));
        return ctx.Value;
    }

    return ReadMMALocked(Register);
}


/*****************************************************************************
 * SynchronizedReadMMA()
 *****************************************************************************
 * Interrupt-sync routine (DIRQL) wrapping the MMA index/data read; the byte comes
 * back in the context.
 */
NTSTATUS
SynchronizedReadMMA
(
    IN      PINTERRUPTSYNC  InterruptSync,
    IN      PVOID           Context
)
{
    PSYNC_PORT_CONTEXT ctx = (PSYNC_PORT_CONTEXT)Context;
    ctx->Value = ctx->That->ReadMMALocked((BYTE)ctx->Address);
    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAdapterCommon::ReadMMALocked()
 *****************************************************************************
 * The raw base+4 index / base+5 data read. Runs at DIRQL inside the interrupt-sync
 * (or directly during Init); ISR-context callers use it directly.
 */
STDMETHODIMP_(BYTE)
CAdapterCommon::
ReadMMALocked
(
    IN      BYTE    Register
)
{
    ASSERT(m_pPortBase);

    if (m_PowerState > PowerDeviceD1)
        return 0;

    WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_MMA0_ADDR, Register);
    KeStallExecutionProcessor(ChipWriteDelayUs(CHIP_MMA, 0));
    return READ_PORT_UCHAR(m_pPortBase + ALG_REG_MMA0_DATA);
}


/*****************************************************************************
 * CAdapterCommon::ReadMMAStatus()
 *****************************************************************************
 * Read the YMZ263 MMA status register.
 *
 * The status is a DIRECT read of the register-select port at base+4 (38CH); it
 * is not reached through the register/data protocol that ReadMMA uses (writing
 * an index to base+4 then reading base+5 returns that register's data, not the
 * status — manual ch07). Non-paged: called at DIRQL from the ISR and the MIDI
 * service path. The flags are level-sensitive, so the read does not clear them.
 */
STDMETHODIMP_(BYTE)
CAdapterCommon::
ReadMMAStatus
(   void
)
{
    ASSERT(m_pPortBase);

    return READ_PORT_UCHAR(m_pPortBase + ALG_REG_MMA0_ADDR);
}


/*****************************************************************************
 * CAdapterCommon::WriteMMA1()
 *****************************************************************************
 * Write a YMZ263 MMA Channel 1 register (base+6 index / base+7 data). Channel 1 is a
 * distinct index/data latch from Channel 0 (base+4/base+5), and the ISR touches only
 * Channel 0, so this write cannot be interrupted mid-latch by the ISR and needs no
 * interrupt-sync serialization (see common.h). Non-paged: reached from the wave stream's
 * stereo start/stop, which can run at DISPATCH_LEVEL. Used only on the stereo path
 * (call/0017).
 */
STDMETHODIMP_(void)
CAdapterCommon::
WriteMMA1
(
    IN      BYTE    Register,
    IN      BYTE    Value
)
{
    ASSERT(m_pPortBase);

    if (m_PowerState > PowerDeviceD1)
        return;

    WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_MMA1_ADDR, Register);
    KeStallExecutionProcessor(ChipWriteDelayUs(CHIP_MMA, 0));
    WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_MMA1_DATA, Value);
    KeStallExecutionProcessor(ChipWriteDelayUs(CHIP_MMA, 0));
}


/*****************************************************************************
 * InterruptServiceRoutine()
 *****************************************************************************
 * ISR for the Ad Lib Gold.
 *
 * Reads the Control Chip status register to determine interrupt source(s).
 * Note: interrupt status bits are ACTIVE LOW (0 = pending).
 */
NTSTATUS
InterruptServiceRoutine
(
    IN      PINTERRUPTSYNC  InterruptSync,
    IN      PVOID           DynamicContext
)
{
    ASSERT(InterruptSync);
    ASSERT(DynamicContext);

    CAdapterCommon *that = (CAdapterCommon *)DynamicContext;
    ASSERT(that->m_pPortBase);

    /* Enable control bank to read status */
    WRITE_PORT_UCHAR(that->m_pPortBase + ALG_REG_FM1_ADDR, ALG_BANK_CONTROL);
    UCHAR status = READ_PORT_UCHAR(that->m_pPortBase + ALG_REG_FM1_ADDR);
    WRITE_PORT_UCHAR(that->m_pPortBase + ALG_REG_FM1_ADDR, ALG_BANK_OPL3);

    /*
     * Claim the interrupt only for the two sources this ISR actually services, the
     * sampling and FM lines. The SCSI and telephone bits in the status byte belong to
     * other drivers; testing them here (the wider ALG_STATUS_IRQ_MASK) would let this
     * ISR return SUCCESS for an interrupt it cannot clear, stalling the real handler.
     * A source is pending when its bit is 0, so both bits set means neither is ours.
     */
    if ((status & (ALG_STATUS_SMP_IRQ | ALG_STATUS_FM_IRQ)) ==
        (ALG_STATUS_SMP_IRQ | ALG_STATUS_FM_IRQ))
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Sampling/MMA interrupt (D1 = 0 means pending) */
    if (!(status & ALG_STATUS_SMP_IRQ))
    {
        /*
         * Read the MMA status once and dispatch by the flag each path owns.
         * The flags are level-sensitive (manual ch07), so the read does not
         * clear them; a path stays pending until its FIFO condition clears.
         */
        UCHAR mmaStatus = that->ReadMMAStatus();

        if (that->m_pWaveMiniport)
        {
            that->m_pWaveMiniport->ServiceWaveISR(mmaStatus);
        }

        if (MmaStatusMidiRxReady(mmaStatus) && that->m_pMidiMiniport)
        {
            that->m_pMidiMiniport->ServiceMidiISR();
        }
    }

    /* FM/OPL3 timer interrupt (D0 = 0 means pending) */
    if (!(status & ALG_STATUS_FM_IRQ))
    {
        /* Read OPL3 status to acknowledge */
        READ_PORT_UCHAR(that->m_pPortBase + ALG_REG_FM0_ADDR);
    }

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * Pageable code — registry persistence and EEPROM
 */
#pragma code_seg("PAGE")

/*****************************************************************************
 * CAdapterCommon::RestoreMixerSettingsFromRegistry()
 *****************************************************************************
 * Restore mixer settings from the driver's registry key.
 * Follows the SB16 DDK sample pattern exactly.
 */
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::
RestoreMixerSettingsFromRegistry
(   void
)
{
    PAGED_CODE();

    PREGISTRYKEY    DriverKey;
    PREGISTRYKEY    SettingsKey;

    _DbgPrintF(DEBUGLVL_VERBOSE, ("[RestoreMixerSettingsFromRegistry]"));

    /* Open the driver registry key */
    NTSTATUS ntStatus = PcNewRegistryKey(
        &DriverKey,
        NULL,                       /* OuterUnknown              */
        DriverRegistryKey,          /* Registry key type         */
        KEY_ALL_ACCESS,
        m_pDeviceObject,
        NULL,                       /* Subdevice                 */
        NULL,                       /* ObjectAttributes          */
        0,                          /* Create options            */
        NULL                        /* Disposition               */
    );

    if (NT_SUCCESS(ntStatus))
    {
        UNICODE_STRING  KeyName;
        ULONG           Disposition;

        RtlInitUnicodeString(&KeyName, L"Settings");

        ntStatus = DriverKey->NewSubKey(
            &SettingsKey,
            NULL,
            KEY_ALL_ACCESS,
            &KeyName,
            REG_OPTION_NON_VOLATILE,
            &Disposition
        );

        if (NT_SUCCESS(ntStatus))
        {
            ULONG ResultLength;

            if (Disposition == REG_CREATED_NEW_KEY)
            {
                /* New key — write defaults */
                for (ULONG i = 0; i < SIZEOF_ARRAY(DefaultMixerSettings); i++)
                {
                    ControlRegWrite(DefaultMixerSettings[i].RegisterIndex,
                                    DefaultMixerSettings[i].RegisterSetting);
                }
            }
            else
            {
                /* Existing key — read saved values */
                PVOID KeyInfo = ExAllocatePool(
                    PagedPool,
                    sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD)
                );

                if (NULL != KeyInfo)
                {
                    for (ULONG i = 0; i < SIZEOF_ARRAY(DefaultMixerSettings); i++)
                    {
                        RtlInitUnicodeString(&KeyName,
                                             DefaultMixerSettings[i].KeyName);

                        ntStatus = SettingsKey->QueryValueKey(
                            &KeyName,
                            KeyValuePartialInformation,
                            KeyInfo,
                            sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD),
                            &ResultLength
                        );

                        if (NT_SUCCESS(ntStatus))
                        {
                            PKEY_VALUE_PARTIAL_INFORMATION PartialInfo =
                                PKEY_VALUE_PARTIAL_INFORMATION(KeyInfo);

                            if (PartialInfo->DataLength == sizeof(DWORD))
                            {
                                BYTE reg = DefaultMixerSettings[i].RegisterIndex;
                                BYTE val = BYTE(*(PDWORD(PartialInfo->Data)));
                                /* The master output volume (reg 04/05) is a dB code whose
                                 * audible floor is 0xDC (-64dB); below that is -80dB OFF.
                                 * A value saved from the old buggy default (0xD8) would
                                 * silence the card, so recover it to 0dB (call/0025). */
                                if ((reg == CTRL_REG_MASTER_VOL_L ||
                                     reg == CTRL_REG_MASTER_VOL_R) && val < 0xDC)
                                {
                                    val = 0xFC;
                                }
                                /* Reg 08h's effect/channel fields were composed on a
                                 * wrong bit layout before call/0029, and no UI writes
                                 * them, so a saved value's low bits are stale. Rebuild
                                 * them as linear stereo on both channels and preserve
                                 * only the user's mute bit. */
                                if (reg == CTRL_REG_OUTPUT_MODE)
                                {
                                    val = (BYTE)(CTRL_MODE_FORCED_BITS |
                                                 (val & CTRL_MODE_MUTE) |
                                                 CTRL_MODE_STEREO_LINEAR |
                                                 CTRL_MODE_SOURCE_BOTH);
                                }
                                ControlRegWrite(reg, val);
                            }
                        }
                        else
                        {
                            /* Key missing — use default */
                            ControlRegWrite(
                                DefaultMixerSettings[i].RegisterIndex,
                                DefaultMixerSettings[i].RegisterSetting
                            );
                        }
                    }

                    /* The restore is complete: every setting was either restored from the
                     * registry or defaulted. A per-key miss is not a failure, so do not let
                     * the loop's last QueryValueKey status leak out -- that made a good
                     * restore look failed, and ControlRegReset then clobbered it with
                     * defaults. */
                    ntStatus = STATUS_SUCCESS;

                    ExFreePool(KeyInfo);
                }
                else
                {
                    /* Allocation failed — use defaults */
                    for (ULONG i = 0; i < SIZEOF_ARRAY(DefaultMixerSettings); i++)
                    {
                        ControlRegWrite(DefaultMixerSettings[i].RegisterIndex,
                                        DefaultMixerSettings[i].RegisterSetting);
                    }
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                }
            }

            /* Read the registry-gated card-EEPROM save flag; absent or unreadable means
             * off, so a fresh install never writes the EEPROM (call/0019). A local status
             * keeps this off the restore's own success path. */
            {
                PVOID FlagInfo = ExAllocatePool(
                    PagedPool,
                    sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD)
                );

                if (NULL != FlagInfo)
                {
                    UNICODE_STRING FlagName;
                    ULONG          FlagLength;

                    RtlInitUnicodeString(&FlagName, L"SaveToEEPROM");

                    NTSTATUS flagStatus = SettingsKey->QueryValueKey(
                        &FlagName,
                        KeyValuePartialInformation,
                        FlagInfo,
                        sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD),
                        &FlagLength
                    );

                    if (NT_SUCCESS(flagStatus))
                    {
                        PKEY_VALUE_PARTIAL_INFORMATION FlagPartial =
                            PKEY_VALUE_PARTIAL_INFORMATION(FlagInfo);

                        if (FlagPartial->DataLength == sizeof(DWORD))
                        {
                            m_EepromPersist =
                                (*(PDWORD(FlagPartial->Data)) != 0);
                        }
                    }

                    ExFreePool(FlagInfo);
                }
            }

            SettingsKey->Release();
        }

        DriverKey->Release();
    }

    return ntStatus;
}


/*****************************************************************************
 * CAdapterCommon::SaveMixerSettingsToRegistry()
 *****************************************************************************
 * Save current mixer settings to the driver's registry key.
 */
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::
SaveMixerSettingsToRegistry
(   void
)
{
    PAGED_CODE();

    PREGISTRYKEY    DriverKey;
    PREGISTRYKEY    SettingsKey;

    _DbgPrintF(DEBUGLVL_VERBOSE, ("[SaveMixerSettingsToRegistry]"));

    NTSTATUS ntStatus = PcNewRegistryKey(
        &DriverKey,
        NULL,
        DriverRegistryKey,
        KEY_ALL_ACCESS,
        m_pDeviceObject,
        NULL,
        NULL,
        0,
        NULL
    );

    if (NT_SUCCESS(ntStatus))
    {
        UNICODE_STRING KeyName;

        RtlInitUnicodeString(&KeyName, L"Settings");

        ntStatus = DriverKey->NewSubKey(
            &SettingsKey,
            NULL,
            KEY_ALL_ACCESS,
            &KeyName,
            REG_OPTION_NON_VOLATILE,
            NULL
        );

        if (NT_SUCCESS(ntStatus))
        {
            for (ULONG i = 0; i < SIZEOF_ARRAY(DefaultMixerSettings); i++)
            {
                RtlInitUnicodeString(&KeyName,
                                     DefaultMixerSettings[i].KeyName);

                DWORD KeyValue = DWORD(
                    m_ControlRegs[DefaultMixerSettings[i].RegisterIndex]);

                ntStatus = SettingsKey->SetValueKey(
                    &KeyName,
                    REG_DWORD,
                    PVOID(&KeyValue),
                    sizeof(DWORD)
                );

                if (!NT_SUCCESS(ntStatus))
                {
                    break;
                }
            }

            /* If the operator turned on the registry-gated flag (off by default, call/0019),
             * also persist to the card's EEPROM. This is the only safe trigger: PASSIVE_LEVEL
             * with the hardware mapped and powered, so the pageable 2.5ms save cannot fault. */
            if (NT_SUCCESS(ntStatus) && m_EepromPersist)
            {
                SaveToEEPROM();
            }

            SettingsKey->Release();
        }

        DriverKey->Release();
    }

    return ntStatus;
}


/*****************************************************************************
 * CAdapterCommon::SaveToEEPROM()
 *****************************************************************************
 * Save all Control Chip register values to the card's EEPROM.
 */
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::
SaveToEEPROM
(   void
)
{
    PAGED_CODE();

    if (m_PowerState > PowerDeviceD1)
        return STATUS_DEVICE_POWERED_OFF;

    /* Enable control bank */
    WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_ADDR, ALG_BANK_CONTROL);
    WaitForReady();

    /* Select register 0 (Control/ID) */
    WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_ADDR, CTRL_REG_CONTROL_ID);

    /* Write ST bit (D1) to trigger EEPROM save */
    WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_DATA, CTRL_ID_SAVE);

    /* The EEPROM save takes ~2.5ms, and RB covers register access rather than the EEPROM
     * cycle, so wait the full tabulated cycle (chiptiming.h reg 0 = 2500us) instead of a
     * bounded ready poll that would return before the save completes (as RestoreFromEEPROM
     * does for the same reason). */
    KeStallExecutionProcessor(ChipWriteDelayUs(CHIP_CONTROL, CTRL_REG_CONTROL_ID));

    /* Restore OPL3 bank */
    WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_ADDR, ALG_BANK_OPL3);

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAdapterCommon::RestoreFromEEPROM()
 *****************************************************************************
 * Restore all Control Chip register values from the card's EEPROM.
 * Takes ~2.5ms with no status bit to poll — must use fixed delay.
 */
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::
RestoreFromEEPROM
(   void
)
{
    PAGED_CODE();

    if (m_PowerState > PowerDeviceD1)
        return STATUS_DEVICE_POWERED_OFF;

    /* Enable control bank */
    WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_ADDR, ALG_BANK_CONTROL);
    WaitForReady();

    /* Select register 0 (Control/ID) */
    WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_ADDR, CTRL_REG_CONTROL_ID);

    /* Write RT bit (D0) to trigger EEPROM restore */
    WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_DATA, CTRL_ID_RESTORE);

    /* No status bit — must wait 2.5ms for completion */
    KeStallExecutionProcessor(2500);

    /* Re-read all registers into shadow cache */
    WaitForReady();
    {
        BYTE reg;
        for (reg = 0; reg < CTRL_REG_MAX; reg++)
        {
            WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_ADDR, reg);
            m_ControlRegs[reg] = READ_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_DATA);
        }
    }

    /* Restore OPL3 bank */
    WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_ADDR, ALG_BANK_OPL3);

    /* Update model fields from the refreshed cache */
    m_CardModel   = m_ControlRegs[CTRL_REG_CONTROL_ID] & CTRL_ID_MODEL_MASK;
    m_CardOptions = m_ControlRegs[CTRL_REG_CONTROL_ID];

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * Non-pageable code — power management
 *
 * PowerChangeState may be called at DISPATCH_LEVEL.
 */
#pragma code_seg()

/*****************************************************************************
 * CAdapterCommon::PowerChangeState()
 *****************************************************************************
 * Change power state for the device.
 */
STDMETHODIMP_(void)
CAdapterCommon::
PowerChangeState
(
    IN      POWER_STATE     NewState
)
{
    _DbgPrintF(DEBUGLVL_VERBOSE, ("[CAdapterCommon::PowerChangeState]"));

    if (NewState.DeviceState != m_PowerState)
    {
        switch (NewState.DeviceState)
        {
        case PowerDeviceD0:
            /*
             * Entering full power.  Restore mixer registers from the
             * shadow cache to hardware.  Must set m_PowerState first
             * so ControlRegWrite will hit the hardware.
             */
            m_PowerState = NewState.DeviceState;
            {
                BYTE i;
                for (i = CTRL_MIXER_FIRST; i <= CTRL_MIXER_LAST; i++)
                {
                    ControlRegWrite(i, m_ControlRegs[i]);
                }
            }
            _DbgPrintF(DEBUGLVL_VERBOSE, ("  Entering D0 (full power)"));
            break;

        case PowerDeviceD1:
        case PowerDeviceD2:
        case PowerDeviceD3:
            m_PowerState = NewState.DeviceState;
            _DbgPrintF(DEBUGLVL_VERBOSE, ("  Entering D%d",
                ULONG(m_PowerState) - ULONG(PowerDeviceD0)));
            break;

        default:
            _DbgPrintF(DEBUGLVL_VERBOSE, ("  Unknown Device Power State"));
            break;
        }
    }
}

/*****************************************************************************
 * CAdapterCommon::QueryPowerChangeState()
 *****************************************************************************
 * Query to see if the device can change to this power state.
 */
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::
QueryPowerChangeState
(
    IN      POWER_STATE     NewStateQuery
)
{
    _DbgPrintF(DEBUGLVL_VERBOSE, ("[CAdapterCommon::QueryPowerChangeState]"));

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CAdapterCommon::QueryDeviceCapabilities()
 *****************************************************************************
 * Called at startup to get the caps for the device.
 */
STDMETHODIMP_(NTSTATUS)
CAdapterCommon::
QueryDeviceCapabilities
(
    IN      PDEVICE_CAPABILITIES    PowerDeviceCaps
)
{
    _DbgPrintF(DEBUGLVL_VERBOSE, ("[CAdapterCommon::QueryDeviceCapabilities]"));

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAdapterCommon::WriteSurroundReg()
 *****************************************************************************
 * Program one SP2 (YM7128) register. Encode the write into the bit-serial command
 * stream (sp2.h, unit-tested against the SDK SURROUND sample) and clock it out over
 * Control Chip register 0x18 (call/0012). The caller serialises with the ISR.
 */
STDMETHODIMP_(void)
CAdapterCommon::
WriteSurroundReg
(
    IN      BYTE    Register,
    IN      BYTE    Value
)
{
    unsigned char seq[SP2_BYTES_PER_REG];
    int n = Sp2EncodeReg(Register, Value, seq);
    int i;

    for (i = 0; i < n; i++)
    {
        ControlRegWrite(CTRL_REG_SURROUND, (BYTE)seq[i]);
    }
}
