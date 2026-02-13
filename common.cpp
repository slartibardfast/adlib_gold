/*****************************************************************************
 * common.cpp - Common code used by all the Ad Lib Gold miniports.
 *****************************************************************************
 *
 * Implementation of the adapter common object.  Handles Control Chip
 * register access with bank switching, interrupt dispatch, mixer shadow
 * cache with registry persistence, and power management.
 */

#include "common.h"

#define STR_MODULENAME "AdLibGold: "


/*****************************************************************************
 * CAdapterCommon
 *****************************************************************************
 * Adapter common object.
 */
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
    PWAVEMINIPORTADLIBGOLD  m_pWaveMiniport;
    PMIDIMINIPORTADLIBGOLD  m_pMidiMiniport;

    BOOLEAN WaitForReady(void);

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
    STDMETHODIMP_(void) SetWaveMiniport(IN PWAVEMINIPORTADLIBGOLD Miniport)
    {
        m_pWaveMiniport = Miniport;
    }
    STDMETHODIMP_(void) SetMidiMiniport(IN PMIDIMINIPORTADLIBGOLD Miniport)
    {
        m_pMidiMiniport = Miniport;
    }
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
    { L"LeftMasterVol",  CTRL_REG_MASTER_VOL_L,  0xD8 },  /* ~-20dB, D7-D6 set */
    { L"RightMasterVol", CTRL_REG_MASTER_VOL_R,  0xD8 },
    { L"Bass",           CTRL_REG_BASS,           0xF6 },  /* 0dB flat, D7-D4 set */
    { L"Treble",         CTRL_REG_TREBLE,         0xF6 },  /* 0dB flat, D7-D4 set */
    { L"OutputMode",     CTRL_REG_OUTPUT_MODE,    0xC4 },  /* Linear stereo, both ch, unmuted */
    { L"LeftFMVol",      CTRL_REG_FM_VOL_L,       0xC0 },  /* Mid-range (192 of 128-255) */
    { L"RightFMVol",     CTRL_REG_FM_VOL_R,       0xC0 },
    { L"LeftSampVol",    CTRL_REG_SAMP_VOL_L,     0xC0 },
    { L"RightSampVol",   CTRL_REG_SAMP_VOL_R,     0xC0 },
    { L"LeftAuxVol",     CTRL_REG_AUX_VOL_L,      0xC0 },
    { L"RightAuxVol",    CTRL_REG_AUX_VOL_R,      0xC0 },
    { L"MicVol",         CTRL_REG_MIC_VOL,         0x80 },  /* Silent */
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
 * CAdapterCommon::GetInterruptSync()
 *****************************************************************************
 * Returns a pointer to the interrupt synchronization object.
 */
STDMETHODIMP_(PINTERRUPTSYNC)
CAdapterCommon::
GetInterruptSync
(   void
)
{
    PAGED_CODE();

    return m_pInterruptSync;
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
    return m_CardModel;
}


/*****************************************************************************
 * Non-pageable code
 *
 * Everything below runs at DISPATCH_LEVEL or DIRQL and must not be paged out.
 */
#pragma code_seg()


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
 * CALLER RESPONSIBILITY: After the interrupt is connected, this must be
 * called within InterruptSync->CallSynchronizedRoutine() to prevent
 * races with the ISR.  During Init (before Connect), no sync is needed.
 */
STDMETHODIMP_(void)
CAdapterCommon::
ControlRegWrite
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

        /* 2. Poll until not busy */
        WaitForReady();

        /* 3. Write register index */
        WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_ADDR, Register);

        /* 4. Write data value */
        WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_DATA, Value);

        /* 5. Apply required delay based on register number */
        if (Register >= 0x04 && Register <= 0x08)
        {
            /* Registers 4-8: ~450us — poll SB/RB for completion */
            WaitForReady();
        }
        else if (Register >= 0x09 && Register <= 0x16)
        {
            /* Registers 9-16h: 5us delay */
            KeStallExecutionProcessor(5);
        }

        /* 6. Restore OPL3 bank 1 access */
        WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_ADDR, ALG_BANK_OPL3);
    }

    /* Always update shadow cache */
    if (Register < CTRL_REG_MAX)
    {
        m_ControlRegs[Register] = Value;
    }
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
        /* Bank 0: direct access, no conflict */
        WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM0_ADDR, (UCHAR)Address);
        KeStallExecutionProcessor(23);
        WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM0_DATA, Data);
        KeStallExecutionProcessor(23);
    }
    else
    {
        /* Bank 1: ensure OPL3 mode, then write */
        WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_ADDR, ALG_BANK_OPL3);
        WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_ADDR, (UCHAR)(Address & 0xFF));
        KeStallExecutionProcessor(23);
        WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_FM1_DATA, Data);
        KeStallExecutionProcessor(23);
    }
}


/*****************************************************************************
 * CAdapterCommon::WriteMMA()
 *****************************************************************************
 * Write to a YMZ263 MMA register (Channel 0).
 */
STDMETHODIMP_(void)
CAdapterCommon::
WriteMMA
(
    IN      BYTE    Register,
    IN      BYTE    Value
)
{
    ASSERT(m_pPortBase);

    if (m_PowerState > PowerDeviceD1)
        return;

    WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_MMA0_ADDR, Register);
    KeStallExecutionProcessor(1);
    WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_MMA0_DATA, Value);
    KeStallExecutionProcessor(1);
}


/*****************************************************************************
 * CAdapterCommon::ReadMMA()
 *****************************************************************************
 * Read from a YMZ263 MMA register (Channel 0).
 */
STDMETHODIMP_(BYTE)
CAdapterCommon::
ReadMMA
(
    IN      BYTE    Register
)
{
    ASSERT(m_pPortBase);

    if (m_PowerState > PowerDeviceD1)
        return 0;

    WRITE_PORT_UCHAR(m_pPortBase + ALG_REG_MMA0_ADDR, Register);
    KeStallExecutionProcessor(1);
    return READ_PORT_UCHAR(m_pPortBase + ALG_REG_MMA0_DATA);
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
     * If all IRQ source bits are 1 (inactive), this is not our interrupt.
     */
    if ((status & ALG_STATUS_IRQ_MASK) == ALG_STATUS_IRQ_MASK)
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Sampling/MMA interrupt (D1 = 0 means pending) */
    if (!(status & ALG_STATUS_SMP_IRQ))
    {
        /*
         * Read MMA status once.  Status bits auto-clear on read,
         * so a single read must serve both wave (PRQ) and MIDI (RRQ).
         */
        UCHAR mmaStatus = READ_PORT_UCHAR(that->m_pPortBase + ALG_REG_MMA0_ADDR);

        if (that->m_pWaveMiniport)
        {
            that->m_pWaveMiniport->ServiceWaveISR();
        }

        if ((mmaStatus & MMA_STATUS_RRQ) && that->m_pMidiMiniport)
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
                                ControlRegWrite(
                                    DefaultMixerSettings[i].RegisterIndex,
                                    BYTE(*(PDWORD(PartialInfo->Data)))
                                );
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

    /* Wait for RB to clear (hardware auto-clears ST when done) */
    WaitForReady();

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
