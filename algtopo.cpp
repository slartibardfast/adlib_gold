/*****************************************************************************
 * algtopo.cpp - Ad Lib Gold topology miniport implementation
 *****************************************************************************
 *
 * Exposes the Ad Lib Gold Control Chip mixer as a KS topology filter.
 * Property handlers translate between KS volume/mute/tone properties
 * and Control Chip register reads/writes via the adapter common object.
 *
 * All property handlers are functional: volume (Level), mute (OnOff), and
 * bass/treble (Tone) with dB-scaled get/set/basicsupport, plus CPU resources.
 */

#include "algtopo.h"
#include "sp2modes.h"   /* SP2 surround presets the sp2_mode node downloads (call/0012) */

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
 * Pin 5: Line capture input (external, the analog source the ADC records)
 * Pin 6: Wave capture output (to the wave capture miniport)
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
    },
    /* PIN_LINEIN_SOURCE — the analog source the ADC records (bridge in) */
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
            NULL,
            0
        }
    },
    /* PIN_WAVEIN_DEST — bridge to the Wave capture miniport (dataflow out) */
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
            &KSNODETYPE_LEGACY_AUDIO_CONNECTOR,
            NULL,
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


/* SP2 surround enable (KSPROPERTY_AUDIO_LOUDNESS on/off) */
static NTSTATUS PropertyHandler_Sp2OnOff(PPCPROPERTY_REQUEST);

static
PCPROPERTY_ITEM PropertiesSp2Enable[] =
{
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_LOUDNESS,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Sp2OnOff
    },
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_CPU_RESOURCES,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_CpuResources
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationSp2Enable, PropertiesSp2Enable);


/* SP2 surround mode select (KSPROPERTY_AUDIO_WIDENESS, one step per documented mode) */
static NTSTATUS PropertyHandler_Sp2Mode(PPCPROPERTY_REQUEST);

static
PCPROPERTY_ITEM PropertiesSp2Mode[] =
{
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_WIDENESS,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_Sp2Mode
    },
    {
        &KSPROPSETID_Audio,
        KSPROPERTY_AUDIO_CPU_RESOURCES,
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
        PropertyHandler_CpuResources
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationSp2Mode, PropertiesSp2Mode);


/*****************************************************************************
 * MiniportNodes
 *****************************************************************************
 *
 * Topology:
 *   Pin0 (wave)  -> [SampVol] -+
 *   Pin1 (FM)    -> [FMVol]   -+-> [MasterVol] -> [Bass] -> [Treble]
 *   Pin2 (aux)   -> [AuxVol]  -+     -> [Mute] -> Pin4 (lineout)
 *   Pin3 (mic)   -> [MicVol]  -+
 *
 * When the card reports the SP2 surround module, two more nodes sit between
 * Treble and Mute:  [Treble] -> [Sp2Enable] -> [Sp2Mode] -> [Mute].
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
    },
    /* NODE_RECORD_GAIN -- ADC record level (regs 02h/03h), on the capture path. Reuses the
     * volume automation table; PropertyHandler_Level reads NodeRegMap[NODE_RECORD_GAIN]. */
    {
        0,
        &AutomationVolume,
        &KSNODETYPE_VOLUME,
        NULL
    },
    /* NODE_SP2_ENABLE */
    {
        0,
        &AutomationSp2Enable,
        &KSNODETYPE_LOUDNESS,
        NULL
    },
    /* NODE_SP2_MODE */
    {
        0,
        &AutomationSp2Mode,
        &KSNODETYPE_STEREO_WIDE,
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
    { NODE_MUTE,          0,  PCFILTER_NODE, PIN_LINEOUT_DEST },

    /* Capture path: analog line in -> ADC record gain -> wave capture bridge */
    { PCFILTER_NODE,  PIN_LINEIN_SOURCE,  NODE_RECORD_GAIN,  1 },
    { NODE_RECORD_GAIN,  0,  PCFILTER_NODE, PIN_WAVEIN_DEST }
};


/*****************************************************************************
 * MiniportConnectionsSp2
 *****************************************************************************
 * Wiring for a card that has the SP2 surround module: identical to the base
 * graph except the output runs treble -> [Sp2Enable] -> [Sp2Mode] -> mute.
 */
static
PCCONNECTION_DESCRIPTOR
MiniportConnectionsSp2[] =
{
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

    /* Master -> bass -> treble -> SP2 enable -> SP2 mode -> mute -> lineout */
    { NODE_MASTER_VOLUME, 0,  NODE_BASS,       1 },
    { NODE_BASS,          0,  NODE_TREBLE,     1 },
    { NODE_TREBLE,        0,  NODE_SP2_ENABLE, 1 },
    { NODE_SP2_ENABLE,    0,  NODE_SP2_MODE,   1 },
    { NODE_SP2_MODE,      0,  NODE_MUTE,       1 },
    { NODE_MUTE,          0,  PCFILTER_NODE,   PIN_LINEOUT_DEST },

    /* Capture path: analog line in -> ADC record gain -> wave capture bridge */
    { PCFILTER_NODE,  PIN_LINEIN_SOURCE,  NODE_RECORD_GAIN,  1 },
    { NODE_RECORD_GAIN,  0,  PCFILTER_NODE, PIN_WAVEIN_DEST }
};


/*****************************************************************************
 * MiniportFilterDescriptor / MiniportFilterDescriptorSp2
 *****************************************************************************
 * Two topology filter descriptors sharing one node array: the base exposes the
 * always-present mixer nodes; the SP2 variant additionally exposes the two
 * surround nodes and routes through them. GetDescription() picks by hardware.
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
    NODE_BASE_ELEMENT_COUNT,                /* NodeCount (no SP2)     */
    MiniportNodes,                          /* Nodes                  */
    SIZEOF_ARRAY(MiniportConnections),      /* ConnectionCount        */
    MiniportConnections,                    /* Connections            */
    0,                                      /* CategoryCount          */
    NULL                                    /* Categories             */
};

static
PCFILTER_DESCRIPTOR MiniportFilterDescriptorSp2 =
{
    0,                                      /* Version                */
    NULL,                                   /* AutomationTable        */
    sizeof(PCPIN_DESCRIPTOR),               /* PinSize                */
    SIZEOF_ARRAY(MiniportPins),             /* PinCount               */
    MiniportPins,                           /* Pins                   */
    sizeof(PCNODE_DESCRIPTOR),              /* NodeSize               */
    NODE_TOP_ELEMENT_COUNT,                 /* NodeCount (with SP2)   */
    MiniportNodes,                          /* Nodes                  */
    SIZEOF_ARRAY(MiniportConnectionsSp2),   /* ConnectionCount        */
    MiniportConnectionsSp2,                 /* Connections            */
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

    /* Zero the members the failure cleanup and destructor touch before anything can
     * fail: the DDK placement new does not zero pool, and the destructor releases
     * AdapterCommon under an `if` guard that heap garbage would defeat. */
    AdapterCommon = NULL;
    Sp2Present    = FALSE;
    Sp2Enabled    = FALSE;
    Sp2Mode       = 1;

    NTSTATUS ntStatus =
        UnknownAdapter->QueryInterface(IID_IAdapterCommon,
                                       (PVOID *)&AdapterCommon);

    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = ProcessResources(ResourceList);

        if (NT_SUCCESS(ntStatus))
        {
            AdapterCommon->ControlRegReset();

            /* Detect the SP2 surround module (ID register OP1, 0 when present)
             * and seed the surround state to off. */
            BYTE id = AdapterCommon->ControlRegRead(CTRL_REG_CONTROL_ID);
            Sp2Present = (id & CTRL_ID_OPT_SURROUND) ? FALSE : TRUE;
            Sp2Enabled = FALSE;
            Sp2Mode    = 1;     /* first active mode, applied when enabled */
            if (Sp2Present)
            {
                /* Download the bypass preset (every attenuator at -90 dB)
                 * rather than trust the chip's initial-clear, which holds at
                 * cold power-on but not across a warm reboot. The payload and
                 * protocol are audited against the SDK's surround appendix
                 * (call/0033). */
                Sp2Download();
            }
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

    /* Surface the SP2 surround nodes only where the card reports the module,
     * so the mixer reflects the hardware rather than offering a dead control
     * (topology.allium SurroundResolves, call/0012). */
    *OutFilterDescriptor = Sp2Present ? &MiniportFilterDescriptorSp2
                                      : &MiniportFilterDescriptor;

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * Property Handlers
 *****************************************************************************/

/*
 * Mapping table: node ID -> Control Chip register pair (left, right).
 * For mono nodes, RegRight == 0 (unused).
 */
/*
 * Register encoding family for a level node (call/0026):
 *   NK_DBCODE  - master 04/05: byte = 0xC0|field, field 0x1C..0x3F == -64..+6 dB, 2 dB/step.
 *   NK_LINVOL  - mixing volumes 09-0F: linear-in-amplitude, code 128 (silent) .. 255 (0 dB).
 *   NK_LINGAIN - record gain 02/03: linear gain = (val*10)/256, val 0..255.
 * KSPROPERTY_AUDIO_VOLUMELEVEL is a signed LONG in 1/65536 dB, NOT a raw register byte, so
 * the handler converts through the tables/helpers below (SB16 template, DDK wdm.txt:24077).
 */
enum NODE_KIND { NK_DBCODE, NK_LINVOL, NK_LINGAIN };

#define KSDB(x)   ((LONG)(x) * 65536)      /* integer dB -> KS 1/65536-dB units */

typedef struct
{
    BYTE        RegLeft;
    BYTE        RegRight;
    BYTE        MinVal;     /* Raw register clamp, low  (write backstop) */
    BYTE        MaxVal;     /* Raw register clamp, high (write backstop) */
    NODE_KIND   Kind;       /* Register encoding family                  */
    LONG        DbMin;      /* Advertised range floor,   1/65536 dB      */
    LONG        DbMax;      /* Advertised range ceiling, 1/65536 dB      */
    LONG        DbDelta;    /* Advertised stepping,      1/65536 dB      */
} NODE_REG_MAP;

/*
 * LinVolTab[idx] : register code (128+idx) -> KS 1/65536 dB, for the linear mixing
 * volumes. idx 1..127 == code 129..255; code 255 == 0 dB (unity). value = round(
 * 20*log10(idx/127.0)*65536). value[0] is the silent sentinel (code 128, owned by MUTE).
 * Generated offline (call/0026); no floating point runs in the kernel.
 */
static const LONG LinVolTab[128] =
{
    (LONG)0x80000000, -2757498, -2362932, -2132125, -1968366, -1841344, -1737559, -1649811,
    -1573800, -1506753, -1446778, -1392523, -1342993, -1297430, -1255245, -1215971,
    -1179233, -1144724, -1112187, -1081410, -1052212, -1024438, -997957, -972654,
    -948427, -925190, -902864, -881381, -860679, -840703, -821405, -802740,
    -784667, -767151, -750158, -733657, -717621, -702024, -686844, -672057,
    -657646, -643590, -629872, -616478, -603391, -590599, -578088, -565846,
    -553861, -542124, -530624, -519351, -508298, -497455, -486814, -476369,
    -466113, -456037, -446137, -436406, -426839, -417430, -408174, -399066,
    -390101, -381276, -372585, -364025, -355592, -347281, -339091, -331016,
    -323055, -315203, -307458, -299817, -292278, -284836, -277491, -270240,
    -263080, -256008, -249024, -242124, -235306, -228570, -221912, -215331,
    -208825, -202393, -196033, -189743, -183522, -177368, -171279, -165256,
    -159295, -153396, -147558, -141779, -136058, -130394, -124785, -119232,
    -113732, -108284, -102889, -97544, -92248, -87002, -81803, -76652,
    -71547, -66487, -61471, -56500, -51571, -46685, -41840, -37037,
    -32273, -27549, -22864, -18217, -13608, -9036, -4500, 0,
};

/*
 * RecGainTab[val] : record-gain register value -> KS 1/65536 dB. val 1..255,
 * gain = (val*10)/256, value = round(20*log10(val*10.0/256)*65536). value[0] = -inf sentinel.
 */
static const LONG RecGainTab[256] =
{
    (LONG)0x80000000, -1845808, -1451242, -1220436, -1056676, -929654, -825870, -738121,
    -662110, -595064, -535088, -480834, -431304, -385740, -343555, -304282,
    -267544, -233034, -200498, -169720, -140522, -112749, -86268, -60964,
    -36738, -13500, 8826, 30309, 51011, 70986, 90284, 108949,
    127022, 144538, 161532, 178033, 194069, 209665, 224846, 239632,
    254044, 268100, 281817, 295211, 308298, 321090, 333602, 345844,
    357828, 369566, 381066, 392338, 403392, 414235, 424875, 435320,
    445577, 455652, 465552, 475283, 484850, 494259, 503515, 512623,
    521588, 530414, 539104, 547664, 556098, 564408, 572599, 580673,
    588635, 596486, 604231, 611872, 619412, 626853, 634198, 641449,
    648610, 655681, 662666, 669566, 676383, 683120, 689778, 696358,
    702864, 709296, 715656, 721946, 728168, 734322, 740410, 746434,
    752394, 758293, 764132, 769911, 775632, 781296, 786904, 792458,
    797958, 803405, 808801, 814146, 819441, 824687, 829886, 835037,
    840143, 845203, 850218, 855190, 860118, 865004, 869849, 874653,
    879416, 884140, 888825, 893472, 898081, 902654, 907189, 911689,
    916154, 920584, 924980, 929342, 933670, 937967, 942231, 946463,
    950664, 954834, 958974, 963084, 967165, 971216, 975239, 979234,
    983201, 987140, 991052, 994938, 998797, 1002630, 1006438, 1010220,
    1013978, 1017710, 1021419, 1025103, 1028764, 1032401, 1036016, 1039607,
    1043176, 1046723, 1050247, 1053750, 1057232, 1060692, 1064132, 1067551,
    1070949, 1074327, 1077686, 1081024, 1084344, 1087644, 1090924, 1094187,
    1097430, 1100655, 1103862, 1107051, 1110222, 1113376, 1116512, 1119632,
    1122734, 1125819, 1128888, 1131940, 1134976, 1137996, 1141000, 1143988,
    1146960, 1149917, 1152859, 1155786, 1158698, 1161594, 1164477, 1167344,
    1170198, 1173037, 1175862, 1178673, 1181470, 1184254, 1187024, 1189780,
    1192524, 1195254, 1197971, 1200675, 1203367, 1206045, 1208712, 1211365,
    1214007, 1216636, 1219253, 1221859, 1224452, 1227034, 1229603, 1232162,
    1234709, 1237244, 1239769, 1242282, 1244784, 1247275, 1249756, 1252225,
    1254684, 1257132, 1259570, 1261998, 1264415, 1266822, 1269219, 1271605,
    1273982, 1276349, 1278706, 1281054, 1283391, 1285719, 1288038, 1290347,
    1292647, 1294938, 1297220, 1299492, 1301755, 1304010, 1306255, 1308492,
};

static
NODE_REG_MAP NodeRegMap[] =
{
    /* NODE_SAMP_VOLUME  */ { CTRL_REG_SAMP_VOL_L,   CTRL_REG_SAMP_VOL_R,   0x80, 0xFF, NK_LINVOL,  -2757498,   0,       KSDB(1) },
    /* NODE_FM_VOLUME    */ { CTRL_REG_FM_VOL_L,     CTRL_REG_FM_VOL_R,     0x80, 0xFF, NK_LINVOL,  -2757498,   0,       KSDB(1) },
    /* NODE_AUX_VOLUME   */ { CTRL_REG_AUX_VOL_L,    CTRL_REG_AUX_VOL_R,    0x80, 0xFF, NK_LINVOL,  -2757498,   0,       KSDB(1) },
    /* NODE_MIC_VOLUME   */ { CTRL_REG_MIC_VOL,      0,                     0x80, 0xFF, NK_LINVOL,  -2757498,   0,       KSDB(1) },
    /* NODE_MASTER_VOLUME*/ { CTRL_REG_MASTER_VOL_L, CTRL_REG_MASTER_VOL_R, 0xDC, 0xFF, NK_DBCODE,  KSDB(-64),  KSDB(6), KSDB(2) },
    /* NODE_BASS   -- PropertyHandler_Tone  */ { 0, 0, 0, 0, NK_DBCODE, 0, 0, 0 },
    /* NODE_TREBLE -- PropertyHandler_Tone  */ { 0, 0, 0, 0, NK_DBCODE, 0, 0, 0 },
    /* NODE_MUTE   -- PropertyHandler_OnOff */ { 0, 0, 0, 0, NK_DBCODE, 0, 0, 0 },
    /* NODE_RECORD_GAIN  */ { CTRL_REG_GAIN_L,       CTRL_REG_GAIN_R,       0x00, 0xFF, NK_LINGAIN, -1845808, 1308492,  KSDB(1) },
};


/*
 * RegToKsDb() - Control Chip register byte -> KS 1/65536 dB (per node family).
 */
static
LONG
RegToKsDb
(
    IN  NODE_REG_MAP *m,
    IN  BYTE          b
)
{
    switch (m->Kind)
    {
    case NK_DBCODE:
        {
            BYTE field = (BYTE)(b & 0x3F);
            if (field < 0x1C)                    /* off region (-80 dB), report floor */
                return m->DbMin;
            return m->DbMin + (LONG)(field - 0x1C) * KSDB(2);
        }
    case NK_LINVOL:
        {
            int idx = (int)b - 128;
            if (idx <= 0)  return m->DbMin;      /* silent -> advertised floor */
            if (idx > 127) idx = 127;
            return LinVolTab[idx];
        }
    case NK_LINGAIN:
        if (b == 0) return m->DbMin;
        return RecGainTab[b];
    }
    return m->DbMin;
}

/*
 * KsDbToReg() - KS 1/65536 dB -> Control Chip register byte (clamped, nearest).
 */
static
BYTE
KsDbToReg
(
    IN  NODE_REG_MAP *m,
    IN  LONG          lvl
)
{
    if (lvl < m->DbMin) lvl = m->DbMin;
    if (lvl > m->DbMax) lvl = m->DbMax;

    switch (m->Kind)
    {
    case NK_DBCODE:
        {
            LONG steps = (lvl - m->DbMin + KSDB(1)) / KSDB(2);   /* nearest 2 dB, 0..35 */
            if (steps < 0)  steps = 0;
            if (steps > 35) steps = 35;
            return (BYTE)(0xC0 | (0x1C + (BYTE)steps));          /* D6,D7 forced 1 */
        }
    case NK_LINVOL:
        {
            int  best  = 1;
            LONG bestd = LinVolTab[1] - lvl;
            int  i;
            if (bestd < 0) bestd = -bestd;
            for (i = 2; i <= 127; i++)
            {
                LONG d = LinVolTab[i] - lvl;
                if (d < 0) d = -d;
                if (d < bestd) { bestd = d; best = i; }
            }
            return (BYTE)(128 + best);
        }
    case NK_LINGAIN:
        {
            int  best  = 1;
            LONG bestd = RecGainTab[1] - lvl;
            int  i;
            if (bestd < 0) bestd = -bestd;
            for (i = 2; i <= 255; i++)
            {
                LONG d = RecGainTab[i] - lvl;
                if (d < 0) d = -d;
                if (d < bestd) { bestd = d; best = i; }
            }
            return (BYTE)best;
        }
    }
    return m->MinVal;
}


/*****************************************************************************
 * PropertyHandler_Level()
 *****************************************************************************
 * Volume level get/set for source and master volume nodes.
 *
 * KS VOLUMELEVEL is a signed LONG in 1/65536 dB units. GET/SET convert
 * to/from the Control Chip register byte per the node's encoding family via
 * RegToKsDb()/KsDbToReg(), and BASICSUPPORT advertises the range in dB. A
 * unity (0 dB) SET therefore lands on the audible register value rather than
 * clamping a raw (BYTE)0 up to the silent minimum (call/0026).
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
        *(PLONG(PropertyRequest->Value)) = RegToKsDb(map, val);
        PropertyRequest->ValueSize = sizeof(LONG);
        ntStatus = STATUS_SUCCESS;
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
    {
        LONG ksval = *(PLONG(PropertyRequest->Value));
        BYTE val   = KsDbToReg(map, ksval);   /* KS 1/65536 dB -> register byte */

        /* Raw-register safety backstop (KsDbToReg already returns in-range) */
        if (val < map->MinVal) val = map->MinVal;
        if (val > map->MaxVal) val = map->MaxVal;

        /* Diagnostic: what the mixer/system SETs, and the register value it maps to.
         * Compiles out of the free build (checked build only). */
        _DbgPrintF(DEBUGLVL_VERBOSE,
            ("VolSet node=%d ch=%d ksval=%d -> reg0x%02X=0x%02X",
             (ULONG)PropertyRequest->Node, (LONG)channel, ksval,
             (ULONG)map->RegLeft, (ULONG)val));

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

            range->Bounds.SignedMinimum = map->DbMin;
            range->Bounds.SignedMaximum = map->DbMax;
            range->SteppingDelta        = map->DbDelta;
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
 * CMiniportTopologyAdLibGold::Sp2Download()
 *****************************************************************************
 * Download the currently selected surround preset to the SP2. When disabled we
 * send the off preset (the datasheet initial-clear state), so toggling the
 * enable node is audible without changing the mode. Each of the 31 registers
 * goes out over Control-Chip register 0x18 (WriteSurroundReg, call/0012).
 */
void
CMiniportTopologyAdLibGold::
Sp2Download
(   void
)
{
    PAGED_CODE();

    if (!AdapterCommon || !Sp2Present)
        return;

    unsigned char preset[SP2_NUM_REGS];
    int i;

    Sp2ModePreset(Sp2Enabled ? (int)Sp2Mode : 0, preset);

    for (i = 0; i < SP2_NUM_REGS; i++)
        AdapterCommon->WriteSurroundReg((BYTE)i, preset[i]);
}


/*****************************************************************************
 * PropertyHandler_Sp2OnOff()
 *****************************************************************************
 * SP2 surround enable get/set (KSPROPERTY_AUDIO_LOUDNESS, a BOOL on/off).
 */
static
NTSTATUS
PropertyHandler_Sp2OnOff
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
        *(PBOOL(PropertyRequest->Value)) = that->Sp2Enabled;
        PropertyRequest->ValueSize = sizeof(BOOL);
        ntStatus = STATUS_SUCCESS;
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
    {
        that->Sp2Enabled = *(PBOOL(PropertyRequest->Value)) ? TRUE : FALSE;
        that->Sp2Download();
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
 * PropertyHandler_Sp2Mode()
 *****************************************************************************
 * SP2 surround mode select (KSPROPERTY_AUDIO_WIDENESS, one step per documented
 * mode). The value is a mode index 0..SP2_MODE_COUNT-1.
 */
static
NTSTATUS
PropertyHandler_Sp2Mode
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

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        if (PropertyRequest->ValueSize < sizeof(LONG))
            return STATUS_BUFFER_TOO_SMALL;

        *(PLONG(PropertyRequest->Value)) = (LONG)that->Sp2Mode;
        PropertyRequest->ValueSize = sizeof(LONG);
        ntStatus = STATUS_SUCCESS;
    }
    else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
    {
        if (PropertyRequest->ValueSize < sizeof(LONG))
            return STATUS_BUFFER_TOO_SMALL;

        LONG mode = *(PLONG(PropertyRequest->Value));

        if (mode < 0)
            mode = 0;
        if (mode >= SP2_MODE_COUNT)
            mode = SP2_MODE_COUNT - 1;

        that->Sp2Mode = (ULONG)mode;
        that->Sp2Download();        /* no-op unless enabled */
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

            range->Bounds.SignedMinimum = 0;
            range->Bounds.SignedMaximum = SP2_MODE_COUNT - 1;
            range->SteppingDelta        = 1;
            range->Reserved             = 0;

            PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION) +
                                         sizeof(KSPROPERTY_MEMBERSHEADER) +
                                         sizeof(KSPROPERTY_STEPPING_LONG);
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
