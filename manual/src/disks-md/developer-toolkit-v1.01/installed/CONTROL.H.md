# `developer-toolkit-v1.01/installed/CONTROL.H`

> UTF-8 rendering of a DOS-encoded (CP437 / CRLF) file. Byte-for-byte original: [`CONTROL.H`](../../../disks/developer-toolkit-v1.01/installed/CONTROL.H).

```c

#ifndef	_CTRLDRV_H
#define	_CTRLDRV_H

#ifndef	_GLOBAL_H
#include 	"global.h"
#endif

/***************************************************************************
                              Definitions
***************************************************************************/

/*
 *	Values returned by the timer driver set of routines
 */

enum	CtrlErrorList {
	// Ctrl drivers functions ERROR codes

	CTRL_NO_ERROR,				// no error
	CTRL_FUNCTION_ERROR,		// reported by the almost all functions
	};


/***************************************************************************
   								Structure
****************************************************************************/

/*
 * Fast access function pointer used to call entry functions of all other
 * drivers.
 */

typedef	void	(interrupt far *VoidProcPtr)();

/*
 * Fast access function pointer used to call entry functions of all other
 * drivers.
 */

typedef	void 	far CallbackProc;
typedef	void	(far *CallbackPtr)();

/****************************************************************************
								PUBLIC variables  
*****************************************************************************/

/* 
 * GSS Compatibility level is now determined at run-time in
 *	CtGetGoldCardPresence() -Called by InitControlDriver()-.
 *
 * Global variable gssLevel can be used to determine which compatibility
 * level is used, in the application. 0 is no card found.
 *
 * Global variable phantomControl is set to 1 if the level2 control
 * features are located as phantom register at baseAddress + 2. 
 */

#define levelNoCard 0
#define level1 1
#define level2 2

extern unsigned char phantomControl;
extern unsigned char gssLevel;

/****************************************************************************
								PUBLIC functions  
*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif
		WORD	InitControlDriver(void);
		WORD	CloseControlDriver(void);
		WORD	CtSetControlDriverAddress(WORD destPort);

		WORD    SetControlChip(WORD controlID, DWORD param);
		WORD    CtStoreConfigInPermMem(void);
		WORD    CtRestoreConfigFromPermMem(void);
		WORD    CtSetChannel0SampGain(WORD value);
		WORD    CtSetChannel1SampGain(WORD value);
		WORD    CtSetChannel0SampFreq(WORD value);
		WORD    CtSetChannel1SampFreq(WORD value);
		WORD    CtSetChannel0FilterMode(WORD value);
		WORD    CtSetChannel1FilterMode(WORD value);
		WORD    CtStereoMonoAuxSamp(WORD value);
		WORD    CtEnabDisabMicroOutput(WORD value);
		WORD    CtEnabDisabInternPcSpeak(WORD value);
		WORD    CtSelectInterruptLineNbr(WORD value);
		WORD    CtEnabDisabInterrupt(WORD value);
		WORD    CtSelectDMA0ChannelSampChan(WORD value);
		WORD    CtSelectDMA1ChannelSampChan(WORD value);
		WORD    CtEnabDisabDMA0SampChan(WORD value);
		WORD    CtEnabDisabDMA1SampChan(WORD value);
		WORD    CtSetRelocationAddress(WORD value);
		WORD    CtSetMixerLevelForFMLeft(WORD value);
		WORD    CtSetMixerLevelForFMRight(WORD value);
		WORD    CtSetMixerLevelForLeftSamplePb(WORD value);
		WORD    CtSetMixerLevelForRightSamplePb(WORD value);
		WORD    CtSetMixerLevelForAuxLeft(WORD value);
		WORD    CtSetMixerLevelForAuxRight(WORD value);
		WORD    CtSetMixerLevelForMicrophone(WORD value);
		WORD    CtSetMixerLevelForTelephone(WORD value);
		WORD    CtSetOutputVolumeLeft(WORD value);
		WORD    CtSetOutputVolumeRight(WORD value);
		WORD    CtSetOutputBassLevel(WORD value);
		WORD    CtSetOutputTrebleLevel(WORD value);
		WORD    CtEnabDisabOutputMuting(WORD value);
		WORD    CtSelectSCSIInterruptNumber(WORD value);
		WORD    CtEnabDisabSCSIInterrupt(WORD value);
		WORD    CtEnabDisabSCSIDMA(WORD value);
		WORD    CtSelectSCSIDMAChannel(WORD value);
		WORD    CtSetSCSIRelocationAddress(WORD value);
		WORD    CtSetHangUpPickUpTelephoneLine(WORD value);
		WORD    CtSelectOutputSources(WORD value);
		WORD    CtSelectOutputMode(WORD value);
		WORD    CtSelectSurroundingPreset(WORD value);
		WORD    CtSelectInterruptRoutine(void);
		WORD    GetControlChip(WORD controlID);
		WORD    CtGetDriverInformation(void);
		WORD    CtGetBoardIdentificationCode(void);
		WORD    CtGetBoardOptions(void);
		WORD    CtGetControllerStatus(void);
		WORD    CtGetRingTelephoneStatus(void);

		WORD    CtGetChannel0SampGain(void);
		WORD    CtGetChannel1SampGain(void);
		WORD    CtGetChannel0SampFreq(void);
		WORD    CtGetChannel0SampFreq(void);
		WORD    CtGetChannel0FilterMode(void); 
		WORD    CtGetChannel1FilterMode(void);
		WORD    CtGetStereoMonoAuxSamp(void);
		WORD    CtGetEnabDisabMicroOutput(void);
		WORD    CtGetEnabDisabInternPcSpeak(void);
		WORD    CtGetInterruptLineNbr(void);
		WORD    CtGetEnabDisabInterrupt(void);
		WORD    CtGetDMA0ChannelSampChan(void);
		WORD    CtGetEnabDisabDMA0SampChan(void);
		WORD    CtGetDMA1ChannelSampChan(void);
		WORD    CtGetEnabDisabDMA1SampChan(void);
		WORD    CtGetRelocationAddress(void);
		WORD    CtGetMixLevelForFMLeft(void);
		WORD    CtGetMixLevelForFMRight(void);
		WORD    CtGetMixLevelForLeftSamplePb(void);
		WORD    CtGetMixLevelForRightSamplePb(void);
		WORD    CtGetMixLevelForAuxLeft(void);
		WORD    CtGetMixLevelForAuxRight(void);
		WORD    CtGetMixLevelForMicrophone(void);
		WORD    CtGetMixLevelForTelephone(void);
		WORD    CtGetOutputVolumeLeft(void);
		WORD    CtGetOutputVolumeRight(void);
		WORD    CtGetOutputBassLevel(void);
		WORD    CtGetOutputTrebleLevel(void);
		WORD    CtGetEnabDisabOutputMuting(void);
		WORD    CtGetSCSIInterruptNumber(void);
		WORD    CtGetEnabDisabSCSIInterrupt(void);
		WORD    CtGetSCSIDMAChannel(void);
		WORD    CtGetEnabDisabSCSIDMA(void);
		WORD    CtGetSCSIRelocationAddress(void);
		WORD    CtGetHangUpPickUpPhoneLine(void);
		WORD    CtGetOutputSources(void);
		WORD    CtGetOutputMode(void);
		WORD    CtGetSurroundingPreset(void);
		WORD    CtGetInterruptRoutine(void);
		WORD    CtGetGoldCardPresence(void);
		WORD	SetControlRegister(WORD reg, WORD val);
		WORD	GetControlRegister(int reg);

		WORD	CtProgramSurroundPreset(BYTE *ptrData);

		WORD	CtGetMMAReg0D(void);
		WORD	CtSetMMAReg0DBits(BYTE serie);
		WORD	CtResetMMAReg0DBits(BYTE serie);

		WORD	Hexa(char *s);

#ifdef __cplusplus
	};
#endif


#endif

```
