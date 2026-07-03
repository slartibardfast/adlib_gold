# `developer-toolkit-v1.01/installed/INTERR.H`

> UTF-8 rendering of a DOS-encoded (CP437 / CRLF) file. Byte-for-byte original: [`INTERR.H`](../../../disks/developer-toolkit-v1.01/installed/INTERR.H).

```c

#ifndef	_INTERR_H
#define	_INTERR_H

#ifndef	_GLOBAL_H
#include 	"global.h"
#endif

#ifndef	_CONTROL_H
#include 	"control.h"
#endif

/***************************************************************************
                              Definitions
***************************************************************************/
/*
 *	All AdLib drivers sub id 
 */

typedef enum	AdlibSubDriversID {
	ADLIB_DISPATCH_DRIVER_ID,
	ADLIB_CTRLCHIP_DRIVER_ID,
	ADLIB_FM_DRIVER_ID,		 
	ADLIB_WAVE_DRIVER_ID,	 
	ADLIB_TIMER_DRIVER_ID,	 
	ADLIB_MIDI_DRIVER_ID,	 
	ADLIB_SCSI_DRIVER_ID,	 
	ADLIB_SYNC_DRIVER_ID,
	ADLIB_VOICEPAD_DRIVER_ID,
	ADLIB_MAX_NUMBER_ID
	}
	AdLibSubDriverID;

/***************************************************************************
   								Structure
****************************************************************************/

/*
 * This structure is managed by this CtrlDrv but is used by all other
 * Adlib Drivers. A copy is transfered on request to the driver who
 * ask for it.
 */

typedef    struct  FastAccessTable {
   CallbackPtr	intFunc;
   } FastAccessTable;

/****************************************************************************
								PUBLIC functions  
*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif
		WORD	InitInterruptService(WORD baseAddress);
		WORD	RemoveInterruptService(void);
		CallbackPtr SetDriverCallback(AdLibSubDriverID driverID, 
										CallbackPtr newProc);
		CallbackPtr ResetDriverCallback(AdLibSubDriverID driverID);
#ifdef __cplusplus
	};
#endif


#endif

```
