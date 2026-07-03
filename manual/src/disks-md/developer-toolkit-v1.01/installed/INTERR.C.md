# `developer-toolkit-v1.01/installed/INTERR.C`

> UTF-8 rendering of a DOS-encoded (CP437 / CRLF) file. Byte-for-byte original: [`INTERR.C`](../../../disks/developer-toolkit-v1.01/installed/INTERR.C).

```c

/**************************************************************************
                   
Module name:   Interr.C

Version:		0.09

Author:        Francois Rousseau

Date:          november 1992

Description:	

*****************************************************************************/

/****************************************************************************
								Module History
21/11/92       0.09
*****************************************************************************/

/****************************************************************************
                               Includes
*****************************************************************************/

#ifdef TURBO
#pragma	hdrfile	interr.sym
#endif

#include   <stdio.h>
#include   <stdlib.h>
#include   <dos.h>

#include   "global.H"
#include   "Interr.H"
#include   "control.H"
#include   "timer.H"
#include   "midi.H"
#include   "wave.H"

#ifdef TURBO
#pragma	hdrstop
#undef		inportb					// Protection against fast access
#undef		outportb
#endif

/*****************************************************************************/

PRIVATE	unsigned int near mySP;
PRIVATE	unsigned int near mySS;
PRIVATE	unsigned int near oldSP;
PRIVATE	unsigned int near oldSS;

/*
 *	Redirection table (function pointers) to external modules:
 *		MIDI, Wave, timer, etc. (listed in ctrldrv.h)
 */

PRIVATE	FastAccessTable driverTable[ADLIB_MAX_NUMBER_ID + 1];

/*
 *	Mask for 8259
 */

PRIVATE	BYTE intMasks[] = { 
			1 << 3, 1 << 4, 1 << 5, 1 << 7,
			1 << 10 -8, 1 << 11 -8, 1 << 12 -8, 1 << 15 -8};

/*
 *	This module needs access to on board chips. The initialisation
 *	routine gets the basic address and stores it in those variables.
 */

PRIVATE    WORD   controlIoPort = 0x0000;
PRIVATE    WORD   opl3IoPort 	 = 0x0000;
PRIVATE    WORD   mmaIoPort  	 = 0x0000;
PRIVATE    WORD   delayIO    = 0x0080;

/*
 *	Used for level 1 to keep track of the current gold interrupt selection.
 * the value used in software is set with an environment variable in the
 *	autoexec.bat file.
 */

PRIVATE	BYTE	interrupt8259;

/*
 *	Interrupt routines pointers for installation and de-installation
 */

PRIVATE	void		interrupt	ProcessInterrupt();
PRIVATE	void		(interrupt far *oldVect)();

/*****************************************************************************/

/*
 *	Synopsis:		CallbackProc FastDoNothing()
 *
 *	Description:	Used as a default value for all interrupt service routines
 *
 * Arguments:		None.
 *
 *	Returned value: None
 *                 
 */

PRIVATE
CallbackProc FastDoNothing()
	{
	}

/*
 *	Synopsis:		WORD	InitInterruptService(WORD baseAddress)
 *
 *	Description:	Used to install the GOLD interrupt routine.
 *
 * Arguments:		WORD baseAddress
 *						The port address of the mma & opl3 chip and
 *						control chip are passed as argument.
 *
 *	Return Value:	0	no error
 *					1	error		
 *                 
 */

WORD	InitInterruptService(WORD baseAddress)
	{
	WORD  	port8259;
	BYTE	theByte;
	BYTE	mask;
	char	*s;
   int     i;
	BYTE	vector;
	BYTE 	vecs[] = { 11, 12, 13, 15, 0x72, 0x73, 0x74, 0x77 };
	BYTE 	IRQS[] = { 0, 0, 0, 0, 1, 2, 0, 3, 0, 0, 4, 5, 6, 0, 0, 7 };

	opl3IoPort 		= baseAddress;
	controlIoPort 	= baseAddress + 2;
	mmaIoPort 		= baseAddress + 4;

   for (i = 0; i < ADLIB_MAX_NUMBER_ID + 1; i++) {
	    driverTable[i].intFunc = FastDoNothing;
		}

	/*
	 * 	Set interrupt vector 
	 */

	if (gssLevel == level2) {
		/*
		 *	IRQ3 = 0, IRQ7 = 3 
		 */
		interrupt8259 = CtGetInterruptLineNbr();
		}
	if (gssLevel == level1) {
		s = getenv("GOLDINTSEL");
		if ((s == NULL) OR (! isxdigit(s[0]))) {
		   	printf("Environment variable for Gold interrupt not valid: %s\n", s);
			/*
			 * 	Default interrupt IRQ 5
			 */
		   	s = "2";
			}
		interrupt8259 = IRQS[Hexa(s)];
		}

	asm	pushf				// Disable interrupt
	asm	cli

	vector = vecs[interrupt8259];

   oldVect = getvect(vector);
   setvect(vector, ProcessInterrupt);

	if (interrupt8259 < 4) 
		port8259 = 0x0020;
	else port8259 = 0x00A0;

	/*
	 *	Enable ctrl chip interrupt by putting a 0 at the appropriate
	 *	place in one of the two 8259.
	 */

	theByte = inportb(port8259 + 1);	   
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	mask = theByte & ~intMasks[interrupt8259];
	outportb(port8259 + 1, mask);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);

	asm	popf

	return(0);
	}


/*
 *	Synopsis:		WORD	RemoveInterruptService()
 *
 *	Description:	This routine will restore the context before the 
 *					execution of this program.
 *
 *	Argument:		no arguments
 *
 *	Return Value:	0	no error
 *					1	error
 */

WORD	RemoveInterruptService()
	{
	WORD  	port8259;
	BYTE	theByte;
	BYTE	mask;

	asm	pushf
	asm	cli

   setvect(CtGetInterruptRoutine(), oldVect);

	if (interrupt8259 < 4) 
		port8259 = 0x0020;
	else port8259 = 0x00A0;

	/*
	 *	Enable ctrl chip interrupt by putting a 1 at the appropriate
	 *	place in one of the two 8259.
	 */

	theByte = inportb(port8259 + 1);	   
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	mask = theByte | intMasks[interrupt8259];
	outportb(port8259 + 1, mask);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);

	asm	popf
	return(0);
	}

CallbackPtr 
SetDriverCallback(AdLibSubDriverID driverID, CallbackPtr newProc)

	{
	driverTable[driverID].intFunc = newProc;
   return(newProc);
   }

CallbackPtr 
ResetDriverCallback(AdLibSubDriverID driverID)

	{
   driverTable[driverID].intFunc = FastDoNothing;
   return(FastDoNothing);
   }

/*************************************************************************/

#define	maxReentry 		18				// No reason, just enough...

static unsigned int 	near thisSP;
static unsigned int 	near reentryCount= 0;
static BYTE			near ctStatus[maxReentry];
static BYTE			near status[maxReentry];
static BYTE			near MMA[maxReentry];
static BYTE			near OPL3[maxReentry];
static BYTE			near SCSI[maxReentry];
static BYTE			near PHONE[maxReentry];

/*
 *	patch against filled 0's when receiving MIDI
 */

static BYTE			near again[maxReentry];	

static BYTE			near loopCond[maxReentry]; // To implement level 1 & 2

#ifndef TURBO
static BYTE	tempStatus;
#endif

/*
 *	Synopsis:			ProcessInterrupt()
 *
 *	Description:		This is the main interrupt routine generated on the PC
 *						bus by the Control Chip. Interrupts may either come
 *						from the OPL3 or MMA chips. Level 1 only supports
 *						interrupts from the MMA.
 *
 *						SCSI interrupts are not handles,they should be handled by
 *						the CD-ROM driver.
 *
 *	Argument:			no arguments
 *
 * Returned values:	none.
 *
 */	

void   interrupt	ProcessInterrupt()
	{
	/*
	 *	Prepare context switching
	 */

	asm 	mov		ax, ds
	asm 	mov		es, ax

   thisSP = mySP 	- (512 *  reentryCount++);
   asm     mov     DS:oldSS, ss
   asm     mov     DS:oldSP, sp
   asm     mov     ss, DS:mySS
   asm     mov     sp, DS:thisSP

	/*
	 *	Enable reentry (by enabling interrupt)
	 */

	// asm		sti          

	/*
	 *	To provide support for level_1 and 2. This loopCond simulates
	 *	in both case a Do While control structure. It may look complex...
	 */
	loopCond[reentryCount] = 0xF0;

   MMA[reentryCount] = 0x00;
   again[reentryCount] = 0x00;			

	/*
	 *	In level 1 there won't be any of these interruption sources
	 */

	OPL3[reentryCount] = 0x00;
	SCSI[reentryCount] = 0x00;
	PHONE[reentryCount] = 0x00;

	/*
	 *	It's just like a Do While at this point. The while loop is used
	 *	for spped while processing fast incoming interrupt.
	 */

	while (~loopCond[reentryCount] & 0x07) {

		/*
		 * 	If running level 2 then we need a whille loop over the content
		 *	of control chip's status register. Make sure to assign the loopCond
		 *	variable for the while loop condition above. The level 1 while
		 *	loop condition is set at the end of the while block ( { } ).
		 */
		 
		if (gssLevel == level2) {
			loopCond[reentryCount] = GetControlRegister(-1);
			ctStatus[reentryCount] = loopCond[reentryCount];
			MMA[reentryCount] 	= (ctStatus[reentryCount] & 0x02) >> 1;
			PHONE[reentryCount] = (ctStatus[reentryCount] & 0x04) >> 2;
			OPL3[reentryCount] 	= (ctStatus[reentryCount] & 0x01);
			}

		/*
		 *	In level 1, interrupts will be coming from the MMA chip only. To
		 *	keep the following code functionnal for the two level we force
		 *	this variable to 0.
		 */

		if (gssLevel == level1) {
			MMA[reentryCount] 	= 0;
			}
	       	
		/*
	 	 *  First find which interrupt to process
	 	 *	Check Wave sampler first for speed
	 	 */

		if (! MMA[reentryCount]) {
			/*****************************************************
			 *****************************************************
			 *
			 * MIDI reception requires a while loop over the MMA
			 *
			 *****************************************************
			 *****************************************************
			 */
			status[reentryCount] = inportb(mmaIoPort);
			outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);

			if (status[reentryCount] & 0x03) {
				#ifdef TURBO			
					_BL = status[reentryCount];
				#else
					tempStatus = status[reentryCount];
					asm	mov	bl, tempStatus
				#endif
				(*driverTable[ADLIB_WAVE_DRIVER_ID].intFunc)();
				}
			if (status[reentryCount] & 0x70) {
				#ifdef TURBO
					_BL = status[reentryCount];
				#else
					tempStatus = status[reentryCount];
					asm	mov	bl, tempStatus
				#endif
					
				(*driverTable[ADLIB_TIMER_DRIVER_ID].intFunc)();
				}
			if (status[reentryCount] & 0x80) {
				/*
			 	 * 	Special case of overrun on MIDI or Wave...
			 	 */
				#ifdef TURBO
					_BL = status[reentryCount];
				#else
					tempStatus = status[reentryCount];
					asm	mov	bl, tempStatus
				#endif
				(*driverTable[ADLIB_MIDI_DRIVER_ID].intFunc)();
				}
			if (status[reentryCount] & 0x0C) {  
				#ifdef TURBO
					_BL = status[reentryCount];
				#else
					tempStatus = status[reentryCount];
					asm	mov	bl, tempStatus
				#endif
				(*driverTable[ADLIB_MIDI_DRIVER_ID].intFunc)();
				if (status[reentryCount] & 0x04) {
					again[reentryCount] = inportb(mmaIoPort);
					outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
					}
				}
			}

		/********************************************************************
		 *				OPL3, PHONE interruptions
		 ********************************************************************/

		if (gssLevel == level2) {
			if (! OPL3[reentryCount]) {

				if (inportb(opl3IoPort) & 0x60) {
					outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
					/*
				 	* Don't process MMA interrupt a second time with _BL == 0
				 	*/
					#ifdef TURBO
						_BL = 0x00;
					#else
						asm	xor	bl, bl
					#endif
					(*driverTable[ADLIB_TIMER_DRIVER_ID].intFunc)();
					}

				/*
	 			 *	Reset last timer interrupt in OPL3
				 */

				outportb(opl3IoPort, 0x04);
				outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
				outportb(opl3IoPort + 1, 0x80);
				outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
				}

			if (! PHONE[reentryCount]) {
				CtGetHangUpPickUpTelephoneLine();
				}
			} /* End of level 2 */

		if (gssLevel == level1) {
			loopCond[reentryCount] = 0x0F;
		}

	}	/* End of while loop	*/

   --reentryCount;

   asm     mov     ss, DS:oldSS
   asm     mov     sp, DS:oldSP

	/********************************************************************
	 *				Chain Interrupt
	 ********************************************************************/

	(*oldVect)();

	/********************************************************************
	 *				Reset interrupt under service (8259)
	 ********************************************************************/
	asm pushf
   asm cli


	if (interrupt8259 > 3) {
		outportb(0x00A0, 0x20);
		outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
		}

	outportb(0x0020, 0x20);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);

   asm popf
	}





```
