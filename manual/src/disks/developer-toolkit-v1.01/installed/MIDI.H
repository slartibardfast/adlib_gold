#ifndef    _MIDIDRV_H
#define    _MIDIDRV_H

/***************************************************************************
								Definitions
***************************************************************************/

/*
 *	Callback functions are always called with two arguments:
 *		WORD	msg:	Reason of the callback
 *		DWORD	param:	Optionnal parameter
 */

enum	MIDICallbackMsg {
	MIDIDebugMsg,
	MIDIRxData,
	MIDIRxOverrun,
	};

/*
 *	Values returned by the MIDI driver set of routines
 */

enum	MIDIErrorList {
	// MIDI drivers functions ERROR codes

	MIDI_NO_ERROR,				// no error
	MIDI_FUNCTION_ERROR,		// reported by the almost all functions
	};

/***************************************************************************
									Structure
***************************************************************************/

/****************************************************************************
							PUBLIC variables
*****************************************************************************/

/****************************************************************************
								PUBLIC functions  
*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif
	CallbackProc	MIDIDrvInterruptEntry(void);
	WORD	InitMIDIDriver(void);
	WORD    CloseMIDIDriver(void);
	WORD	SetMidiDispatcher(CallbackPtr function);
	WORD	ResetMidiDispatcher(void);
	WORD	ResetMMAMidiOut(BYTE arg);
	WORD	MaskMMAMidiOutInterrupt(BYTE arg);
	WORD	GetMMAMidiOutEmpty(void);
	WORD	SendMMAMidiData(BYTE data);
	WORD	ResetMMAMidiIn(BYTE arg);
	WORD	MaskMMAOverrunError(BYTE arg);
	WORD	MaskMMAMidiInInterrupt(BYTE arg);
	WORD	GetMMAMidiInReady(void);
	WORD	GetMMAMidiOverrunStatus(void);
	WORD	ReadMMAMidiData(void);

#ifdef __cplusplus
	};
#endif

#endif
