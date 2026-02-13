/*****************************************************************************
 * algtopo.h - Ad Lib Gold topology miniport private definitions
 *****************************************************************************
 *
 * Topology miniport exposing the Ad Lib Gold Control Chip mixer as
 * KS volume, tone, and mute nodes for sndvol32.
 */

#ifndef _ALGTOPO_PRIVATE_H_
#define _ALGTOPO_PRIVATE_H_

#include "common.h"


/*****************************************************************************
 * Pin identifiers
 *
 * These match the order in MiniportPins[] (defined in algtopo.cpp).
 */
enum
{
    PIN_WAVEOUT_SOURCE = 0,     /* From Wave render miniport           */
    PIN_FMSYNTH_SOURCE,         /* From FM synth miniport              */
    PIN_AUX_SOURCE,             /* External aux line input             */
    PIN_MIC_SOURCE,             /* Microphone input                    */
    PIN_LINEOUT_DEST,           /* Line output / speaker               */

    PIN_TOP_ELEMENT_COUNT       /* Must be last                        */
};


/*****************************************************************************
 * Node identifiers
 *
 * These match the order in MiniportNodes[] (defined in algtopo.cpp).
 */
enum
{
    NODE_SAMP_VOLUME = 0,       /* Sampling volume L/R (regs 0Bh/0Ch) */
    NODE_FM_VOLUME,             /* FM volume L/R (regs 09h/0Ah)       */
    NODE_AUX_VOLUME,            /* Aux volume L/R (regs 0Dh/0Eh)      */
    NODE_MIC_VOLUME,            /* Mic volume (reg 0Fh)               */
    NODE_MASTER_VOLUME,         /* Master volume L/R (regs 04h/05h)   */
    NODE_BASS,                  /* Bass tone (reg 06h)                */
    NODE_TREBLE,                /* Treble tone (reg 07h)              */
    NODE_MUTE,                  /* Master mute (reg 08h, D5)          */

    NODE_TOP_ELEMENT_COUNT      /* Must be last                        */
};


/*****************************************************************************
 * CMiniportTopologyAdLibGold
 *****************************************************************************
 * Ad Lib Gold topology miniport.
 */
class CMiniportTopologyAdLibGold
:   public IMiniportTopology,
    public CUnknown
{
private:
    PADAPTERCOMMON  AdapterCommon;

    NTSTATUS ProcessResources
    (
        IN      PRESOURCELIST   ResourceList
    );

public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CMiniportTopologyAdLibGold);

    ~CMiniportTopologyAdLibGold();

    /*************************************************************************
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

    /*************************************************************************
     * IMiniportTopology methods
     */
    STDMETHODIMP Init
    (
        IN      PUNKNOWN        UnknownAdapter,
        IN      PRESOURCELIST   ResourceList,
        IN      PPORTTOPOLOGY   Port
    );

    /*************************************************************************
     * Friends — property handlers need access to AdapterCommon
     */
    friend
    NTSTATUS
    PropertyHandler_Level
    (
        IN      PPCPROPERTY_REQUEST PropertyRequest
    );
    friend
    NTSTATUS
    PropertyHandler_OnOff
    (
        IN      PPCPROPERTY_REQUEST PropertyRequest
    );
    friend
    NTSTATUS
    PropertyHandler_Tone
    (
        IN      PPCPROPERTY_REQUEST PropertyRequest
    );
    friend
    NTSTATUS
    PropertyHandler_CpuResources
    (
        IN      PPCPROPERTY_REQUEST PropertyRequest
    );
};

#endif  /* _ALGTOPO_PRIVATE_H_ */
