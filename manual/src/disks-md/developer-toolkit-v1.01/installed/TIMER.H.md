# `developer-toolkit-v1.01/installed/TIMER.H`

> UTF-8 rendering of a DOS-encoded (CP437 / CRLF) file. Byte-for-byte original: [`TIMER.H`](../../../disks/developer-toolkit-v1.01/installed/TIMER.H).

```c

#ifndef    _TIMERDRV_H
#define    _TIMERDRV_H

/***************************************************************************
	                              Definitions
***************************************************************************/

/*
 *	When setting an event those definitions set the mode. ONESHOT will be 
 *	executed only one time. PERIODIC will be repeat periodically until the
 *	application calls the routine ResetTimerEvent()
 */

enum timerEventType {
	TIME_ONESHOT,
	TIME_PERIODIC,
	NO_TIME_EVENT
	};

/*
 * fix names for all timers
 */

enum TimerName {
	OPL3Timer1,
   OPL3Timer2,
   MMATimer0,
   MMATimer1,
   MMATimer2,
   MMABaseCounter,
   };

/*
 *	List of services offered thru the "TimerDrvService" routine
 */


/*
 *	Values returned by the timer driver set of routines
 */

enum	TimerErrorList {
	// Timer drivers functions ERROR codes

	TIMER_NO_ERROR,				// no error
	TIMER_FUNCTION_ERROR,		// reported by the almost all functions

	// WORD SetTimerEvent ERROR codes

	TIMER_ARGUMENT_ERROR,		// invalid input arguments 
	TIMER_NOT_AVAILABLE,		// no timer available for scheduling
	TIMER_EVENT_USED,			// another event is already posted 

	// WORD ResetTimerEvent ERROR codes

	TIMER_NO_EVENT,				// won't reset because no event was scheduled 
	TIMER_AUTO_RESET,		   	// the interrupt will reset the event
	};

/****************************************************************************
								Structure
*****************************************************************************/

/****************************************************************************
								PUBLIC functions  
*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif
	CallbackProc	TimerDrvIntEntry(void);
	WORD	InitTimerDriver(void);
	WORD	CloseTimerDriver(void);
	WORD	AllocateOPL3Timer1(void);			
	WORD	FreeOPL3Timer1(void);				
	WORD	AllocateOPL3Timer2(void);
	WORD	FreeOPL3Timer2(void);
	WORD	AllocateMMATimer0(void);
	WORD	FreeMMATimer0(void);
	WORD	AllocateMMATimer1(void);
	WORD	FreeMMATimer1(void);
	WORD	AllocateMMATimer2(void);
	WORD	FreeMMATimer2(void);
	WORD	AllocateMMABaseCounter(void);
	WORD	FreeMMABaseCounter(void);
	WORD	EnableOPL3Timer1(void);				
	WORD	DisableOPL3Timer1(void);			
	WORD	EnableOPL3Timer2(void);
	WORD	DisableOPL3Timer2(void);
	WORD	EnableMMATimer0(void);
	WORD	DisableMMATimer0(void);
	WORD	EnableMMATimer1(void);
	WORD	DisableMMATimer1(void);
	WORD	EnableMMATimer2(void);
	WORD	DisableMMATimer2(void);
	WORD	LoadStartOPL3Timer1(void);			
	WORD	StopOPL3Timer1(void);				
	WORD	LoadStartOPL3Timer2(void);
	WORD	StopOPL3Timer2(void);
	WORD	LoadStartMMATimer0(void);
	WORD	StopMMATimer0(void);
	WORD	LoadStartMMATimer1(void);
	WORD	StopMMATimer1(void);
	WORD	LoadStartMMATimer2(void);
	WORD	StopMMATimer2(void);
	WORD	LoadStartMMABaseCounter(void);
	WORD	StopMMABaseCounter(void);
	WORD	AssignOPL3Timer1IntService(CallbackPtr function); 
	WORD	AssignOPL3Timer2IntService(CallbackPtr function);
	WORD	AssignMMATimer0IntService(CallbackPtr function);
	WORD	AssignMMATimer1IntService(CallbackPtr function);
	WORD	AssignMMATimer2IntService(CallbackPtr function);
	WORD	RestoreOPL3Timer1IntService(void);
	WORD	RestoreOPL3Timer2IntService(void);
	WORD	RestoreMMATimer0IntService(void);
	WORD	RestoreMMATimer1IntService(void);
	WORD	RestoreMMATimer2IntService(void);
	WORD	SetOPL3Timer1Counter(BYTE wPeriod);				  
	WORD	SetOPL3Timer2Counter(BYTE wPeriod);
	WORD	SetMMATimer0Counter(WORD wPeriod);
	WORD	SetMMATimer1Counter(BYTE wPeriod);
	WORD	SetMMATimer2Counter(WORD wPeriod);
	WORD	SetMMABaseCounterCounter(WORD wPeriod);
	WORD	ResetOPL3LastTimerInt(void);				  
	WORD	GetOPL3TimerIntStatus(void);				  
	WORD	GetMMATimerIntStatus(void);				  
	WORD	GetMMATimer2Content(void);
	WORD	SetOPL3Timer1Period(DWORD lPeriod);				  
	WORD	SetOPL3Timer2Period(DWORD lPeriod);
	WORD	SetMMATimer0Period(DWORD lPeriod);
	WORD	SetMMATimer1Period(DWORD lPeriod);
	WORD	SetMMATimer2Period(DWORD lPeriod);
	WORD	SetMMABaseCounterPeriod(DWORD lPeriod);
	WORD	GetOPL3Timer1Caps(DWORD far *lPeriodMin, DWORD far *lPeriodMax);  
	WORD	GetOPL3Timer2Caps(DWORD far *lPeriodMin, DWORD far *lPeriodMax);
	WORD	GetMMATimer0Caps(DWORD far *lPeriodMin, DWORD far *lPeriodMax);
	WORD	GetMMATimer1Caps(DWORD far *lPeriodMin, DWORD far *lPeriodMax);
	WORD	GetMMATimer2Caps(DWORD far *lPeriodMin, DWORD far *lPeriodMax);
	WORD	SetAvailableTimerEvent(CallbackPtr function, DWORD period, WORD mode);
	WORD	ResetAvailableTimerEvent(void);
	WORD	SetTimerEvent(BYTE timer, CallbackPtr function, DWORD period, WORD mode);
	WORD	ResetTimerEvent(BYTE timer);
#ifdef __cplusplus
	};
#endif


#endif






```
