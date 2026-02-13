/*****************************************************************************
 * common.h - Common code used by all the Ad Lib Gold miniports.
 *****************************************************************************
 *
 * Shared hardware access, interrupt synchronization, and Control Chip
 * register I/O for the Ad Lib Gold sound card WDM driver.
 */

#ifndef _COMMON_H_
#define _COMMON_H_

#include "stdunk.h"
#include "portcls.h"
#include "ksdebug.h"

/*****************************************************************************
 * Constants
 */

/*
 * Maximum number of subdevices: Topology + Wave + FMSynth + MIDI + spare
 */
#define MAX_MINIPORTS           5

/*
 * Maximum DMA buffer length (16 KB, same as SB16 sample)
 */
#define MAXLEN_DMA_BUFFER       0x4000

/*****************************************************************************
 * Port offset constants (relative to I/O base, default 388h)
 *
 *  base+0  FM Bank 0 Address   (OPL3 array 0 register select)
 *  base+1  FM Bank 0 Data      (OPL3 array 0 data write)
 *  base+2  FM Bank 1 Address / Control Chip Address (bank-switched)
 *  base+3  FM Bank 1 Data    / Control Chip Data    (bank-switched)
 *  base+4  MMA Channel 0 Address  (YMZ263)
 *  base+5  MMA Channel 0 Data     (YMZ263)
 *  base+6  MMA Channel 1 Address  (YMZ263)
 *  base+7  MMA Channel 1 Data     (YMZ263)
 */
#define ALG_REG_FM0_ADDR        0x00
#define ALG_REG_FM0_DATA        0x01
#define ALG_REG_FM1_ADDR        0x02
#define ALG_REG_FM1_DATA        0x03
#define ALG_REG_MMA0_ADDR       0x04
#define ALG_REG_MMA0_DATA       0x05
#define ALG_REG_MMA1_ADDR       0x06
#define ALG_REG_MMA1_DATA       0x07

/*****************************************************************************
 * Bank switching values
 *
 * Writing these to base+2 (ALG_REG_FM1_ADDR) switches between
 * the Control Chip register bank and OPL3 array 1.
 */
#define ALG_BANK_CONTROL        0xFF    /* Enable Control Chip access        */
#define ALG_BANK_OPL3           0xFE    /* Enable OPL3 array 1 access        */

/*****************************************************************************
 * Status register bits (read from base+2 in Control Chip mode)
 *
 * D7 = RB  (Register Busy — EEPROM operation in progress)
 * D6 = SB  (Soft Busy — register write in progress)
 * D3 = SCSI interrupt   (ACTIVE LOW: 0 = pending)
 * D2 = Telephone interrupt
 * D1 = Sampling/MMA interrupt
 * D0 = FM/OPL3 timer interrupt
 */
#define ALG_STATUS_RB           0x80
#define ALG_STATUS_SB           0x40
#define ALG_STATUS_BUSY_MASK    0xC0    /* SB | RB                           */

#define ALG_STATUS_SCSI_IRQ     0x08    /* Active low: 0 = pending           */
#define ALG_STATUS_TEL_IRQ      0x04
#define ALG_STATUS_SMP_IRQ      0x02
#define ALG_STATUS_FM_IRQ       0x01
#define ALG_STATUS_IRQ_MASK     0x0F    /* All four IRQ source bits          */

/*****************************************************************************
 * MMA status register bits (read from base+4, MMA Channel 0 address port)
 *
 * These bits are auto-cleared on read.
 */
#define MMA_STATUS_TRQ          0x01    /* Timer interrupt request            */
#define MMA_STATUS_PRQ          0x02    /* Playback FIFO request              */
#define MMA_STATUS_RRQ          0x04    /* MIDI receive data ready            */

/*****************************************************************************
 * Control Chip register indices (0x00 through 0x18)
 */
#define CTRL_REG_CONTROL_ID     0x00    /* EEPROM save/restore; read=model ID */
#define CTRL_REG_TELEPHONE      0x01    /* Telephone control                  */
#define CTRL_REG_GAIN_L         0x02    /* Sampling gain, left channel        */
#define CTRL_REG_GAIN_R         0x03    /* Sampling gain, right channel       */
#define CTRL_REG_MASTER_VOL_L   0x04    /* Final output volume, left          */
#define CTRL_REG_MASTER_VOL_R   0x05    /* Final output volume, right         */
#define CTRL_REG_BASS           0x06    /* Bass tone control                  */
#define CTRL_REG_TREBLE         0x07    /* Treble tone control                */
#define CTRL_REG_OUTPUT_MODE    0x08    /* Mute, stereo mode, source select   */
#define CTRL_REG_FM_VOL_L       0x09    /* FM synth volume, left              */
#define CTRL_REG_FM_VOL_R       0x0A    /* FM synth volume, right             */
#define CTRL_REG_SAMP_VOL_L     0x0B    /* Sampling volume, left              */
#define CTRL_REG_SAMP_VOL_R     0x0C    /* Sampling volume, right             */
#define CTRL_REG_AUX_VOL_L      0x0D    /* Aux input volume, left             */
#define CTRL_REG_AUX_VOL_R      0x0E    /* Aux input volume, right            */
#define CTRL_REG_MIC_VOL        0x0F    /* Microphone volume (mono)           */
#define CTRL_REG_TEL_VOL        0x10    /* Telephone volume                   */
#define CTRL_REG_AUDIO_SEL      0x11    /* Filters, PC speaker, mic feedback  */
#define CTRL_REG_RESERVED       0x12    /* Reserved (must be 0)               */
#define CTRL_REG_IRQ_DMA0       0x13    /* IRQ select + DMA channel 0         */
#define CTRL_REG_DMA1           0x14    /* DMA channel 1                      */
#define CTRL_REG_AUDIO_RELOC    0x15    /* Audio section I/O relocation       */
#define CTRL_REG_SCSI_IRQ_DMA   0x16    /* SCSI IRQ/DMA select                */
#define CTRL_REG_SCSI_RELOC     0x17    /* SCSI section I/O relocation        */
#define CTRL_REG_SURROUND       0x18    /* Surround sound module (YM7128)     */

#define CTRL_REG_MAX            0x19    /* Total number of Control Chip regs  */

/*
 * Range of mixer-related registers for shadow cache restore on D0 entry.
 * Registers 0x04 through 0x0F cover all volume/tone/mode controls.
 */
#define CTRL_MIXER_FIRST        0x04
#define CTRL_MIXER_LAST         0x0F

/*****************************************************************************
 * Register 0x00 (Control/ID) bit definitions
 */

/* Write bits */
#define CTRL_ID_SAVE            0x02    /* D1: Save registers to EEPROM      */
#define CTRL_ID_RESTORE         0x01    /* D0: Restore registers from EEPROM */

/* Read bits */
#define CTRL_ID_MODEL_MASK      0x0F    /* D3-D0: Model identifier           */
#define CTRL_ID_OPT_TEL         0x20    /* D5: 0=telephone present           */
#define CTRL_ID_OPT_SURROUND   0x40    /* D6: 0=surround present            */
#define CTRL_ID_OPT_SCSI        0x80    /* D7: 0=SCSI present                */

/* Model ID values */
#define ALG_MODEL_GOLD1000      0x00
#define ALG_MODEL_GOLD2000      0x01
#define ALG_MODEL_GOLD2000MC    0x02

/*****************************************************************************
 * Register 0x08 (Output Mode) bit definitions
 */
#define CTRL_MODE_FORCED_BITS   0xC0    /* D7-D6 must be 1                   */
#define CTRL_MODE_MUTE          0x20    /* D5: Mute                          */
#define CTRL_MODE_STEREO_MASK   0x0C    /* D3-D2: Stereo mode                */
#define CTRL_MODE_STEREO_MONO   0x00    /* Forced mono                       */
#define CTRL_MODE_STEREO_LINEAR 0x04    /* Linear stereo                     */
#define CTRL_MODE_STEREO_PSEUDO 0x08    /* Pseudo stereo                     */
#define CTRL_MODE_STEREO_SPATIAL 0x0C   /* Spatial stereo                    */
#define CTRL_MODE_SOURCE_MASK   0x03    /* D1-D0: Source select              */

/*****************************************************************************
 * Registers 0x06/0x07 (Bass/Treble) bit definitions
 */
#define CTRL_TONE_FORCED_BITS   0xF0    /* D7-D4 must be 1 for regs 06h/07h */
#define CTRL_TONE_MASK          0x0F    /* D3-D0: tone value                 */

/*****************************************************************************
 * Register 0x11 (Audio Selection) bit definitions
 */
#define CTRL_ASEL_SPKR          0x20    /* D5: PC speaker connected          */
#define CTRL_ASEL_MFB           0x08    /* D3: Mic feedback removed          */
#define CTRL_ASEL_XMO           0x04    /* D2: Aux input mono                */
#define CTRL_ASEL_FLT1          0x02    /* D1: Ch1 filter (1=input,0=output) */
#define CTRL_ASEL_FLT0          0x01    /* D0: Ch0 filter (1=input,0=output) */

/*****************************************************************************
 * Register 0x13 (IRQ/DMA Channel 0) bit definitions
 */
#define CTRL_DMA0_ENABLE        0x80    /* D7: DMA channel 0 enable          */
#define CTRL_DMA0_SEL_SHIFT     5       /* D6-D5: DMA channel select         */
#define CTRL_DMA0_SEL_MASK      0x60
#define CTRL_IRQ_ENABLE         0x10    /* D4: Audio interrupt enable         */
#define CTRL_IRQ_SEL_MASK       0x07    /* D2-D0: IRQ line select            */

/* IRQ select values (D2-D0 of register 0x13) */
#define CTRL_IRQ_SEL_3          0x00
#define CTRL_IRQ_SEL_4          0x01
#define CTRL_IRQ_SEL_5          0x02
#define CTRL_IRQ_SEL_7          0x03
#define CTRL_IRQ_SEL_10         0x04    /* Gold 2000 only */
#define CTRL_IRQ_SEL_11         0x05    /* Gold 2000 only */
#define CTRL_IRQ_SEL_12         0x06    /* Gold 2000 only */
#define CTRL_IRQ_SEL_15         0x07    /* Gold 2000 only */

/*****************************************************************************
 * Register 0x14 (DMA Channel 1) bit definitions
 */
#define CTRL_DMA1_ENABLE        0x80    /* D7: DMA channel 1 enable          */
#define CTRL_DMA1_SEL_SHIFT     5       /* D6-D5: DMA channel select         */
#define CTRL_DMA1_SEL_MASK      0x60

/*****************************************************************************
 * SIZEOF_ARRAY helper (if not already defined)
 */
#ifndef SIZEOF_ARRAY
#define SIZEOF_ARRAY(ar)        (sizeof(ar)/sizeof(ar[0]))
#endif

/*****************************************************************************
 * Mixer settings structure for registry persistence
 */
typedef struct
{
    PWCHAR  KeyName;
    BYTE    RegisterIndex;
    BYTE    RegisterSetting;
} MIXERSETTING, *PMIXERSETTING;

/*****************************************************************************
 * Forward declarations for miniport interfaces
 *
 * These are defined in their respective headers (algwave.h, midi.h).
 * We only need opaque pointers here for the ISR dispatch mechanism.
 */

/* {A1B2C3D4-1111-2222-3333-AABBCCDDEEFF} */
DEFINE_GUID(IID_IWaveMiniportAdLibGold,
0xa1b2c3d4, 0x1111, 0x2222, 0x33, 0x33, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff);

DECLARE_INTERFACE_(IWaveMiniportAdLibGold, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(void,ServiceWaveISR)
    (   THIS
    )   PURE;
};

typedef IWaveMiniportAdLibGold *PWAVEMINIPORTADLIBGOLD;

/* {A1B2C3D4-4444-5555-6666-AABBCCDDEEFF} */
DEFINE_GUID(IID_IMidiMiniportAdLibGold,
0xa1b2c3d4, 0x4444, 0x5555, 0x66, 0x66, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff);

DECLARE_INTERFACE_(IMidiMiniportAdLibGold, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(void,ServiceMidiISR)
    (   THIS
    )   PURE;
};

typedef IMidiMiniportAdLibGold *PMIDIMINIPORTADLIBGOLD;

/*****************************************************************************
 * IAdapterCommon
 *****************************************************************************
 * Interface for adapter common object.
 */

/* {7EDA2950-BF9F-11D0-871F-00A0C911B544} — same GUID family as SB16 sample */
DEFINE_GUID(IID_IAdapterCommon,
0x7eda2950, 0xbf9f, 0x11d0, 0x87, 0x1f, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);

DECLARE_INTERFACE_(IAdapterCommon, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    /* Lifecycle */
    STDMETHOD_(NTSTATUS,Init)
    (   THIS_
        IN      PRESOURCELIST   ResourceList,
        IN      PDEVICE_OBJECT  DeviceObject
    )   PURE;

    STDMETHOD_(PINTERRUPTSYNC,GetInterruptSync)
    (   THIS
    )   PURE;

    /* Control Chip register access */
    STDMETHOD_(void,ControlRegWrite)
    (   THIS_
        IN      BYTE    Register,
        IN      BYTE    Value
    )   PURE;

    STDMETHOD_(BYTE,ControlRegRead)
    (   THIS_
        IN      BYTE    Register
    )   PURE;

    STDMETHOD_(void,ControlRegReset)
    (   THIS
    )   PURE;

    /* Bank switching */
    STDMETHOD_(void,EnableControlBank)
    (   THIS
    )   PURE;

    STDMETHOD_(void,EnableOPL3Bank1)
    (   THIS
    )   PURE;

    /* OPL3 register access (bank-coordinated) */
    STDMETHOD_(void,WriteOPL3)
    (   THIS_
        IN      ULONG   Address,
        IN      UCHAR   Data
    )   PURE;

    /* MMA register access (YMZ263) */
    STDMETHOD_(void,WriteMMA)
    (   THIS_
        IN      BYTE    Register,
        IN      BYTE    Value
    )   PURE;

    STDMETHOD_(BYTE,ReadMMA)
    (   THIS_
        IN      BYTE    Register
    )   PURE;

    /* Miniport registration for ISR dispatch */
    STDMETHOD_(void,SetWaveMiniport)
    (   THIS_
        IN      PWAVEMINIPORTADLIBGOLD  Miniport
    )   PURE;

    STDMETHOD_(void,SetMidiMiniport)
    (   THIS_
        IN      PMIDIMINIPORTADLIBGOLD  Miniport
    )   PURE;

    /* Registry persistence */
    STDMETHOD_(NTSTATUS,RestoreMixerSettingsFromRegistry)
    (   THIS
    )   PURE;

    STDMETHOD_(NTSTATUS,SaveMixerSettingsToRegistry)
    (   THIS
    )   PURE;

    /* EEPROM persistence */
    STDMETHOD_(NTSTATUS,SaveToEEPROM)
    (   THIS
    )   PURE;

    STDMETHOD_(NTSTATUS,RestoreFromEEPROM)
    (   THIS
    )   PURE;

    /* Card identification */
    STDMETHOD_(BYTE,GetCardModel)
    (   THIS
    )   PURE;
};

typedef IAdapterCommon *PADAPTERCOMMON;

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
);

#endif  /* _COMMON_H_ */
