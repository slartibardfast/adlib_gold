
/**************************************************************************

Module name:   Timer.c

Version:		1.01

Author:        Francois Rousseau

Date:          november 1991
                   
Description:   This module is used as a timer driver for the Adlib Gold
				Card. It provides routines to interface with the 5
				available timers of the Yamaha OPL3 and MMA.

*****************************************************************************/

/****************************************************************************
								Module History
12/12/91		0.01
14/12/91		0.02
01/01/92		0.03
18/01/92		0.04
24/01/92		0.05
16/02/92		0.06
28/02/92		0.07
12/03/92       0.09
23/03/92       1.00
11/11/92       1.01
*****************************************************************************/

/****************************************************************************
                               Includes
*****************************************************************************/

#ifdef TURBO
#pragma	hdrfile	timer.sym
#endif

#include	<stdio.h>
#include	<dos.h>

#include   "global.h"
#include   "control.h"
#include   "interr.H"
#include   "timer.h"

#ifdef TURBO
#pragma	hdrstop

#undef inportb
#undef outportb
#endif

/****************************************************************************
                             Definitions
*****************************************************************************/

/*
 * The Yamaha OPL3 includes 2 general purpose 8 bits timers. The first timer
 * has a 80 us resolution with a periodic range of .08 msec to 20.4 msec.
 * The second timer has a 320 us resolution with a periodic range of .32 msec
 * to 81.6 msec. 
 *
 * The stay close to the Yamaha documentation we choose to call the timer
 * for OPL3 timer 1 and 2, and for the MMA timer 0, base counter, 1 and 2
 *
 */

#define    OPL3TimerNbr    2

/*
 * The Yamaha MMA includes 3 timers for MIDI usages and one for base counting.
 *     0- Generates MIDI active sense, SMPTE time.
 *      - Base counter.
 *     1- MIDI clock.
 *     2- Record, playback counters.
 *
 * Timer 0 is an autonomous 16 bit timer with a basic clock 
 * resolution of 1.89 us with a periodic range of 1.89 us to 123.83 msec.
 * 
 * The second timer is a special base counter used for the two other timers. 
 * This base counter is a 12 bit timer with a basic clock resolution of 1.89 
 * us with a periodic range of 1.89 us to 7.738 msec.
 *
 * Timer 1 is a 4 bit timer based on the base counter clock for it's own
 * resolution. Possible range is 1.89 us to 116.071 msec.
 *
 * Timer 2 is a 16 bit timer based on the base counter clock for it's own
 * resolution. Possible range is 1.89 us to 507116.92 msec.
 *
 */

#define	MMATimerNbr     4
		
/*
 * OPL3 register name and index
 */

#define	OPL3Timer1Register  0x02
#define    OPL3Timer2Register  0x03
#define    OPL3CommandRegister 0x04

/*
 * MMA register name and index
 */

#define    MMATimer0LowRegister        0x02
#define    MMATimer0HighRegister       0x03
#define    MMABaseCounterLowRegister   0x04
#define    MMATimer1BCRegister         0x05
#define    MMATimer2LowRegister        0x06
#define    MMATimer2HighRegister       0x07
#define    MMACommandRegister          0x08


enum MMARegisterImage {
	MMAImageTimers,
   MMAImageCommand,
   };

/****************************************************************************
								Structure
*****************************************************************************/
/****************************************************************************
								Structure
*****************************************************************************/

/*
 * This structure must be used when calling any services from the 
 * Timer Driver thru its own entry routine.
 */

typedef    struct TimerArgum {
		WORD 		controlID;
		WORD 		timerDv;
		DWORD 		param;
		DWORD 		param2;
		CallbackPtr function;
		} TimerArgum;

/*
 * Some functions needs to know each timer physical limits and current 
 * availability. This structure is managed by the timer driver.
 *
 * BOOL	bInUse:		is this timer currently in use by an application ?    
 * DWORD	lPeriodMin: minimum period allowed in microseconds
 * DWORD	lPeriodMax: maximum period allowed in microseconds
 * WORD	*function():the service routine associated with the interrupt
 *		
 */

typedef   struct TimerUse {
	BOOL		bInUse;     
   DWORD		lPeriodMin;
   DWORD		lPeriodMax;
	BOOL		oneShotEvent;
   CallbackPtr function;
   } TimeUse;

/*
 *	Type of function used for the service table
 */

typedef 	WORD 	(*WordProcPtr)();


/****************************************************************************
							Private variables
*****************************************************************************/

/*
 * The driver likes to keep track of ports to access. Those address are 
 * initialized in the driver initialisation routine (by one of the control
 *	chip driver function).
 */

PRIVATE    WORD   ctrlChipIO = 0x0000;
PRIVATE    WORD   OPL3ChipIO = 0x0000;
PRIVATE    WORD   MMAChipIO  = 0x0000;
PRIVATE    WORD   delayIO    = 0x0080;
/*
 * For each timer allocates memory to hold the structure.
 */

PRIVATE    TimeUse	timeUse[OPL3TimerNbr + MMATimerNbr];

/*
 * OPL3 and MMA register image:
 *
 * A image copy of the register this driver use must be kept for bit
 * manipulation: reset, enable/disable, start/stop.
 *
 */

PRIVATE    BYTE	OPL3RegisterImage;
PRIVATE    BYTE	MMARegisterImage[2];


/*
 * It will be useful to keep the current divider associated with each
 * timer.
 */

PRIVATE    WORD	timerDivider[OPL3TimerNbr + MMATimerNbr];

/*
 *	Private prototype used with the table of function pointer, for all
 *	routines not supported.
 */



/*
 *	Those two variables keep tracks of event scheduling.
 */

BYTE	eventSetMode;
BYTE	eventSetTimer;


/****************************************************************************
						PRIVATE Functions prototyping
*****************************************************************************/


PRIVATE    CallbackProc	DoNothing();
PRIVATE    BYTE	ReadOPL3Register(BYTE regis, BYTE alReg);
PRIVATE    BYTE	ReadMMARegister(BYTE regis, BYTE alReg);
PRIVATE    void	WriteOPL3Register(BYTE regis, BYTE alReg, BYTE value);
PRIVATE    void	WriteMMARegister(BYTE regis, BYTE alReg, BYTE value);
PRIVATE    WORD	AllocateTimer(int index);
PRIVATE    WORD	FreeTimer(int index);
PRIVATE    DWORD	SetTimerPeriod(int timer, DWORD lPeriod);
PRIVATE    DWORD	SplitDivider(DWORD divider);
PRIVATE	WORD	far TimerDrvService(WORD segm, WORD offs);
PRIVATE	void	ExecServiceGeneric(BYTE timer);


/****************************************************************************
									Routines
*****************************************************************************/


/*
 * Synopsis:	    void far	TimerDrvIntEntry()
 *
 * Description:    Called by the Fast Int Processing routine of the
 *					stay-resident dispatcher module.
 *
 *	Argument:		none
 *
 * Return Value:	none
 *
 */

CallbackProc	TimerDrvIntEntry()
	{
	BYTE	mmaStatus;
	BYTE	status;
								   
#ifdef TURBO   
	mmaStatus = _BL;
#else
   asm mov mmaStatus, bl
#endif

	asm		push	ds
	asm		push	es

	asm 	mov		ax, ds
	asm 	mov		es, ax

	if (mmaStatus == 0x00) {			// As specified in dispatch.c
   	status = (BYTE)GetOPL3TimerIntStatus();

		if (status & 0x02) {
			/*
		 	*  OPL3 Timer 1 interrupted
		 	*/
			ExecServiceGeneric(OPL3Timer1);
			}

   	if (status & 0x01) {
			/*
		 	*  OPL3 Timer 2 interrupted
		 	*/
			ExecServiceGeneric(OPL3Timer2);
			}
		}


	if (mmaStatus & 0x10) {
		/*
	 	 *  MMA Timer 0 interrupted
	 	 */
		ExecServiceGeneric(MMATimer0);
		}

	if (mmaStatus & 0x20) {
		/*
		 *  MMA Timer 1 interrupted
		 */
		ExecServiceGeneric(MMATimer1);
		}
	if (mmaStatus & 0x40) {
		/*
		 *  MMA Timer 2 interrupted
		 */
		ExecServiceGeneric(MMATimer2);
		}

	asm	pop		es
	asm	pop		ds
	}


/*
 * Synopsis:	    ReadOPL3Register(BYTE regis, BYTE alReg)
 *					ReadMMARegister(BYTE regis, BYTE alReg)
 *
 * Description:    This routine will returns the register value indexed by
 *					the register number and the AL status (L or H).
 *
 *	Argument:		BYTE regis
 *
 *					BYTE alReg
 *
 * Return Value:	Value at this position
 *
 */

PRIVATE
BYTE   ReadOPL3Register(BYTE regis, BYTE alReg)
   {
	BYTE	retVal;

	asm	pushf
	asm	cli
	outportb(OPL3ChipIO, regis + alReg);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	retVal = inportb(OPL3ChipIO);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	asm	popf

	return(retVal);
   }

PRIVATE
BYTE	ReadMMARegister(BYTE regis, BYTE alReg)
   {
	BYTE	retVal;

	asm	pushf
	asm	cli
	outportb(MMAChipIO, regis + alReg);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	retVal = inportb(MMAChipIO);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	asm	popf

	return(retVal);
   }

/*
 * Synopsis:		WriteOPL3Register(BYTE regis, BYTE alReg, BYTE value)
 *					WriteMMARegister(BYTE regis, BYTE alReg, BYTE value)
 *
 * Description:    This routine will write the value in the register indexed 
 *					by the regis number and the alReg status (L or H).
 *
 * Return Value:	No returned value.
 *
 */

PRIVATE
void   WriteOPL3Register(BYTE regis, BYTE alReg, BYTE value)
   {
	asm	pushf
	asm	cli
	outportb(OPL3ChipIO, regis + alReg);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	outportb(OPL3ChipIO + 1, value);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	asm	popf
   }

PRIVATE
void   WriteMMARegister(BYTE regis, BYTE alReg, BYTE value)
   {
	asm	pushf
	asm	cli
	outportb(MMAChipIO, regis + alReg);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	outportb(MMAChipIO +1, value);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	asm	popf
   }

/*
 * Synopsis:       WORD	InitTimerDriver()
 *
 * Description:    This procedure initialize the timeUse structure with
 *                 default (not in use) values and their physical limits. 
 *                 This procedure should be used the first time the driver is 
 *                 called.
 *
 *	Argument:		no argument
 *
 * Return Value:	TIMER_NO_ERROR		
 *					TIMER_FUNCTION_ERROR
 *
 */

PUBLIC
WORD	InitTimerDriver()
   {
   int 	i;
	WORD	base;

	OPL3ChipIO 	= base = CtGetRelocationAddress();
	ctrlChipIO 	= base + 2;
	MMAChipIO 	= base + 4;

   SetDriverCallback(ADLIB_TIMER_DRIVER_ID, TimerDrvIntEntry);

	/*
	 *	Put invalid values in those variables
	 */

	eventSetMode = 0xFF;
	eventSetTimer = 0xFF;

	/*
	 *	First make sure default images 0's are really in the registers
	 */

	WriteOPL3Register(2, 0, 0x00);
	WriteOPL3Register(3, 0, 0x00);
	WriteOPL3Register(4, 0, 0x00);
	WriteOPL3Register(4, 0, 0x80);
		
   /*
    *  Register image all start with unknown values of zeros.
    */

   OPL3RegisterImage = 0x00;
   MMARegisterImage[MMAImageTimers] = 0x00;
   MMARegisterImage[MMAImageCommand] = 0x00;

   for (i = 0 ; i < OPL3TimerNbr + MMATimerNbr ; i++) {
       timeUse[i].bInUse = FALSE;
       timeUse[i].function = DoNothing;
       timerDivider[i] = 0x0000;
       timeUse[i].oneShotEvent = NO_TIME_EVENT;
		}

   /*
    *  Individual timer  structure initial Registration.
	 *	The minimum period is multipllied by 1000 for more presision
    */

   timeUse[OPL3Timer1].lPeriodMin = 79968;
   timeUse[OPL3Timer1].lPeriodMax = 20471; 

   timeUse[OPL3Timer2].lPeriodMin = 319873;
   timeUse[OPL3Timer2].lPeriodMax = 81887;

   timeUse[MMATimer0].lPeriodMin = 1890;
   timeUse[MMATimer0].lPeriodMax = 123839;

   timeUse[MMATimer1].lPeriodMin = 1890;
   timeUse[MMATimer1].lPeriodMax = 123839;

   timeUse[MMATimer2].lPeriodMin = 1890;
   timeUse[MMATimer2].lPeriodMax = 507246000;

   timeUse[MMABaseCounter].lPeriodMin = 1890;
   timeUse[MMABaseCounter].lPeriodMax = 7739;

	/*
	 *	Some more protection just in case...
	 */

	DisableOPL3Timer1();	StopOPL3Timer1();
	DisableOPL3Timer2();	StopOPL3Timer2();
	DisableMMATimer0();	    StopMMATimer0();
	DisableMMATimer1();	    StopMMATimer1();
	DisableMMATimer2();	    StopMMATimer2();

	ResetOPL3LastTimerInt();

	return TIMER_NO_ERROR;
	}


WORD
CloseTimerDriver(void)

	{
   ResetDriverCallback(ADLIB_TIMER_DRIVER_ID);
   return(0);
	}


/* 
 * Synopsis:       WORD	AllocateTimer(int index)
 *                 WORD	FreeTimer(int index)
 *
 * Description:    This procedure will reserve and from then denied any 
 *                 external application access to this timer. The application
 *                 should free the timer after use. This service routine
 *                 is used by all timers as the main code of their own 
 *                 allocation routines.
 *
 * Arguments:      int index   : which timer ot allocate or free.
 *
 * Return Value:   WORD    true or false if operation succeed.
 *
 */

PRIVATE
WORD   AllocateTimer(int index)
   {
   if (timeUse[index].bInUse == TRUE) return FALSE;	/* Error case	*/
   else {
       timeUse[index].bInUse = TRUE;
       return TRUE;
       }
   }

PRIVATE
WORD   FreeTimer(int index)
   {
   if (timeUse[index].bInUse == TRUE) {
       timeUse[index].bInUse = FALSE;	
		return TRUE;
		}
   else {
       return FALSE;					/* Error case	*/
       }
   }

/* 
 * Synopsis:		WORD	AllocateOPL3Timer1()
 *					WORD	FreeOPL3Timer1()
 *					WORD	AllocateOPL3Timer2()
 *					WORD	FreeOPL3Timer2()
 *					WORD	AllocateMMATimer0()
 *					WORD	FreeMMATimer0()
 *					WORD	AllocateMMATimer1()
 *					WORD	FreeMMATimer1()
 *					WORD	AllocateMMATimer2()
 *					WORD	FreeMMATimer2()
 *					WORD	AllocateMMABaseCounter()
 *					WORD	FreeMMABaseCounter()
 *
 * Description:    This procedure will reserve and from then denied any 
 *                 external application access to this timer. The application
 *                 should free the timer after use.
 *
 * Return Value:   WORD    true or false if operation succeed.
 *
 */

PUBLIC
WORD   AllocateOPL3Timer1()
   {
	WORD result;
   result = AllocateTimer(OPL3Timer1);

   /*
    *  if allocated set a bunch of default before returning
    */

   if (result) {
   	RestoreOPL3Timer1IntService();
       }

   return(result);
   }

PUBLIC
WORD   FreeOPL3Timer1()
   {
   return FreeTimer(OPL3Timer1);
   }

PUBLIC
WORD   AllocateOPL3Timer2()
   {
	WORD result;

	result = AllocateTimer(OPL3Timer2);
   /*
    *  if allocated set a bunch of default before returning
    */

   if (result) {
   	RestoreOPL3Timer2IntService();
       }

   return(result);
   }

PUBLIC
WORD   FreeOPL3Timer2()
   {
   return FreeTimer(OPL3Timer2);
   }

PUBLIC
WORD   AllocateMMATimer0()
   {
   WORD result;

	result = AllocateTimer(MMATimer0);
   /*
    *  if allocated set a bunch of default before returning
    */

   if (result) {
   	RestoreMMATimer0IntService();
       }

   return(result);
   }

PUBLIC
WORD   FreeMMATimer0()
   {
   return FreeTimer(MMATimer0);
   }

PUBLIC
WORD   AllocateMMATimer1()
   {
   WORD result;
   result = AllocateTimer(MMATimer1);

   /*
    *  if allocated set a bunch of default before returning
    */

   if (result) {
   	RestoreMMATimer1IntService();
       }

   return(result);
   }

PUBLIC
WORD   FreeMMATimer1()
   {
   return FreeTimer(MMATimer1);
   }

PUBLIC
WORD   AllocateMMATimer2()
   {
   WORD result;
   result = AllocateTimer(MMATimer2);

   /*
    *  if allocated set a bunch of default before returning
    */

   if (result) {
   	RestoreMMATimer2IntService();
       }

   return(result);
   }

PUBLIC
WORD   FreeMMATimer2()
   {
   return FreeTimer(MMATimer2);
   }

PUBLIC
WORD   AllocateMMABaseCounter()
   {
   return AllocateTimer(MMABaseCounter);
   }

PUBLIC
WORD   FreeMMABaseCounter()
   {
   return FreeTimer(MMABaseCounter);
   }

/* 
 * Synopsis:       WORD	EnableOPL3Timer1()
 * 				WORD	DisableOPL3Timer1()
 * 				WORD	EnableOPL3Timer2()
 * 				WORD	DisableOPL3Timer2()
 * 				WORD	EnableMMATimer0()
 * 				WORD	DisableMMATimer0()
 * 				WORD	EnableMMATimer1()
 * 				WORD	DisableMMATimer1()
 * 				WORD	EnableMMATimer2()
 * 				WORD	DisableMMATimer2()
 *
 * Description:    This will set or reset the mask bit associated with
 *                 this timer interrupt enable/disable.
 *
 * Return Value:	TIMER_NO_ERROR		
 *					TIMER_FUNCTION_ERROR
 *
 */

PUBLIC
WORD	EnableOPL3Timer1()
   {
	if (gssLevel == level1) return TIMER_FUNCTION_ERROR;

   OPL3RegisterImage &= 0xBF;   /* Set bit 6 to 0   */
   WriteOPL3Register(OPL3CommandRegister, 0x00, OPL3RegisterImage);
	return TIMER_NO_ERROR;
   }

PUBLIC
WORD   DisableOPL3Timer1()
   {
   OPL3RegisterImage |= 0x40;  /* Set bit 6 to 1   */
   WriteOPL3Register(OPL3CommandRegister, 0x00, OPL3RegisterImage);
	return TIMER_NO_ERROR;
   }

PUBLIC
WORD   EnableOPL3Timer2()
   {
	if (gssLevel == level1) return TIMER_FUNCTION_ERROR;

   OPL3RegisterImage &= 0xDF;   /* Set bit 5 to 0   */
   WriteOPL3Register(OPL3CommandRegister, 0x00, OPL3RegisterImage);
	return TIMER_NO_ERROR;
   }

PUBLIC
WORD   DisableOPL3Timer2()
   {
   OPL3RegisterImage |= 0x20;  /* Set bit 5 to 1   */
   WriteOPL3Register(OPL3CommandRegister, 0x00, OPL3RegisterImage);
	return TIMER_NO_ERROR;
   }

PUBLIC
WORD   EnableMMATimer0()
   {
   MMARegisterImage[MMAImageCommand] &= 0xEF;  /* Set bit 4 to 0   */
   WriteMMARegister(MMACommandRegister, 0x00, MMARegisterImage[MMAImageCommand]);
	return TIMER_NO_ERROR;
   }

PUBLIC
WORD   DisableMMATimer0()
   {
   MMARegisterImage[MMAImageCommand] |= 0x10;  /* Set bit 4 to 1   */
   WriteMMARegister(MMACommandRegister, 0x00, MMARegisterImage[MMAImageCommand]);
	return TIMER_NO_ERROR;
   }

PUBLIC
WORD   EnableMMATimer1()
   {
   MMARegisterImage[MMAImageCommand] &= 0xDF;  /* Set bit 4 to 0   */
   WriteMMARegister(MMACommandRegister, 0x00, MMARegisterImage[MMAImageCommand]);
	return TIMER_NO_ERROR;
   }

PUBLIC
WORD   DisableMMATimer1()
   {
   MMARegisterImage[MMAImageCommand] |= 0x20;  /* Set bit 4 to 1   */
   WriteMMARegister(MMACommandRegister, 0x00, MMARegisterImage[MMAImageCommand]);
	return TIMER_NO_ERROR;
   }

PUBLIC
WORD   EnableMMATimer2()
   {
   MMARegisterImage[MMAImageCommand] &= 0xBF;  /* Set bit 5 to 0   */
   WriteMMARegister(MMACommandRegister, 0x00, MMARegisterImage[MMAImageCommand]);
	return TIMER_NO_ERROR;
   }

PUBLIC
WORD   DisableMMATimer2()
   {
   MMARegisterImage[MMAImageCommand] |= 0x40;  /* Set bit 5 to 1   */
   WriteMMARegister(MMACommandRegister, 0x00, MMARegisterImage[MMAImageCommand]);
	return TIMER_NO_ERROR;
   }

/* 
 * Synopsis:		WORD	LoadStartOPL3Timer1()
 * 				WORD	StopOPL3Timer1()
 * 				WORD	LoadStartOPL3Timer2()
 * 				WORD	StopOPL3Timer2()
 * 				WORD	LoadStartMMATimer0()
 * 				WORD	StopMMATimer0()
 * 				WORD	LoadStartMMATimer1()
 * 				WORD	StopMMATimer1()
 * 				WORD	LoadStartMMATimer2()
 * 				WORD	StopMMATimer2()
 * 				WORD	LoadStartMMABaseCounter()
 * 				WORD	StopMMABaseCounter()
 *
 * Description:    This will load the physical counter with the count 
 *                 associated and start the counter. 
 *
 * Return Value:	TIMER_NO_ERROR		
 *					TIMER_FUNCTION_ERROR
 *
 */

PUBLIC
WORD   LoadStartOPL3Timer1()
   {
   OPL3RegisterImage |= 0x01;  /* Set bit 0 to 1   */
   WriteOPL3Register(OPL3CommandRegister, 0x00, OPL3RegisterImage);
	return TIMER_NO_ERROR;
   }

PUBLIC
WORD   StopOPL3Timer1()
   {
   OPL3RegisterImage &= 0xFE;  /* Set bit 0 to 0   */
   WriteOPL3Register(OPL3CommandRegister, 0x00, OPL3RegisterImage);
	return TIMER_NO_ERROR;
   }

PUBLIC
WORD   LoadStartOPL3Timer2()
   {
   OPL3RegisterImage |= 0x02;  /* Set bit 1 to 1   */
   WriteOPL3Register(OPL3CommandRegister, 0x00, OPL3RegisterImage);
	return TIMER_NO_ERROR;
   }

PUBLIC
WORD   StopOPL3Timer2()
   {
   OPL3RegisterImage &= 0xFD;  /* Set bit 1 to 0   */
   WriteOPL3Register(OPL3CommandRegister, 0x00, OPL3RegisterImage);
	return TIMER_NO_ERROR;
   }

PUBLIC
WORD   LoadStartMMATimer0()
   {
   MMARegisterImage[MMAImageCommand] |= 0x01;  /* Set bit 0 to 1   */
   WriteMMARegister(MMACommandRegister, 0x00, MMARegisterImage[MMAImageCommand]);
	return TIMER_NO_ERROR;
   }

PUBLIC
WORD   StopMMATimer0()
   {
   MMARegisterImage[MMAImageCommand] &= 0xFE;  /* Set bit 0 to 0   */
   WriteMMARegister(MMACommandRegister, 0x00, MMARegisterImage[MMAImageCommand]);
	return TIMER_NO_ERROR;
   }

PUBLIC
WORD   LoadStartMMATimer1()
   {
   MMARegisterImage[MMAImageCommand] |= 0x02;  /* Set bit 1 to 1   */
   WriteMMARegister(MMACommandRegister, 0x00, MMARegisterImage[MMAImageCommand]);
	return(LoadStartMMABaseCounter());
   }

PUBLIC
WORD   StopMMATimer1()
   {
   MMARegisterImage[MMAImageCommand] &= 0xFD;  /* Set bit 1 to 0   */
   WriteMMARegister(MMACommandRegister, 0x00, MMARegisterImage[MMAImageCommand]);
	return(StopMMABaseCounter());
   }

PUBLIC
WORD   LoadStartMMATimer2()
   {
   MMARegisterImage[MMAImageCommand] |= 0x04;  /* Set bit 2 to 1   */
   WriteMMARegister(MMACommandRegister, 0x00, MMARegisterImage[MMAImageCommand]);
	return(LoadStartMMABaseCounter());
   }

PUBLIC
WORD   StopMMATimer2()
   {
   MMARegisterImage[MMAImageCommand] &= 0xFB;  /* Set bit 2 to 0   */
   WriteMMARegister(MMACommandRegister, 0x00, MMARegisterImage[MMAImageCommand]);
	return(StopMMABaseCounter());
   }

PUBLIC
WORD   LoadStartMMABaseCounter()
   {
   MMARegisterImage[MMAImageCommand] |= 0x08;  /* Set bit 3 to 1   */
   WriteMMARegister(MMACommandRegister, 0x00, MMARegisterImage[MMAImageCommand]);
	return TIMER_NO_ERROR;
   }

PUBLIC
WORD   StopMMABaseCounter()
   {
   MMARegisterImage[MMAImageCommand] &= 0xF7;  /* Set bit 3 to 0   */
   WriteMMARegister(MMACommandRegister, 0x00, MMARegisterImage[MMAImageCommand]);
	return TIMER_NO_ERROR;
   }

/*
 *	Synopsis:		WORD	AssignOPL3Timer1IntService(CallbackPtr function);
 *					WORD	AssignOPL3Timer2IntService(CallbackPtr function);
 *					WORD	AssignMMATimer0IntService(CallbackPtr function);
 *					WORD	AssignMMATimer1IntService(CallbackPtr function);
 *					WORD	AssignMMATimer2IntService(CallbackPtr function);
 *
 *	Description:	The function pointer passed as argument will be executed
 *					every time an interrupt is generated by this timer.
 *
 *	Argument:		CallbackPtr function:	pointer to the function
 *
 * Return Value:	TIMER_NO_ERROR		
 *					TIMER_FUNCTION_ERROR
 */

WORD	AssignOPL3Timer1IntService(CallbackPtr function)
	{
	timeUse[OPL3Timer1].function = function;
	return TIMER_NO_ERROR;
	}


WORD	AssignOPL3Timer2IntService(CallbackPtr function)
	{
	timeUse[OPL3Timer2].function = function;
	return TIMER_NO_ERROR;
	}


WORD	AssignMMATimer0IntService(CallbackPtr function)
	{
	timeUse[MMATimer0].function = function;
	return TIMER_NO_ERROR;
	}


WORD	AssignMMATimer1IntService(CallbackPtr function)
	{
	timeUse[MMATimer1].function = function;
	return TIMER_NO_ERROR;
	}

WORD	AssignMMATimer2IntService(CallbackPtr function)
	{
	timeUse[MMATimer2].function = function;
	return TIMER_NO_ERROR;
	}

/*
 *	Synopsis:		WORD	RestoreOPL3Timer1IntService()
 *					WORD	RestoreOPL3Timer2IntService()
 *					WORD	RestoreMMATimer0IntService()
 *					WORD	RestoreMMATimer1IntService()
 *					WORD	RestoreMMATimer2IntService()
 *
 *	Description:	This routine is used to restore  the default processing to
 *					any timer interrupt service. (DoNothing...)
 *
 * Return Value:	TIMER_NO_ERROR		
 *					TIMER_FUNCTION_ERROR
 */

PUBLIC
WORD	RestoreOPL3Timer1IntService()
	{
asm pushf
asm cli
	timeUse[OPL3Timer1].function = DoNothing;
asm popf
	return TIMER_NO_ERROR;
	}

PUBLIC
WORD	RestoreOPL3Timer2IntService()
	{
asm pushf
asm cli
	timeUse[OPL3Timer2].function = DoNothing;
asm popf
	return TIMER_NO_ERROR;
	}

PUBLIC
WORD	RestoreMMATimer0IntService()
	{
asm pushf
asm cli
	timeUse[MMATimer0].function = DoNothing;
asm popf
	return TIMER_NO_ERROR;
	}

PUBLIC
WORD	RestoreMMATimer1IntService()
	{
asm pushf
asm cli
	timeUse[MMATimer1].function = DoNothing;
asm popf
	return TIMER_NO_ERROR;
	}

PUBLIC
WORD	RestoreMMATimer2IntService()
	{
asm pushf
asm cli
	timeUse[MMATimer2].function = DoNothing;
asm popf
	return TIMER_NO_ERROR;
	}

PRIVATE
void		ExecServiceGeneric(BYTE timer)
	{
	(*(timeUse[timer].function))();

	if ((eventSetTimer == timer) AND (eventSetMode == TIME_ONESHOT)) {
		ResetAvailableTimerEvent();
		}
	if (timeUse[timer].oneShotEvent == TIME_ONESHOT) ResetTimerEvent(timer);
	}

/* 
 * Synopsis:		WORD	SetOPL3Timer1Counter(BYTE wPeriod)
 *
 * Description:	Set the OPL3 timer1 with the current value. Base clock is
 * 				79.9682 us.
 *
 * Return Value:	TIMER_NO_ERROR		
 *					TIMER_FUNCTION_ERROR
 *
 */

PUBLIC
WORD   SetOPL3Timer1Counter(BYTE wPeriod)
   {
   WriteOPL3Register(OPL3Timer1Register, 0x00, wPeriod);
   timerDivider[OPL3Timer1] = wPeriod;
	return TIMER_NO_ERROR;
   }

/* 
 * Synopsis:		WORD	SetOPL3Timer2Counter(BYTE wPeriod)
 *
 * Description:    Set the OPL3 timer2 with the current value. Base clock is 
 *                 319.873 us.    
 *
 * Return Value:   No returned value.
 *
 */

PUBLIC
WORD	SetOPL3Timer2Counter(BYTE wPeriod)
   {
   WriteOPL3Register(OPL3Timer2Register, 0x00, wPeriod);
   timerDivider[OPL3Timer2] = wPeriod;
	return TIMER_NO_ERROR;
   }

/* 
 * Synopsis:       WORD	SetMMATimer0Counter(WORD wPeriod)
 *
 * Description:    Set the MMA timer0 with the current 16 bits value. 
 *
 * Return Value:	TIMER_NO_ERROR		
 *					TIMER_FUNCTION_ERROR
 *
 */

PUBLIC
WORD   SetMMATimer0Counter(WORD wPeriod)
   {
   WriteMMARegister(MMATimer0LowRegister, 0x00, (BYTE)(wPeriod % 256));
   WriteMMARegister(MMATimer0HighRegister, 0x00, (BYTE)(wPeriod / 256));
   timerDivider[MMATimer0] = wPeriod;
	return TIMER_NO_ERROR;
   }

/* 
 * Synopsis:       WORD	SetMMATimer1Counter(BYTE wPeriod)
 *
 * Description:    Set the MMA timer1 with the current 4 bits value. 
 *
 * Return Value:   No returned value.
 *
 */

PUBLIC
WORD   SetMMATimer1Counter(BYTE wPeriod)
   {
   BYTE    temp;

   wPeriod &= 0x0F;

   /*
    *  Update image first then write down new value
    */

   temp = MMARegisterImage[MMAImageTimers] & 0x0F;
   temp += (wPeriod << 4);
   MMARegisterImage[MMAImageTimers] = temp;

   WriteMMARegister(MMATimer1BCRegister, 0x00, MMARegisterImage[MMAImageTimers]);
   timerDivider[MMATimer1] = wPeriod;
	return TIMER_NO_ERROR;
   }

/* 
 * Synopsis:       WORD	SetMMATimer2Counter(WORD wPeriod)
 *
 * Description:    Set the MMA timer2 with the current 16 bits value. 
 *
 * Return Value:	TIMER_NO_ERROR		
 *					TIMER_FUNCTION_ERROR
 *
 */

PUBLIC
WORD   SetMMATimer2Counter(WORD wPeriod)
   {
   WriteMMARegister(MMATimer2LowRegister, 0x00, (BYTE)(wPeriod % 256));
   WriteMMARegister(MMATimer2HighRegister, 0x00, (BYTE)(wPeriod / 256));
   timerDivider[MMATimer2] = wPeriod;
	return TIMER_NO_ERROR;
   }

/* 
 * Synopsis:       WORD	SetMMABaseCounterCounter(WORD wPeriod)
 *
 * Description:    Set the MMA base counter with the current 12 bits value. 
 *
 * Return Value:	TIMER_NO_ERROR		
 *					TIMER_FUNCTION_ERROR
 *
 */

PUBLIC
WORD   SetMMABaseCounterCounter(WORD wPeriod)
   {
   BYTE    temp;

   wPeriod &= 0x0FFF;
   WriteMMARegister(MMABaseCounterLowRegister, 0x00, (BYTE)(wPeriod % 256));

   /*
    *  Update image first then write down new value
    */

   temp = MMARegisterImage[MMAImageTimers] & 0xF0;
   temp += (wPeriod / 256);
   MMARegisterImage[MMAImageTimers] = temp;

   WriteMMARegister(MMATimer1BCRegister, 0x00, MMARegisterImage[MMAImageTimers]);
   timerDivider[MMABaseCounter] = wPeriod;
	return TIMER_NO_ERROR;
   }

/********************************************************************/

/* 
 * Synopsis:       WORD	ResetOPL3LastTimerInt()
 *
 * Description:    This will reset the /IRQ signal generated by timers 1 and 2
 *                 RST=1 sets /IRQ=H
 *
 * Return Value:	TIMER_NO_ERROR		
 *					TIMER_FUNCTION_ERROR
 *
 */

PUBLIC
WORD   ResetOPL3LastTimerInt()
   {
   WriteOPL3Register(OPL3CommandRegister, 0x00, 0x80);
	return TIMER_NO_ERROR;
   }

/* 
 * Synopsis:       WORD	GetOPL3TimerIntStatus()
 *
 * Description:    This will returned 0 if no timer has interrupted, 2 if 
 *                 timer 1 did, 1 if timer 2 did, 3 if both did...
 *
 * Return Value:   BYTE    0, 1, 2.
 *
 */

PUBLIC
WORD	GetOPL3TimerIntStatus()
   {
   BYTE    retVal;

   /* 
    *  First check if proper initialisation has been done.
    */

   if (OPL3ChipIO == 0x0000) return 0; 

   retVal = inportb(OPL3ChipIO);
   retVal &= 0x60;
   retVal >>= 5;
   return (WORD)retVal;
   }

/* 
 * Synopsis:       WORD	GetMMATimerIntStatus()
 *
 * Description:    This will returned 0 if no timer has interrupted, 1 if 
 *                 timer 0 did, 2 if timer 1 did and 4 if timer 2 did.
 *
 *					The MMA chip has a special behavior: it will reset the 
 *					interrupt bit after a staus register lecture...
 *
 * Return Value:   BYTE    0, 1, 2 or 4. or any bit combination up to 7.
 *
 */

PUBLIC
WORD	GetMMATimerIntStatus()
   {
   BYTE    retVal;

   /* 
    *  First check if proper initialisation has been done.
    */

   if (MMAChipIO == 0x0000) return 0; 

   retVal = inportb(MMAChipIO);
   retVal &= 0x70;
   retVal >>= 4;
   return (WORD)retVal;
   }

/* 
 * Synopsis:       WORD	GetMMATimer2Content()
 *
 * Description:    This routine returns the content of the MMA timer 2.
 *					This is the only timer that can be read. 
 *
 *	Argument:		none
 *
 * Return Value:   16 bit content of MMA timer 2
 *
 */

PUBLIC
WORD	GetMMATimer2Content()
	{
   WORD    retVal;

   /* 
    *  First check if proper initialisation has been done.
    */

   if (MMAChipIO == 0x0000) return 0; 

	asm	pushf
	asm	cli
	outportb(MMAChipIO, MMATimer2LowRegister);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
  retVal = inportb(MMAChipIO + 1);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	outportb(MMAChipIO, MMATimer2HighRegister);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	retVal += (256 * inportb(MMAChipIO + 1));
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	
	asm	popf

   return retVal;
	}

/************************** Second level function ***************************/

/*
 * Synopsis:       WORD	SetOPL3Timer1Period(DWORD lPeriod)
 *                 WORD	SetOPL3Timer2Period(DWORD lPeriod)
 *                 WORD	SetMMATimer0Period(DWORD lPeriod)
 *                 WORD	SetMMATimer1Period(DWORD lPeriod)
 *                 WORD	SetMMATimer2Period(DWORD lPeriod)
 *                 WORD	SetMMABaseCounterPeriod(DWORD lPeriod)
 *
 * Description:    Those functions offers a different approach to timer
 *                 initialisation. Instead of specifying the divider number
 *                 the argument passed is the period (usec) associated with it.
 *
 * Return Value:	TIMER_NO_ERROR		
 *					TIMER_FUNCTION_ERROR
 */

PUBLIC
WORD   SetOPL3Timer1Period(DWORD lPeriod)
   {			 
   BYTE divider;

	divider = 256 - (BYTE)SetTimerPeriod(OPL3Timer1, lPeriod);
   SetOPL3Timer1Counter(divider);

	return TIMER_NO_ERROR;	
   }

PUBLIC
WORD   SetOPL3Timer2Period(DWORD lPeriod)
   {
   BYTE    divider;

	divider = 256 - (BYTE)SetTimerPeriod(OPL3Timer2, lPeriod);
   SetOPL3Timer2Counter(divider);

	return TIMER_NO_ERROR;
   }
       
PUBLIC
WORD   SetMMATimer0Period(DWORD lPeriod)
   {
   WORD divider;

   divider = (WORD)SetTimerPeriod(MMATimer0, lPeriod);
   SetMMATimer0Counter(divider);

	return TIMER_NO_ERROR;
   }

PUBLIC
WORD   SetMMATimer1Period(DWORD lPeriod)
   {
   WORD	divider;
	WORD	store;

   divider = SetTimerPeriod(MMATimer1, lPeriod);


	if (divider < 0x0FFF) {
		SetMMABaseCounterCounter(divider);
		SetMMATimer1Counter(1);
		}
	else {
		store = SplitDivider(divider);
		SetMMATimer1Counter((BYTE)store);
		SetMMABaseCounterCounter(divider / store);
		}


	return TIMER_NO_ERROR;
   }

PUBLIC
WORD   SetMMATimer2Period(DWORD lPeriod)
   {
   DWORD	divider;
	DWORD	store;

   divider = SetTimerPeriod(MMATimer2, lPeriod);

	if (divider < (DWORD)0x0FFF) {
		SetMMABaseCounterCounter((WORD)divider);
		SetMMATimer2Counter(1);
		}
	else {
		store = SplitDivider(divider);
		SetMMATimer2Counter((WORD)store);
		SetMMABaseCounterCounter((WORD)(divider / store));
		}

	return 0;
   }

PUBLIC
WORD   SetMMABaseCounterPeriod(DWORD lPeriod)
   {
   WORD divider;

   divider = 4096 - (WORD)SetTimerPeriod(MMABaseCounter, lPeriod);
   SetMMABaseCounterCounter(divider);
	return TIMER_NO_ERROR;
   }

/*
 * Synopsis:       DWORD	SetTimerPeriod(int timer, DWORD lPeriod)
 *
 * Description:    his routine will take a timed period in us and check
 *                 for boundary overrun. If inside limits then get the
 *                 divider associated with the corresponding timer to
 *                 get the proper period passed as argument.
 *
 * Arguments:      int timer:      which timer to use the base clock
 *                 DWORD lPeriod:   how should it take in usec
 *
 * Return Value:   the divider to place in the register
 *     
 */

PRIVATE
DWORD   SetTimerPeriod(int timer, DWORD lPeriod)
   {
   DWORD    divider;

   if (timeUse[timer].lPeriodMin == 0) return FALSE;
   
   if ((lPeriod * 1000) < timeUse[timer].lPeriodMin) {
       lPeriod = timeUse[timer].lPeriodMin;
       }
   
   if (lPeriod > timeUse[timer].lPeriodMax) {
       lPeriod = timeUse[timer].lPeriodMax;
       }

	/*
	 *	if a lPEriod very large is requested then an overflow can occur so...
	 */

	if (lPeriod < (0xFFFFFFFF / (DWORD)1000)) 
	   divider = (1000 * lPeriod) / timeUse[timer].lPeriodMin;
	else {
		divider = lPeriod / timeUse[timer].lPeriodMin;
		divider *= 1000;
		}
   return divider;
   }

/*
 * Synopsis:       WORD    GetOPL3Timer1Caps(DWORD far *lPeriodMin, DWORD far *lPeriodMax)
 *                 WORD    GetOPL3Timer2Caps(DWORD far *lPeriodMin, DWORD far *lPeriodMax)
 *                 WORD    GetMMATimer0Caps(DWORD far *lPeriodMin, DWORD far *lPeriodMax)
 *                 WORD    GetMMATimer1Caps(DWORD far *lPeriodMin, DWORD far *lPeriodMax)
 *                 WORD    GetMMATimer2Caps(DWORD far *lPeriodMin, DWORD far *lPeriodMax)
 *
 * Description:    Used by external modules to query the driver on physical
 *                 limits of each timer. It returns the minimum and maximum
 *                 period covered by the timer in micro seconds.
 *
 * Arguments:      DWORD    *lPeriodMin
 *                     smallest period possible (input clock)
 *					
 *                 DWORD    *lPeriodMax
 *                     longest period possible 
 *
 * Return Value:	TIMER_NO_ERROR		
 *					TIMER_FUNCTION_ERROR
 */

PUBLIC
WORD    GetOPL3Timer1Caps(DWORD far *lPeriodMin, DWORD far *lPeriodMax)
   {
   *lPeriodMin = (timeUse[OPL3Timer1].lPeriodMin / 1000) + 1;
   *lPeriodMax = timeUse[OPL3Timer1].lPeriodMax;
	return TIMER_NO_ERROR;
   }

PUBLIC
WORD    GetOPL3Timer2Caps(DWORD far *lPeriodMin, DWORD far *lPeriodMax)
   {
   *lPeriodMin = (timeUse[OPL3Timer2].lPeriodMin / 1000) + 1;
   *lPeriodMax = timeUse[OPL3Timer2].lPeriodMax;
	return TIMER_NO_ERROR;
   }

PUBLIC
WORD    GetMMATimer0Caps(DWORD far *lPeriodMin, DWORD far *lPeriodMax)
   {
   *lPeriodMin = (timeUse[MMATimer0].lPeriodMin / 1000) + 1;
   *lPeriodMax = timeUse[MMATimer0].lPeriodMax;
	return TIMER_NO_ERROR;
   }

PUBLIC
WORD    GetMMATimer1Caps(DWORD far *lPeriodMin, DWORD far *lPeriodMax)
   {
   *lPeriodMin = (timeUse[MMATimer1].lPeriodMin / 1000) + 1;
   *lPeriodMax = timeUse[MMATimer1].lPeriodMax;
	return TIMER_NO_ERROR;
   }

PUBLIC
WORD    GetMMATimer2Caps(DWORD far *lPeriodMin, DWORD far *lPeriodMax)
   {
   *lPeriodMin = (timeUse[MMATimer2].lPeriodMin / 1000) + 1;
   *lPeriodMax = timeUse[MMATimer2].lPeriodMax;
	return TIMER_NO_ERROR;
   }

/*
 * Synopsis:       WORD	SetAvailableTimerEvent(CallbackPtr function, 
 *									DWORD period, WORD mode)
 *
 * Description:    This high level routine will allocate a timer (if one
 *					available) that will execute the routine specified by 
 *					the argument function, at each "period". A mode must
 *					be specified to specify if the function must be called
 *					only one time or every n periods.
 *
 *	Argument:		CallbackPtr function
 *						Routine to be called when timer is finished counting
 *
 *					DWORD period
 *						period to program into the timer register
 *
 *					WORD mode
 *		   				TIME_ONESHOT
 *		   					Event occurs once, after wPeriod milliseconds.
 *
 *		   				TIME_PERIODIC
 *		   					Event occurs every wPeriod milliseconds.
 *
 *	Return values:	TIMER_NO_ERROR			
 *					TIMER_FUNCTION_ERROR	
 *					TIMER_ARGUMENT_ERROR	
 *					TIMER_NOT_AVAILABLE		
 *					TIMER_EVENT_USED
 */

PUBLIC
WORD	SetAvailableTimerEvent(CallbackPtr function, DWORD period, WORD mode)
	{
	int			i = 0;
	WORD			error;

	if (eventSetTimer != 0xFF) return TIMER_EVENT_USED;

	if ((function == NULL) OR (period <= 0) OR 
		((mode != TIME_ONESHOT) AND (mode != TIME_PERIODIC))) 
		return TIMER_ARGUMENT_ERROR;

	/*
	 *	To prevent the use of OPL3 timers in interrupt mode when in level 1
	 */

	if (gssLevel == level1) i = 2;

   for ( ; i < MMABaseCounter ; i++) {
       if ((timeUse[i].bInUse == FALSE) AND
			(period * 1000 > timeUse[i].lPeriodMin) AND
   		(period < timeUse[i].lPeriodMax)) {
			eventSetMode = mode;
			eventSetTimer = i;
			error = SetTimerEvent(i, function, period, mode);
			if (error)	return(error);

			return TIMER_NO_ERROR;
			}
		}

	return TIMER_NOT_AVAILABLE;
	}
				  
/*
 * Synopsis:       WORD	ResetAvailableTimerEvent()
 *
 * Description:    Routine to call to reset the event previously set with
 *					the routine SetAvailableTimerEvent.
 *
 *	Argument:		none
 *
 * Return Value:	TIMER_NO_ERROR		
 *					TIMER_FUNCTION_ERROR
 *					TIMER_NO_EVENT
 *					TIMER_AUTO_RESET
 */

PUBLIC
WORD	ResetAvailableTimerEvent()
	{
	WORD	error;

	if (eventSetTimer == 0xFF) return TIMER_NO_EVENT;

	if (eventSetMode == TIME_ONESHOT) return TIMER_AUTO_RESET;
	error = ResetTimerEvent(eventSetTimer);
	if (error)
		return(error);

	return TIMER_NO_ERROR;
	}

/*
 * Synopsis:       WORD	SetTimerEvent(BYTE timer, CallbackPtr function, 
 *									DWORD period, WORD mode)
 *
 * Description:    This high level routine will allocate a timer (if one
 *					available) that will execute the routine specified by 
 *					the argument function, at each "period". A mode must
 *					be specified to specify if the function must be called
 *					only one time or every n periods.
 *
 *	Argument:		BYTE timer
 *						On which timer to work
 *
 *					CallbackPtr function
 *						Routine to be called when timer is finished counting
 *
 *					DWORD period
 *						period to program into the timer register
 *
 *					WORD mode
 *		   				TIME_ONESHOT
 *		   					Event occurs once, after wPeriod milliseconds.
 *
 *		   				TIME_PERIODIC
 *		   					Event occurs every wPeriod milliseconds.
 *
 *	Return values:	TIMER_NO_ERROR			
 *					TIMER_FUNCTION_ERROR	
 *					TIMER_ARGUMENT_ERROR	
 *					TIMER_NOT_AVAILABLE		
 *					TIMER_EVENT_USED
 */

PUBLIC
WORD	SetTimerEvent(BYTE timer, CallbackPtr function, DWORD period, WORD mode)
	{
	TimerArgum	argu;

	if (gssLevel == level1 AND (timer <= OPL3Timer2))  return TIMER_FUNCTION_ERROR;

	if ((function == NULL) OR (period <= 0) OR 
		((mode != TIME_ONESHOT) AND (mode != TIME_PERIODIC))) 
		return TIMER_ARGUMENT_ERROR;

	if ((timeUse[timer].bInUse == FALSE) AND
		(period * 1000 > timeUse[timer].lPeriodMin) AND
   	(period < timeUse[timer].lPeriodMax)) {

			if (! AllocateTimer(timer)) {
				return TIMER_FUNCTION_ERROR;
				}

			switch(timer) {
				case	OPL3Timer1:
					AssignOPL3Timer1IntService( function); 
					SetOPL3Timer1Period(period);				  
					EnableOPL3Timer1();				
					LoadStartOPL3Timer1();			
					break;
				case  OPL3Timer2:
					AssignOPL3Timer2IntService( function); 
					SetOPL3Timer2Period(period);
					EnableOPL3Timer2();
					LoadStartOPL3Timer2();
					break;
   			case	MMATimer0:
					AssignMMATimer0IntService( function);
					SetMMATimer0Period(period);
					EnableMMATimer0();
					LoadStartMMATimer0();
					break;
   			case	MMATimer1:
					AssignMMATimer1IntService( function);
					SetMMATimer1Period(period);
					EnableMMATimer1();
					LoadStartMMATimer1();
					break;
   			case	MMATimer2:
					AssignMMATimer2IntService( function);
					SetMMATimer2Period(period);
					EnableMMATimer2();
					LoadStartMMATimer2();
					break;
				}

		timeUse[timer].oneShotEvent = mode;

		return TIMER_NO_ERROR;
		}

	return TIMER_NOT_AVAILABLE;
	}
				  
/*
 * Synopsis:       WORD	ResetTimerEvent(BYTE timer)
 *
 * Description:    Routine to call to reset the event previously set with
 *					the routine SetAvailableTimerEvent.
 *
 *	Argument:		BYTE timer
 *
 * Return Value:	TIMER_NO_ERROR		
 *					TIMER_FUNCTION_ERROR
 *					TIMER_NO_EVENT
 *					TIMER_AUTO_RESET
 */

PUBLIC
WORD	ResetTimerEvent(BYTE timer)
	{
	TimerArgum	argu;

	if (timeUse[timer].oneShotEvent == TIME_ONESHOT) return TIMER_AUTO_RESET;
	
	switch(timer) {
		case	OPL3Timer1:
				StopOPL3Timer1();				
				DisableOPL3Timer1();			
				RestoreOPL3Timer1IntService();			  
				FreeOPL3Timer1();				
				break;
		case  OPL3Timer2:
				StopOPL3Timer2();
				DisableOPL3Timer2();
				RestoreOPL3Timer2IntService();
				FreeOPL3Timer2();
				break;
   	case	MMATimer0:
				StopMMATimer0();
				DisableMMATimer0();
				RestoreMMATimer0IntService();
				FreeMMATimer0();
				break;
   	case	MMATimer1:
				StopMMATimer1();
				DisableMMATimer1();
				RestoreMMATimer1IntService();
				FreeMMATimer1();
				break;
   	case	MMATimer2:
				StopMMATimer2();
				DisableMMATimer2();
				RestoreMMATimer2IntService();
				FreeMMATimer2();
				break;
		}


	timeUse[timer].oneShotEvent = NO_TIME_EVENT;

	return TIMER_NO_ERROR;
	}

/****************************************************************************/



/*
 *	Synopsis:		void	far DoNothing()
 *
 *	Description:	Used as a default value for all interrupt service routines
 *
 *	Returned value: None
 *                 
 */

PRIVATE
void	far DoNothing()
	{
	asm 	mov		ax, ds
	asm 	mov		es, ax
	}

/*
 *	Synopsis:		DWORD	SplitDivider(DWORD divider)
 *
 *	Description:	Sets the divider to place in the base counter and
 *					the destination timer.
 *
 *	Arguments:		DWORD divider
 *						divider to split between the base counter and
 *						the destination timer.
 *
 *	Returned value: The divider to place in MMA timer 1 or 2
 *                 
 */

PRIVATE
DWORD	SplitDivider(DWORD divider)
	{
	DWORD	a = 1;

	if (divider > (DWORD)0x0FFF) a = (divider / 0x0FFF);
	/*
	 *	This way of finding the right values takes quite a while for the
	 *	MMA timer 2 in the big range
	 */

	while ((divider / a) > (DWORD)0x0FFF) a++;

	return(a);
	}



