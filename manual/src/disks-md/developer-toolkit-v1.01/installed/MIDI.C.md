# `developer-toolkit-v1.01/installed/MIDI.C`

> UTF-8 rendering of a DOS-encoded (CP437 / CRLF) file. Byte-for-byte original: [`MIDI.C`](../../../disks/developer-toolkit-v1.01/installed/MIDI.C).

```c

/**************************************************************************

Module name:   Midi.c

Version:		1.01

Author:        Francois Rousseau

Date:          november 1991
                   
Description:   This module includes all code that performs low level
               MIDI operation with the MMA chip from Yamaha into an
               AdLib Gold card environnment.    

*****************************************************************************/

/****************************************************************************
								Module History
12/12/91		0.01
16/12/91		0.02
01/01/92		0.03
18/01/92		0.04
24/01/92		0.05
16/02/92		0.06
28/02/92		0.07
12/03/92       0.09
23/03/92       1.00
*****************************************************************************/

/****************************************************************************
                               Includes
*****************************************************************************/
#ifdef TURBO
#pragma	hdrfile	midi.sym
#endif

#include   <stdio.h>
#include   <dos.h>

#include   "global.H"
#include   "control.h"
#include   "Interr.H"
#include   "midi.h"

#ifdef TURBO
#pragma	hdrstop

#undef inportb
#undef outportb

#endif

/****************************************************************************
                             Definitions
*****************************************************************************/

/*
 * MMA register mapping. Only the registers used by this midi driver.
 */

#define    MIDI_STATUS         0x0D
#define    MIDI_DATA           0x0E

/*
 * Maximum number of byte before overrun in FIFO (16 ? ...)
 */

#define    MIDI_BUFFER_LEN     15

/****************************************************************************
                             Structure
*****************************************************************************/

/*
 *	Queue are used in this module to push data before being send
 */

#define queueLength	1024

/*
 *	Synopsis:		Queue, QueuePtr
 *							 
 *	Description: 	Most of the communication between the various levels
 *					of the ICI is done thru queues. A structure is given to
 *					facilitate the manipulation of queues.			
 */

typedef struct	Queue {
	int		head;
	int		tail;
	int		number;
	BYTE	array[queueLength];
	} Queue,  *QueuePtr;

/****************************************************************************
                           Private Variables
****************************************************************************/

/*
 * The driver likes to keep track of ports to access. Those address are 
 * initialized in the driver initialisation routine.
 */

PRIVATE    WORD    ctrlChipIO = 0x0000;
PRIVATE    WORD    OPL3ChipIO = 0x0000;
PRIVATE    WORD    MMAChipIO  = 0x0000;
PRIVATE    WORD   delayIO    = 0x0080;

/*
 * For short messages a Queue will be used if too many of them are
 * sent in a short period of time.
 */

PRIVATE Queue       MIDITxQueue;

/*
 *	A function pointer must be kept on the current midi event dispatcher
 *	routine. Initialised with DoNothing().
 */

PRIVATE	CallbackPtr currentDispatcher;

/*
 *	Reports if there is data in the transmission queue that will soon
 *	be sent by interrupt.
 */

volatile	BOOL	intSending;


/****************************************************************************
						Private Function Prototyping
****************************************************************************/



PRIVATE    CallbackProc	DoNothing(void);
PRIVATE	WORD	WriteMMAMIDIStatus(BYTE mode, BYTE mask);
PRIVATE	CallbackProc	ExecuteMIDIDispatcher(void);
PRIVATE	WORD 	far MidiDrvService(WORD segm, WORD offs);

PRIVATE	void	QueueByte( QueuePtr queue, BYTE byte);
PRIVATE	BYTE	UnQueueByte( QueuePtr queue);
PRIVATE	void	ResetQueue( QueuePtr queue);
PRIVATE	WORD	GetQueueNumber( QueuePtr queue);

PUBLIC		void	far MMAMidiInterruptHandler(void);


/****************************************************************************
						External Functions prototyping
*****************************************************************************/

/****************************************************************************
                               Main Code
****************************************************************************/

/*
 * Synopsis:	    void	far	MIDIDrvInterruptEntry()
 *
 * Description:    Called by the Fast Interrupt Processing routine of the			
 *					stay-resident dispatcher module.
 *
 *	Argument:		none
 *
 * Return Value:	none
 *
 */

CallbackProc	MIDIDrvInterruptEntry()
	{
	asm		push	ds
	asm		push	es

	asm 	mov		ax, ds
	asm 	mov		es, ax

	MMAMidiInterruptHandler();		// _BL got the MMA status register

	asm	pop		es
	asm	pop		ds
	}

/*
 * Synopsis:       WORD   InitMIDIDriver()
 *
 * Description:    Initialise all local data used by this driver.
 *
 * Argument:       none.
 *
 * Return values:  MIDI_NO_ERROR
 *
 */

PUBLIC
WORD   InitMIDIDriver(void)
   {
	WORD base;

	OPL3ChipIO 	= base = CtGetRelocationAddress();
	ctrlChipIO 	= base + 2;
	MMAChipIO 	= base + 4;

   ResetMMAMidiIn(1); ResetMMAMidiIn(0);
   ResetMMAMidiOut(1); ResetMMAMidiOut(0);

   /*
    *  Disable interrupt in input and output
    */

   MaskMMAMidiOutInterrupt(1);
   MaskMMAMidiInInterrupt(1);
	MaskMMAOverrunError(1);

   ResetQueue(&MIDITxQueue);

   /*
    *  Ready to store next available short message
    */

	intSending = false;

	ResetMidiDispatcher();

   SetDriverCallback(ADLIB_MIDI_DRIVER_ID, MIDIDrvInterruptEntry);

	return MIDI_NO_ERROR;
   }

WORD
CloseMidiDriver(void)

	{
   ResetDriverCallback(ADLIB_MIDI_DRIVER_ID);
	return(0);
	}

	
/*
 * Synopsis:       WORD	SetMidiDispatcher(CallbackPtr function)
 *
 * Description:    This routine is used to set the current interrupt event
 *					dispatcher function.
 *
 * Argument:       CallbackPtr function
 *
 * Return values:  MIDI_NO_ERROR
 *
 */

PUBLIC
WORD	SetMidiDispatcher(CallbackPtr function)
	{
	currentDispatcher = function;
	return MIDI_NO_ERROR;
	}

/*
 * Synopsis:       WORD	ResetMidiDispatcher()	
 *
 * Description:    This routine is used to reset the current interrupt event
 *					dispatcher function to a lcoal DoNothing.
 *
 * Argument:       none;
 *
 * Return values:  MIDI_NO_ERROR
 *
 */

PUBLIC
WORD	ResetMidiDispatcher()
	{
	currentDispatcher = DoNothing;
	return MIDI_NO_ERROR;
	}

/*
 * Synopsis:       callbackProc ExecuteMIDIDispatcher()
 *
 * Description:    This routine is used to call the user callback routine.
 *
 * Argument:       _BX
 *						message to send
 *
 *					_DX:CX
 *						parameter to send
 *
 * Return values:  none.
 *
 */

PRIVATE
CallbackProc ExecuteMIDIDispatcher()
	{
	WORD	msg;
	DWORD	paramLow;
	DWORD	paramHigh;

#ifdef	TURBO
	msg = _BX; 
	paramLow = _CX; 
	paramHigh = _DX; 
#endif

#ifdef	MICROSOFT
	asm		mov		msg, bx
	asm		mov		paramLow, cx
	asm		mov		paramHigh, dx
#endif

	asm 	mov		ax, ds
	asm 	mov		es, ax

	(*currentDispatcher)(msg, (paramHigh << 8) | paramLow);
	}


/*
 * Synopsis:       WORD	WriteMMAMIDIStatus(BYTE mode, BYTE mask)
 *
 * Description:    This routine will write in the MMA register indexed 0x0D.
 *					The reason to centralize the access to this register is
 *					because the Wave driver uses this register.
 *
 * Arguments:      BYTE	mode
 *						specifies if a 1 or a 0 will be placed at the position
 *						specified by mask
 *
 *					BYTE	mask
 *						Where to aplce the 1's or 0's
 *
 * Return values:  MIDI_NO_ERROR
 *					MIDI_FUNCTION_ERROR		error on argument mode
 */

PRIVATE
WORD	WriteMMAMIDIStatus(BYTE mode, BYTE mask)
	{
	if (mode == 1) return(CtSetMMAReg0DBits(mask));
	else if (mode == 0) return(CtResetMMAReg0DBits(mask));
   else return MIDI_FUNCTION_ERROR;
	}

/***************************************************************************
                           MIDI Transmission
****************************************************************************/

/*
 * Synopsis:       WORD	ResetMMAMidiOut(BYTE arg)
 *
 * Description:    This routine is used to access the set the midi out bit
 *                 in the MMA chip. This will reset the MIDI receive
 *                 circuit.
 *
 * Argument:       1:      resets the MIDI receive circuit
 *                 0:      terminates the reset status
 *
 * Return values:  MIDI_NO_ERROR
 *					MIDI_FUNCTION_ERROR
 *
 */

PUBLIC
WORD	ResetMMAMidiOut(BYTE arg)
   {
	return(WriteMMAMIDIStatus(arg, 0x08));
   }

/*
 * Synopsis:       WORD	MaskMMAMidiOutInterrupt(BYTE arg)
 *
 * Description:    Enable/disable interrupt on transmit FIFO empty.
 *
 * Argument:       1:  masks interrupt signals for the MIDI transmit FIFO
 *                 0:  unmask.
 *
 * Return values:  MIDI_NO_ERROR
 *					MIDI_FUNCTION_ERROR
 *
 */

PUBLIC
WORD	MaskMMAMidiOutInterrupt(BYTE arg)
   {
	return(WriteMMAMIDIStatus(arg, 0x04));
   }

/*
 * Synopsis:       WORD	GetMMAMidiOutEmpty()
 *
 * Description:    Return 1 if MIDI transmission FIFO empty. This will clear
 *					all bits in the MMA register.
 *
 * Argument:       none.
 *
 * Return values:  1:      MIDI tramsission FIFO empty
 *                 0:      MIDI tramsission FIFO not empty
 *
 */

PUBLIC
WORD	GetMMAMidiOutEmpty()
   {
   BYTE    retVal;

   /* 
    *  First check if proper initialisation has been done.
    */

   if (MMAChipIO == 0x0000) return 0; 

   retVal = inportb(MMAChipIO);
   outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
   retVal &= 0x08;
   retVal >>= 3;
   return retVal;
   }

/*
 * Synopsis:       WORD	SendMMAMidiData(BYTE data)
 *
 * Description:    This routine is used to transfer a byte into the MIDI
 *                 port.
 *
 * Argument:       BYTE    data
 *                     The byte to send
 *
 * Return values:  0	no error
 *					1	error
 *
 */

PUBLIC
WORD	SendMMAMidiData(BYTE data)
   {
	WORD	retVal = 0;

	asm		pushf
	asm		cli

	if (! intSending) {
		asm	pushf
		asm	cli
		outportb(MMAChipIO, MIDI_DATA);
      outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
		outportb(MMAChipIO + 1, data);
      outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
		asm	popf
		intSending = true;
		}
	else {
		if (GetQueueNumber(&MIDITxQueue) == 1024) retVal = 1;
		else QueueByte(&MIDITxQueue, data);
		}

	asm	popf

	return retVal;
   }

/***************************************************************************
                           MIDI Reception
****************************************************************************/

/*
 * Synopsis:       WORD	ResetMMAMidiIn(BYTE arg)
 *
 * Description:    This routine is used to access the reset the midi in bit
 *                 in the MMA chip. This will reset the MIDI transmit
 *                 circuit.
 *
 * Argument:       1:      resets the MIDI transmit circuit
 *                 0:      terminates the reset status
 *
 * Return values:  MIDI_NO_ERROR
 *					MIDI_FUNCTION_ERROR
 *
 */

PUBLIC
WORD	ResetMMAMidiIn(BYTE arg)
   {
	return(WriteMMAMIDIStatus(arg, 0x02));
   }


/*
 * Synopsis:       WORD	MaskMMAOverrunError(BYTE arg)
 *
 * Description:    This bit enables/disables interrupt to occur on MIDI
 *                 reception overrun error.
 *
 * Argument:       1:      mask interrupt signals due to overrun errors
 *                 0:      un mask them
 *
 * Return values:  MIDI_NO_ERROR
 *					MIDI_FUNCTION_ERROR
 *
 */

PUBLIC
WORD	MaskMMAOverrunError(BYTE arg)
   {
	return(WriteMMAMIDIStatus(arg, 0x10));
   }

/*
 * Synopsis:       WORD	MaskMMAMidiInInterrupt(BYTE arg)
 *
 * Description:    Enable/disable interrupt on midi data available.
 *
 * Argument:       1:  masks interrupt signals for the MIDI receive FIFO
 *                 0:  unmask.
 *
 * Return values:  MIDI_NO_ERROR
 *					MIDI_FUNCTION_ERROR
 *
 */

PUBLIC
WORD	MaskMMAMidiInInterrupt(BYTE arg)
   {
	return(WriteMMAMIDIStatus(arg, 0x01));
   }

/*
 * Synopsis:       WORD	GetMMAMidiInReady()
 *
 * Description:    Return 1 if data available in input from the MIDI port.
 *					This will clear all bits in the MMA register.
 *
 * Argument:       none.
 *
 * Return values:  1:      data available in reception FIFO.
 *                 0:      no data in reception FIFO.
 *
 */

PUBLIC
WORD	GetMMAMidiInReady()
   {
   BYTE    retVal;

   /* 
    *  First check if proper initialisation has been done.
    */

   if (MMAChipIO == 0x0000) return 0; 

   retVal = inportb(MMAChipIO);
   outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
   retVal &= 0x04;
   retVal >>= 2;
   return retVal;
   }

/*
 * Synopsis:       WORD	GetMMAMidiOverrunStatus()
 *
 * Description:    Return 1 if MIDI overrun occured in reception.
 *					This will clear all bits in the MMA register.
 *
 * Argument:       none.
 *
 * Return values:  1:      MIDI reception overrun occured.
 *                 0:      no error occured.
 *
 */

PUBLIC
WORD	GetMMAMidiOverrunStatus()
   {
   BYTE    retVal;

   /* 
    *  First check if proper initialisation has been done.
    */

   if (MMAChipIO == 0x0000) return 0; 

   retVal = inportb(MMAChipIO);
   outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
   retVal &= 0x80;
   retVal >>= 7;
   return retVal;
   }

/*
 * Synopsis:       WORD	ReadMMAMidiData()
 *
 * Description:    This routine is used to read a byte from the MIDI
 *                 port.
 *
 * Argument:       none.
 *
 * Return values:  The byte read.
 *
 */

PUBLIC
WORD	ReadMMAMidiData()
   {
	WORD	retVal;

   if (MMAChipIO == 0x0000) return 0; 

	asm	pushf
	asm	cli
	outportb(MMAChipIO, MIDI_DATA);
   outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
   retVal = inportb(MMAChipIO + 1);
   outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	asm	popf

	return(retVal);
   }


/****************************************************************************

QUEUE Functions:

*****************************************************************************/

/*
 *	Synopsis:		CallbackProc DoNothing()
 *
 *	Description:	Used as a default value for all interrupt service routines
 *
 *	Returned value: None
 *                 
 */

PRIVATE
CallbackProc	DoNothing()
	{
	}

/*
 * Synopsis:		void	QueueByte( queue, byte)     
 *
 *	Description: 	Places the given byte at the tail of the queue.
 *
 *	Argument:		QueuePtr    queue;                 
 *					BYTE        byte;                  
 *
 *	Return value:	none
 *     
 */                                   

PRIVATE
void	QueueByte( QueuePtr queue, BYTE byte)

    {
    int tail;
    
    if (queue->number == queueLength)
        return;
    tail = queue->tail;
    queue->array[tail] = byte;
    if (++tail == queueLength)
        tail = 0;
    queue->tail = tail;
    
    queue->number++;
    }

/*
 * Synopsis:		BYTE    UnQueueByte( QueuePtr queue)
 *
 * Description: 	Unqueues the byte at the head of the specified queue
 *      			and returns it as a result.
 *
 *	Argument:		QueuePtr    queue
 *
 *	Return value:	Return oldest byte stored in the queue.
 */

PRIVATE
BYTE	UnQueueByte( QueuePtr queue)
    {
    BYTE 	byte;
    int 	head;
    
    head = queue->head;
    byte = queue->array[head];

    if (++head == queueLength)
        head = 0;
    queue->head = head;
    queue->number--;
    
    return(byte);
    }

/*
 *	Synopsis:		ResetQueue( QueuePtr queue)
 *                          
 *	Description: 	Zeroes the specified queue.
 *
 *	Argument:		QueuePtr	queue
 *
 *	Return values:	none
 */

PRIVATE
void	ResetQueue( QueuePtr queue)
    {
    queue->head = 0;
    queue->tail = 0;
    queue->number = 0;
    }

/*
 *	Synopsis:		GetQueueNumber( QueuePtr queue)
 *                                               
 *	Description:	Returns the current number of byte in queue.
 *
 *	Argument:  		QueuePtr    queue
 *
 *	Return value:	Number of bytes stored in this queue.
 */

PRIVATE
WORD	GetQueueNumber( QueuePtr queue)
    {
    return(queue->number);
    }


/***************************************************************************
                           MIDI Interrupt
****************************************************************************/

/*
 * Synopsis:       void	MMAMidiInterruptHandler()
 *
 * Description:    Interrupt handler for MIDI in & out. This interrupt 
 *                 handler is called by the CtrlDrv module.
 *
 * Argument:       none.
 *                 
 * Return values:  none always 0
 *                 
 */

PUBLIC
CallbackProc	MMAMidiInterruptHandler()
   {
   int             i;
   BYTE            c;
   BYTE            *p;
   BYTE            len;
   DWORD           data;
	BYTE			mmaStatus;


#ifdef TURBO
	mmaStatus = _BL;			// _BL got the MMA status register
#else
	asm mov mmaStatus, bl
#endif
   /*******************************************************************
    *                  Tranmission interrupt
    *******************************************************************/

	/*
	 *	Fill the transmission FIFO
	 */

   if (mmaStatus & 0x08) {
		if (GetQueueNumber(&MIDITxQueue)) {
			for (i = 0 ; GetQueueNumber(&MIDITxQueue) && (i < MIDI_BUFFER_LEN) ; i++) {
				c = UnQueueByte(&MIDITxQueue);
				outportb(MMAChipIO, MIDI_DATA);
            outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
				outportb(MMAChipIO + 1, c);
            outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
				}
			if (! GetQueueNumber(&MIDITxQueue)) intSending = false;
			}
		else {
			intSending = false;
			}
		}

   /*******************************************************************
    *			                  Reception interrupt
    *******************************************************************/

   if (mmaStatus & 0x04) {
		c = ReadMMAMidiData();
		/*
		 * Delay required
		 */
#ifdef TURBO
		_DX = 0x0000;
		_CX = (WORD)c;
		_BX = MIDIRxData;
#else
		asm mov dx, 0
		asm mov cx, c
		asm mov bx, MIDIRxData
#endif
		ExecuteMIDIDispatcher();
       }

   /*******************************************************************
    *       		           	Overrun
    *******************************************************************/

   if (mmaStatus & 0x80) {
#ifdef TURBO
		_DX = 0x0000;
		_CX = 0x00FF;
		_BX = MIDIRxOverrun;
#else
		asm mov dx, 0
		asm mov cx, 0FFh
		asm mov bx, MIDIRxOverrun;
#endif
	 	ExecuteMIDIDispatcher();

       ResetMMAMidiIn(1); ResetMMAMidiIn(0);
		MaskMMAMidiInInterrupt(0);
		MaskMMAOverrunError(0);
       }
   }

```
