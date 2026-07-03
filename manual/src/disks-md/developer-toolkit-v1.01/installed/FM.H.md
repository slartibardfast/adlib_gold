# `developer-toolkit-v1.01/installed/FM.H`

> UTF-8 rendering of a DOS-encoded (CP437 / CRLF) file. Byte-for-byte original: [`FM.H`](../../../disks/developer-toolkit-v1.01/installed/FM.H).

```c

#ifndef _FMDRV_H
#define _FMDRV_H

#ifndef	_GLOBAL_H
#include	"Global.h"
#endif

/***************************************************************************
								Definitions
***************************************************************************/

/*
 *	A timbre is composed of that size in BYTE
 */

#define	OPL3_TIMBRE_SIZE		28


/***************************************************************************
								Structure
***************************************************************************/


/*
 *	This structure includes enough space to store a complete FM timbre 
 *	description.
 */


typedef struct TIMBRE {
   BYTE    data[OPL3_TIMBRE_SIZE];
   } TIMBRE;

/*
 * This structure must be used when calling any services from the 
 * FM Driver thru its own entry routine.
 */

typedef    struct FMArgum {
		WORD 	controlID;
		WORD 	paramWord;
		int 	paramInt[2];
		BOOL	paramBool[3];
		TIMBRE	far *timbrePtr;
		} FMArgum;

/****************************************************************************
								PUBLIC functions  
*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif
	WORD	InitFMDriver(void);
	WORD	CloseFMDriver(void);
	void 	SetPercModeOpl3(BOOL state);
	void 	Set4OpMaskOpl3(WORD mask);
	void 	SetGlobalOpl3(BOOL noteSelectEnable, BOOL amplitudeModEnable, 
                   		BOOL vibDepthEnable, int pitchBendRange);
	void 	PresetOpl3(int voiceNum, TIMBRE *timbrePtr);
	void 	LevelOpl3(int voiceNum, int level);
	void 	NoteOnOpl3(int voiceNum, int note);
	void 	NoteOffOpl3(int voiceNum);	 
	void 	PitchBendOpl3(int voiceNum, WORD pitchBend);
	void 	LeftRightOpl3(int voiceNum, int leftRight);
#ifdef __cplusplus
	};
#endif

#endif

```
