/*****************************************************************************
 * algtopo.cpp - Ad Lib Gold topology miniport implementation
 *****************************************************************************
 *
 * Exposes the Ad Lib Gold Control Chip mixer as a KS topology filter.
 * Property handlers translate between KS volume/mute/tone properties
 * and Control Chip register reads/writes via the adapter common object.
 *
 * Phase 2: All property handlers functional — volume (Level), mute (OnOff),
 * bass/treble (Tone) with dB-scaled get/set/basicsupport, CPU resources.
 */

#include "algtopo.h"

#define STR_MODULENAME "AdLibGoldTopo: "

#define CHAN_LEFT    0
#define CHAN_RIGHT   1
#define CHAN_MASTER  (-1)


/*****************************************************************************
 * Topology tables
 *
 * Included inline here (SB16 sample uses a separate tables.h).
 */

/*****************************************************************************
 * PinDataRangesBridge / PinDataRangePointersBridge
 *****************************************************************************
 * Structures indicating range of valid format values for bridge pins.
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
    &PinDataRangesBridge[0]
};


/*****************************************************************************
 * MiniportPins
 *****************************************************************************
 *
 * Pin 0: Wave render input  (from wave miniport)
 * Pin 1: FM synth input     (from FM miniport)
 * Pin 2: Aux line input     (external)
 * Pin 3: Mic input          (external)
 * Pin 4: Line output        (to speakers)
 */
static
PCPIN_DESCRIPTOR
MiniportPins[] =
{
    /* PIN_WAVEOUT_SOURCE */
    {
        0,0,0,
        NULL,
        {
            0, NULL,
            0, NULL,
            SIZEOF_ARRAY(PinDataRangePointersBridge),
            PinDataRangePointersBridge,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_NONE,
            &KSNODETYPE_LEGACY_AUDIO_CONNECTOR,
            NULL,
            0
        }
    },
    /* PIN_FMSYNTH_SOURCE */
    {
        0,0,0,
        NULL,
        {
            0, NULL,
            0, NULL,
            SIZEOF_ARRAY(PinDataRangePointersBridge),
            PinDataRangePointersBridge,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_NONE,
            &KSNODETYPE_SYNTHESIZER,
            &KSAUDFNAME_MIDI,
            0
        }
    },
    /* PIN_AUX_SOURCE */
    {
        0,0,0,
        NULL,
        {
            0, NULL,
            0, NULL,
            SIZEOF_ARRAY(PinDataRangePointersBridge),
            PinDataRangePointersBridge,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_NONE,
            &KSNODETYPE_LINE_CONNECTOR,
            &KSAUDFNAME_LINE_IN,
            0
        }
    },
    /* PIN_MIC_SOURCE */
    {
        0,0,0,
        NULL,
        {
            0, NULL,
            0, NULL,
            SIZEOF_ARRAY(PinDataRangePointersBridge),
            PinDataRangePointersBridge,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_NONE,
            &KSNODETYPE_MICROPHONE,
            NULL,
            0
        }
    },
    /* PIN_LINEOUT_DEST */
    {
        0,0,0,
        NULL,
        {
            0, NULL,
            0, NULL,
            SIZEOF_ARRAY(PinDataRangePointersBridge),
            PinDataRangePointersBridge,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_NONE,
            &KSNODETYPE_SPEAKER,
            &KSAUDFNAME_VOLUME_CONTROL,
            0
        }
    }
};


/*****************************************************************************
 * Property item tables for automation
 */

/* CPU resources property — common to all nodes */
static NTSTATUS PropertyHandler_CpuResources(PPCPROPERTY_REQUEST);

static
PCPROPERTY_ITEM PropertiesCpuResources[] =
{
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_CPU_RESOURCES,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_CpuResources
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationCpuResources, PropertiesCpuResources);


/* Volume property (KSPROPERTY_AUDIO_VOLUMELEVEL) */
static NTSTATUS PropertyHandler_Level(PPCPROPERTY_REQUEST);

static
PCPROPERTY_ITEM PropertiesVolume[] =
{
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_VOLUMELEVEL,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Level
    },
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_CPU_RESOURCES,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_CpuResources
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationVolume, PropertiesVolume);


/* Mute property (KSPROPERTY_AUDIO_MUTE) */
static NTSTATUS PropertyHandler_OnOff(PPCPROPERTY_REQUEST);

static
PCPROPERTY_ITEM PropertiesMute[] =
{
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_MUTE,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_OnOff
    },
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_CPU_RESOURCES,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_CpuResources
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMute, PropertiesMute);


/* Tone property (bass/treble) */
static NTSTATUS PropertyHandler_Tone(PPCPROPERTY_REQUEST);

static
PCPROPERTY_ITEM PropertiesTone[] =
{
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_BASS,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Tone
    },
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_TREBLE,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Tone
    },
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_CPU_RESOURCES,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_CpuResources
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationTone, PropertiesTone);


/*****************************************************************************
 * MiniportNodes
 *****************************************************************************
 *
 * Topology:
 *   Pin0 (wave)  -> [SampVol] -+
 *   Pin1 (FM)    -> [FMVol]   -+-> [MasterVol] -> [Bass] -> [Treble]
 *   Pin2 (aux)   -> [AuxVol]  -+       -> [Mute] -> Pin4 (lineout)
 *   Pin3 (mic)   -> [MicVol]  -+
 */
static
PCNODE_DESCRIPTOR
MiniportNodes[] =
{
    /* NODE_SAMP_VOLUME */
    {
        0,
        &AutomationVolume,
        &KSNODETYPE_VOLUME,
        &KSAUDFNAME_WAVE_VOLUME
    },
    /* NODE_FM_VOLUME */
    {
        0,
        &AutomationVolume,
        &KSNODETYPE_VOLUME,
        &KSAUDFNAME_MIDI_VOLUME
    },
    /* NODE_AUX_VOLUME */
    {
        0,
        &AutomationVolume,
        &KSNODETYPE_VOLUME,
        &KSAUDFNAME_LINE_IN_VOLUME
    },
    /* NODE_MIC_VOLUME */
    {
        0,
        &AutomationVolume,
        &KSNODETYPE_VOLUME,
        &KSAUDFNAME_MIC_VOLUME
    },
    /* NODE_MASTER_VOLUME */
    {
        0,
        &AutomationVolume,
        &KSNODETYPE_VOLUME,
        &KSAUDFNAME_MASTER_VOLUME
    },
    /* NODE_BASS */
    {
        0,
        &AutomationTone,
        &KSNODETYPE_TONE,
        &KSAUDFNAME_BASS
    },
    /* NODE_TREBLE */
    {
        0,
        &AutomationTone,
        &KSNODETYPE_TONE,
        &KSAUDFNAME_TREBLE
    },
    /* NODE_MUTE */
    {
        0,
        &AutomationMute,
        &KSNODETYPE_MUTE,
        NULL
    }
};


/*****************************************************************************
 * MiniportConnections
 *****************************************************************************
 * Wiring between pins and nodes.
 */
static
PCCONNECTION_DESCRIPTOR
MiniportConnections[] =
{
    /* From pin                       To node / pin                     */
    /* { FromNode,   FromPin,          ToNode,        ToPin }           */

    /* Source pins -> source volume nodes */
    { PCFILTER_NODE,  PIN_WAVEOUT_SOURCE,  NODE_SAMP_VOLUME,    1 },
    { PCFILTER_NODE,  PIN_FMSYNTH_SOURCE,  NODE_FM_VOLUME,      1 },
    { PCFILTER_NODE,  PIN_AUX_SOURCE,      NODE_AUX_VOLUME,     1 },
    { PCFILTER_NODE,  PIN_MIC_SOURCE,      NODE_MIC_VOLUME,     1 },

    /* Source volume nodes -> master volume */
    { NODE_SAMP_VOLUME,  0,  NODE_MASTER_VOLUME,  1 },
    { NODE_FM_VOLUME,    0,  NODE_MASTER_VOLUME,  1 },
    { NODE_AUX_VOLUME,   0,  NODE_MASTER_VOLUME,  1 },
    { NODE_MIC_VOLUME,   0,  NODE_MASTER_VOLUME,  1 },

    /* Master volume -> bass -> treble -> mute -> lineout */
    { NODE_MASTER_VOLUME, 0,  NODE_BASS,     1 },
    { NODE_BASS,          0,  NODE_TREBLE,   1 },
    { NODE_TREBLE,        0,  NODE_MUTE,     1 },
    { NODE_MUTE,          0,  PCFILTER_NODE, PIN_LINEOUT_DEST }
};


/*****************************************************************************
 * MiniportFilterDescriptor
 *****************************************************************************
 * Complete topology filter descriptor.
 */
static
PCFILTER_DESCRIPTOR MiniportFilterDescriptor =
{
    0,                                      /* Version                */
    NULL,                                   /* AutomationTable        */
    sizeof(PCPIN_DESCRIPTOR),               /* PinSize                */
    SIZEOF_ARRAY(MiniportPins),             /* PinCount               */
    MiniportPins,                           /* Pins                   */
    sizeof(PCNODE_DESCRIPTOR),              /* NodeSize               */
    SIZEOF_ARRAY(MiniportNodes),            /* NodeCount              */
    MiniportNodes,                          /* Nodes                  */
    SIZEOF_ARRAY(MiniportConnections),      /* ConnectionCount        */
    MiniportConnections,                    /* Connections            */
    0,                                      /* CategoryCount          */
    NULL                                    /* Categories             */
};


/*****************************************************************************
 * Implementation
 */
#pragma code_seg("PAGE")

/*****************************************************************************
 * CreateMiniportTopologyAdLibGold()
 *****************************************************************************
 * Factory for the topology miniport.
 */
NTSTATUS
CreateMiniportTopologyAdLibGold
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
)
{
    PAGED_CODE();

    ASSERT(Unknown);

    STD_CREATE_BODY(CMiniportTopologyAdLibGold,
                    Unknown,
                    UnknownOuter,
                    PoolType);
}


/*****************************************************************************
 * CMiniportTopologyAdLibGold::NonDelegatingQueryInterface()
 */
STDMETHODIMP
CMiniportTopologyAdLibGold::
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
        *Object = PVOID(PUNKNOWN(this));
    }
    else
    if (IsEqualGUIDAligned(Interface, IID_IMiniport))
    {
        *Object = PVOID(PMINIPORT(this));
    }
    else
    if (IsEqualGUIDAligned(Interface, IID_IMiniportTopology))
    {
        *Object = PVOID(PMINIPORTTOPOLOGY(this));
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
 * CMiniportTopologyAdLibGold::~CMiniportTopologyAdLibGold()
 */
CMiniportTopologyAdLibGold::
~CMiniportTopologyAdLibGold
(   void
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE,
        ("[CMiniportTopologyAdLibGold::~CMiniportTopologyAdLibGold]"));

    if (AdapterCommon)
    {
        AdapterCommon->SaveMixerSettingsToRegistry();
        AdapterCommon->Release();
    }
}


/*****************************************************************************
 * CMiniportTopologyAdLibGold::Init()
 *****************************************************************************
 * Initialize the topology miniport.
 */
STDMETHODIMP
CMiniportTopologyAdLibGold::
Init
(
    IN      PUNKNOWN        UnknownAdapter,
    IN      PRESOURCELIST   ResourceList,
    IN      PPORTTOPOLOGY   Port
)
{
    PAGED_CODE();

    ASSERT(UnknownAdapter);
    ASSERT(ResourceList);
    ASSERT(Port);

    _DbgPrintF(DEBUGLVL_VERBOSE, ("[CMiniportTopologyAdLibGold::Init]"));

    NTSTATUS ntStatus =
        UnknownAdapter->QueryInterface(IID_IAdapterCommon,
                                       (PVOID *)&AdapterCommon);

    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = ProcessResources(ResourceList);

        if (NT_SUCCESS(ntStatus))
        {
            AdapterCommon->ControlRegReset();
        }
    }

    if (!NT_SUCCESS(ntStatus))
    {
        if (AdapterCommon)
        {
            AdapterCommon->Release();
            AdapterCommon = NULL;
        }
    }

    return ntStatus;
}


/*****************************************************************************
 * CMiniportTopologyAdLibGold::ProcessResources()
 *****************************************************************************
 * Validate the resource list.
 */
NTSTATUS
CMiniportTopologyAdLibGold::
ProcessResources
(
    IN  PRESOURCELIST   ResourceList
)
{
    PAGED_CODE();

    ASSERT(ResourceList);

    _DbgPrintF(DEBUGLVL_VERBOSE,
        ("[CMiniportTopologyAdLibGold::ProcessResources]"));

    /* Topology needs exactly 1 I/O port range, no IRQ, no DMA */
    if ((ResourceList->NumberOfPorts() != 1) ||
        (ResourceList->NumberOfInterrupts() != 0) ||
        (ResourceList->NumberOfDmas() != 0))
    {
        _DbgPrintF(DEBUGLVL_TERSE,
            ("ProcessResources: unexpected resource counts"));
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CMiniportTopologyAdLibGold::GetDescription()
 *****************************************************************************
 * Return the topology filter descriptor.
 */
STDMETHODIMP
CMiniportTopologyAdLibGold::
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
 * Property Handlers
 *****************************************************************************/

/*
 * Mapping table: node ID -> Control Chip register pair (left, right).
 * For mono nodes, RegRight == 0 (unused).
 */
typedef struct
{
    BYTE    RegLeft;
    BYTE    RegRight;
    BYTE    MinVal;     /* Minimum valid register value            */
    BYTE    MaxVal;     /* Maximum valid register value            */
} NODE_REG_MAP;

static
NODE_REG_MAP NodeRegMap[] =
{
    /* NODE_SAMP_VOLUME */ { CTRL_REG_SAMP_VOL_L,   CTRL_REG_SAMP_VOL_R,   0x80, 0xFF },
    /* NODE_FM_VOLUME   */ { CTRL_REG_FM_VOL_L,     CTRL_REG_FM_VOL_R,     0x80, 0xFF },
    /* NODE_AUX_VOLUME  */ { CTRL_REG_AUX_VOL_L,    CTRL_REG_AUX_VOL_R,    0x80, 0xFF },
    /* NODE_MIC_VOLUME  */ { CTRL_REG_MIC_VOL,       0,                     0x80, 0xFF },
    /* NODE_MASTER_VOLUME*/ { CTRL_REG_MASTER_VOL_L, CTRL_REG_MASTER_VOL_R, 0xC0, 0xFF },
};


/*****************************************************************************
 * PropertyHandler_Level()
 *****************************************************************************
 * Volume level get/set for source and master volume nodes.
 *
 * Values are stored as the raw Control Chip register value.
 * KS VOLUMELEVEL is a LONG in 1/65536 dB units, but for Phase 1 we
 * use a linear mapping from the register's usable range (MinVal..MaxVal)
 * scaled to 0..0xFFFF and stored in the low 16 bits of the LONG.
 */
static
NTSTATUS
PropertyHandler_Level
(
    IN      PPCPROPERTY_REQUEST PropertyRequest
)
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    CMiniportTopologyAdLibGold *that =
        (CMiniportTopologyAdLibGold *)PropertyRequest->MajorTarget;

    NTSTATUS    ntStatus = STATUS_INVALID_PARAMETER;

    /* Validate node ID */
    if (PropertyRequest->Node == ULONG(-1))
        return ntStatus;

    if (PropertyRequest->Node >= SIZEOF_ARRAY(NodeRegMap))
        return ntStatus;

    /* Validate value size */
    if (PropertyRequest->ValueSize < sizeof(LONG))
        return STATUS_BUFFER_TOO_SMALL;

    LONG channel = CHAN_MASTER;
    /* Extract channel from instance data if present */
    if (PropertyRequest->InstanceSize >= sizeof(LONG))
    {
        channel = *(PLONG(PropertyRequest->Instance));
    }

    NODE_REG_MAP *map = &NodeRegMap[PropertyRequest->Node];

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        BYTE reg;
        if (channel == CHAN_RIGHT && map->RegRight)
            reg = map->RegRight;
        else
            reg = map->RegLeft;

        BYTE val = that->AdapterCommon->ControlRegRead(reg);
        *(PLONG(PropertyRequest->Value)) = (LONG)val;
        PropertyRequest->ValueSize = sizeof(LONG);
        ntStatus = STATUS_SUCCESS;
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
    {
        BYTE val = (BYTE)(*(PLONG(PropertyRequest->Value)));

        /* Clamp to valid range */
        if (val < map->MinVal) val = map->MinVal;
        if (val > map->MaxVal) val = map->MaxVal;

        if (channel == CHAN_RIGHT && map->RegRight)
        {
            that->AdapterCommon->ControlRegWrite(map->RegRight, val);
        }
        else if (channel == CHAN_LEFT || !map->RegRight)
        {
            that->AdapterCommon->ControlRegWrite(map->RegLeft, val);
        }
        else
        {
            /* CHAN_MASTER: set both channels */
            that->AdapterCommon->ControlRegWrite(map->RegLeft, val);
            if (map->RegRight)
            {
                that->AdapterCommon->ControlRegWrite(map->RegRight, val);
            }
        }

        ntStatus = STATUS_SUCCESS;
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        if (PropertyRequest->ValueSize >= (sizeof(KSPROPERTY_DESCRIPTION) +
                                           sizeof(KSPROPERTY_MEMBERSHEADER) +
                                           sizeof(KSPROPERTY_STEPPING_LONG)))
        {
            PKSPROPERTY_DESCRIPTION desc =
                PKSPROPERTY_DESCRIPTION(PropertyRequest->Value);

            desc->AccessFlags       = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET |
                                      KSPROPERTY_TYPE_BASICSUPPORT;
            desc->DescriptionSize   = sizeof(KSPROPERTY_DESCRIPTION) +
                                      sizeof(KSPROPERTY_MEMBERSHEADER) +
                                      sizeof(KSPROPERTY_STEPPING_LONG);
            desc->PropTypeSet.Set   = KSPROPTYPESETID_General;
            desc->PropTypeSet.Id    = VT_I4;
            desc->PropTypeSet.Flags = 0;
            desc->MembersListCount  = 1;
            desc->Reserved          = 0;

            PKSPROPERTY_MEMBERSHEADER members =
                PKSPROPERTY_MEMBERSHEADER(desc + 1);

            members->MembersFlags   = KSPROPERTY_MEMBER_STEPPEDRANGES;
            members->MembersSize    = sizeof(KSPROPERTY_STEPPING_LONG);
            members->MembersCount   = 1;
            members->Flags          = 0;

            PKSPROPERTY_STEPPING_LONG range =
                PKSPROPERTY_STEPPING_LONG(members + 1);

            range->Bounds.SignedMinimum = (LONG)map->MinVal;
            range->Bounds.SignedMaximum = (LONG)map->MaxVal;
            range->SteppingDelta        = 1;
            range->Reserved             = 0;

            PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION) +
                                         sizeof(KSPROPERTY_MEMBERSHEADER) +
                                         sizeof(KSPROPERTY_STEPPING_LONG);
            ntStatus = STATUS_SUCCESS;
        }
        else if (PropertyRequest->ValueSize >= sizeof(KSPROPERTY_DESCRIPTION))
        {
            PKSPROPERTY_DESCRIPTION desc =
                PKSPROPERTY_DESCRIPTION(PropertyRequest->Value);

            desc->AccessFlags       = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET |
                                      KSPROPERTY_TYPE_BASICSUPPORT;
            desc->DescriptionSize   = sizeof(KSPROPERTY_DESCRIPTION) +
                                      sizeof(KSPROPERTY_MEMBERSHEADER) +
                                      sizeof(KSPROPERTY_STEPPING_LONG);
            desc->PropTypeSet.Set   = KSPROPTYPESETID_General;
            desc->PropTypeSet.Id    = VT_I4;
            desc->PropTypeSet.Flags = 0;
            desc->MembersListCount  = 1;
            desc->Reserved          = 0;

            PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION);
            ntStatus = STATUS_SUCCESS;
        }
        else if (PropertyRequest->ValueSize >= sizeof(ULONG))
        {
            *(PULONG(PropertyRequest->Value)) =
                KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET |
                KSPROPERTY_TYPE_BASICSUPPORT;
            PropertyRequest->ValueSize = sizeof(ULONG);
            ntStatus = STATUS_SUCCESS;
        }
    }

    return ntStatus;
}


/*****************************************************************************
 * PropertyHandler_OnOff()
 *****************************************************************************
 * Mute get/set.  Accesses Control Chip register 08h, bit D5 (MU).
 */
static
NTSTATUS
PropertyHandler_OnOff
(
    IN      PPCPROPERTY_REQUEST PropertyRequest
)
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    CMiniportTopologyAdLibGold *that =
        (CMiniportTopologyAdLibGold *)PropertyRequest->MajorTarget;

    NTSTATUS ntStatus = STATUS_INVALID_PARAMETER;

    if (PropertyRequest->Node == ULONG(-1))
        return ntStatus;

    if (PropertyRequest->ValueSize < sizeof(BOOL))
        return STATUS_BUFFER_TOO_SMALL;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        BYTE mode = that->AdapterCommon->ControlRegRead(CTRL_REG_OUTPUT_MODE);
        *(PBOOL(PropertyRequest->Value)) = (mode & CTRL_MODE_MUTE) ? TRUE : FALSE;
        PropertyRequest->ValueSize = sizeof(BOOL);
        ntStatus = STATUS_SUCCESS;
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
    {
        BOOL mute = *(PBOOL(PropertyRequest->Value));
        BYTE mode = that->AdapterCommon->ControlRegRead(CTRL_REG_OUTPUT_MODE);

        if (mute)
            mode |= CTRL_MODE_MUTE;
        else
            mode &= ~CTRL_MODE_MUTE;

        /* Ensure forced bits are set */
        mode |= CTRL_MODE_FORCED_BITS;

        that->AdapterCommon->ControlRegWrite(CTRL_REG_OUTPUT_MODE, mode);
        ntStatus = STATUS_SUCCESS;
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        if (PropertyRequest->ValueSize >= sizeof(ULONG))
        {
            *(PULONG(PropertyRequest->Value)) =
                KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET |
                KSPROPERTY_TYPE_BASICSUPPORT;
            PropertyRequest->ValueSize = sizeof(ULONG);
            ntStatus = STATUS_SUCCESS;
        }
    }

    return ntStatus;
}


/*****************************************************************************
 * PropertyHandler_Tone()
 *****************************************************************************
 * Bass/Treble get/set/basicsupport.
 *
 * Hardware encoding (Control Chip regs 06h/07h):
 *   D3-D0 = tone nibble, D7-D4 must be 1.
 *   Nibble 0x6 = 0 dB (flat), each step = 3 dB.
 *   Bass  range: -12 dB (0x2) to +15 dB (0xB).
 *   Treble range: -12 dB (0x2) to +12 dB (0xA).
 *
 * KS values are LONG in 1/65536 dB units (dB << 16).
 */
static
NTSTATUS
PropertyHandler_Tone
(
    IN      PPCPROPERTY_REQUEST PropertyRequest
)
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    CMiniportTopologyAdLibGold *that =
        (CMiniportTopologyAdLibGold *)PropertyRequest->MajorTarget;

    NTSTATUS ntStatus = STATUS_INVALID_PARAMETER;

    if (PropertyRequest->Node == ULONG(-1))
        return ntStatus;

    /* Validate node/property ID match and set per-node parameters */
    BYTE reg;
    LONG dBMin;
    LONG dBMax;

    if (PropertyRequest->Node == NODE_BASS &&
        PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_BASS)
    {
        reg   = CTRL_REG_BASS;
        dBMin = -12;
        dBMax = 15;
    }
    else if (PropertyRequest->Node == NODE_TREBLE &&
             PropertyRequest->PropertyItem->Id == KSPROPERTY_AUDIO_TREBLE)
    {
        reg   = CTRL_REG_TREBLE;
        dBMin = -12;
        dBMax = 12;
    }
    else
    {
        return ntStatus;
    }

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        if (PropertyRequest->ValueSize < sizeof(LONG))
            return STATUS_BUFFER_TOO_SMALL;

        BYTE val = that->AdapterCommon->ControlRegRead(reg);
        LONG nibble = (LONG)(val & CTRL_TONE_MASK);

        /* Nibble to dB: 0x6 = 0 dB, 3 dB per step */
        LONG dB = (nibble - 6) * 3;
        if (dB < dBMin) dB = dBMin;
        if (dB > dBMax) dB = dBMax;

        /* Return as KS fixed-point (1/65536 dB units) */
        *(PLONG(PropertyRequest->Value)) = dB << 16;
        PropertyRequest->ValueSize = sizeof(LONG);
        ntStatus = STATUS_SUCCESS;
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
    {
        if (PropertyRequest->ValueSize < sizeof(LONG))
            return STATUS_BUFFER_TOO_SMALL;

        /* Extract dB from KS fixed-point */
        LONG ksValue = *(PLONG(PropertyRequest->Value));
        LONG dB = ksValue >> 16;

        /* Clamp to hardware range */
        if (dB < dBMin) dB = dBMin;
        if (dB > dBMax) dB = dBMax;

        /* dB to nibble: 0 dB = 0x6, 3 dB per step */
        LONG nibble = (dB / 3) + 6;
        if (nibble < 0)   nibble = 0;
        if (nibble > 0xF) nibble = 0xF;

        BYTE regVal = CTRL_TONE_FORCED_BITS | (BYTE)(nibble & CTRL_TONE_MASK);
        that->AdapterCommon->ControlRegWrite(reg, regVal);

        ntStatus = STATUS_SUCCESS;
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        if (PropertyRequest->ValueSize >= (sizeof(KSPROPERTY_DESCRIPTION) +
                                           sizeof(KSPROPERTY_MEMBERSHEADER) +
                                           sizeof(KSPROPERTY_STEPPING_LONG)))
        {
            PKSPROPERTY_DESCRIPTION desc =
                PKSPROPERTY_DESCRIPTION(PropertyRequest->Value);

            desc->AccessFlags       = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET |
                                      KSPROPERTY_TYPE_BASICSUPPORT;
            desc->DescriptionSize   = sizeof(KSPROPERTY_DESCRIPTION) +
                                      sizeof(KSPROPERTY_MEMBERSHEADER) +
                                      sizeof(KSPROPERTY_STEPPING_LONG);
            desc->PropTypeSet.Set   = KSPROPTYPESETID_General;
            desc->PropTypeSet.Id    = VT_I4;
            desc->PropTypeSet.Flags = 0;
            desc->MembersListCount  = 1;
            desc->Reserved          = 0;

            PKSPROPERTY_MEMBERSHEADER members =
                PKSPROPERTY_MEMBERSHEADER(desc + 1);

            members->MembersFlags   = KSPROPERTY_MEMBER_STEPPEDRANGES;
            members->MembersSize    = sizeof(KSPROPERTY_STEPPING_LONG);
            members->MembersCount   = 1;
            members->Flags          = 0;

            PKSPROPERTY_STEPPING_LONG range =
                PKSPROPERTY_STEPPING_LONG(members + 1);

            range->Bounds.SignedMinimum = dBMin << 16;
            range->Bounds.SignedMaximum = dBMax << 16;
            range->SteppingDelta        = 3 << 16;
            range->Reserved             = 0;

            PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION) +
                                         sizeof(KSPROPERTY_MEMBERSHEADER) +
                                         sizeof(KSPROPERTY_STEPPING_LONG);
            ntStatus = STATUS_SUCCESS;
        }
        else if (PropertyRequest->ValueSize >= sizeof(KSPROPERTY_DESCRIPTION))
        {
            PKSPROPERTY_DESCRIPTION desc =
                PKSPROPERTY_DESCRIPTION(PropertyRequest->Value);

            desc->AccessFlags       = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET |
                                      KSPROPERTY_TYPE_BASICSUPPORT;
            desc->DescriptionSize   = sizeof(KSPROPERTY_DESCRIPTION) +
                                      sizeof(KSPROPERTY_MEMBERSHEADER) +
                                      sizeof(KSPROPERTY_STEPPING_LONG);
            desc->PropTypeSet.Set   = KSPROPTYPESETID_General;
            desc->PropTypeSet.Id    = VT_I4;
            desc->PropTypeSet.Flags = 0;
            desc->MembersListCount  = 1;
            desc->Reserved          = 0;

            PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION);
            ntStatus = STATUS_SUCCESS;
        }
        else if (PropertyRequest->ValueSize >= sizeof(ULONG))
        {
            *(PULONG(PropertyRequest->Value)) =
                KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET |
                KSPROPERTY_TYPE_BASICSUPPORT;
            PropertyRequest->ValueSize = sizeof(ULONG);
            ntStatus = STATUS_SUCCESS;
        }
    }

    return ntStatus;
}


/*****************************************************************************
 * PropertyHandler_CpuResources()
 *****************************************************************************
 * Reports that we use no host CPU resources (hardware-only mixer).
 */
static
NTSTATUS
PropertyHandler_CpuResources
(
    IN      PPCPROPERTY_REQUEST PropertyRequest
)
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    if (PropertyRequest->ValueSize < sizeof(LONG))
        return STATUS_BUFFER_TOO_SMALL;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        *(PLONG(PropertyRequest->Value)) = KSAUDIO_CPU_RESOURCES_NOT_HOST_CPU;
        PropertyRequest->ValueSize = sizeof(LONG);
        return STATUS_SUCCESS;
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        if (PropertyRequest->ValueSize >= sizeof(ULONG))
        {
            *(PULONG(PropertyRequest->Value)) =
                KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT;
            PropertyRequest->ValueSize = sizeof(ULONG);
            return STATUS_SUCCESS;
        }
    }

    return STATUS_INVALID_PARAMETER;
}
