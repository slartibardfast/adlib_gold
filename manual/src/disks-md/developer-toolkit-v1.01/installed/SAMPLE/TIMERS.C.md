# `developer-toolkit-v1.01/installed/SAMPLE/TIMERS.C`

> UTF-8 rendering of a DOS-encoded (CP437 / CRLF) file. Byte-for-byte original: [`TIMERS.C`](../../../../disks/developer-toolkit-v1.01/installed/SAMPLE/TIMERS.C).

```c

#ifdef TURBO
#pragma	hdrfile	conc.sym
#endif

#include	<stdio.h>
#include   <stdlib.h>
#include	<dos.h>

#include   "global.H"
#include   "Control.H"
#include   "timer.H"

#ifdef TURBO
#pragma	hdrstop

#undef inportb
#undef outportb
#endif

void			TestConcurrency(void);
void			TestSpecificEvent(void);
void			TestAvailableEvent(void);
CallbackProc   IntService1();
CallbackProc   IntService2();
CallbackProc   IntService3();
CallbackProc   IntService4();
CallbackProc   IntService5();
long 			TickCount();
void			WaitNextSecond();


volatile long	myCounter1 = 0;
volatile long	myCounter2 = 0;
volatile long	myCounter3 = 0;
volatile long	myCounter4 = 0;
volatile long	myCounter5 = 0;



/*
 * Synopsis:       Main.C
 *
 * Description:    This main loop program will first attempt to access the 
 *                 Gold Card then the MMA.
 *
 */

void	main()
	{

	/*
	 *	Get address of the card then tell the driver where to  locate it
	 */

	if (InitControlDriver()) {
		printf("Can not link to control chip driver\n");
		exit(1);
		}

	printf("Example program running level %d\n", gssLevel);

	if (InitTimerDriver()) {
		printf("Can not init timer driver\n");
		exit(1);
		}

	printf("All 3 tests will be executed with a 10 msec period for 5 seconds\n");
	printf("Each interrupt increments a counter that will be printed as the result\n");

	printf("\nThe first test is going to use the 5 timers at the same time\n");
	printf("Please wait.\n");
	TestConcurrency();

	printf("\nThe second test is going to use the 5 timers at the same time\n");
	printf("but using a more higher level of function call: SetTimerEvent()\n");
	printf("Please wait.\n");

	TestSpecificEvent();

	printf("\nThe third test will ask a timer to be associated with an event\n");
	printf("The specific timer used is based on availability and period limits\n");
	printf("Please wait.\n");
	TestAvailableEvent();

	printf("Tests done.\nClosing all interrupt vectors\n");

	CloseTimerDriver();
	CloseControlDriver();
	printf("Goodbye!\n");
   }

/************************** Timer Driver Testing section *********************/


void	TestConcurrency()
	{
	long	end;
	long	period;

	myCounter1 = 0;
	myCounter2 = 0;
	myCounter3 = 0;
	myCounter4 = 0;
	myCounter5 = 0;

	period = 10000;

	if (gssLevel == level2) {
		if (! AllocateOPL3Timer1()) {
			printf("OPL3 timer 1 is not available...\n");
			exit(1);
			}

		AssignOPL3Timer1IntService(IntService1);

 		SetOPL3Timer1Period(period);
		EnableOPL3Timer1();

		if (! AllocateOPL3Timer2()) {
			printf("OPL3 timer 2 is not available...\n");
			exit(1);
			}
		AssignOPL3Timer2IntService(IntService2);
 		SetOPL3Timer2Period(period);
		EnableOPL3Timer2();
		}

	if (! AllocateMMATimer0()) {
		printf("MMA timer 0 is not available...\n");
		exit(1);
		}

	AssignMMATimer0IntService(IntService3);
 	SetMMATimer0Period(period);
	EnableMMATimer0();

	if (! AllocateMMATimer1()) {
		printf("MMA timer 1 is not available...\n");
		exit(1);
		}
	AssignMMATimer1IntService(IntService4);
 	SetMMATimer1Period(period);
	EnableMMATimer1();

	if (! AllocateMMATimer2()) {
		printf("MMA timer 2 is not available...\n");
		exit(1);
		}
	AssignMMATimer2IntService(IntService5);
 	SetMMATimer2Period(period);
	EnableMMATimer2();

	if (gssLevel == level2) {
		LoadStartOPL3Timer1();
		LoadStartOPL3Timer2();
		}

	LoadStartMMATimer0();
	LoadStartMMATimer1();
	LoadStartMMATimer2();

	WaitNextSecond();
	WaitNextSecond();
	WaitNextSecond();
	WaitNextSecond();
	WaitNextSecond();

	if (gssLevel == level2) {
		StopOPL3Timer1();
		DisableOPL3Timer1();

		StopOPL3Timer2();
		DisableOPL3Timer2();
		}

	StopMMATimer0();
	DisableMMATimer0();

	StopMMATimer1();
	DisableMMATimer1();

	StopMMATimer2();
	DisableMMATimer2();

	if (gssLevel == level2) {
   	if (GetOPL3TimerIntStatus()) {
			ResetOPL3LastTimerInt();
			}

		RestoreOPL3Timer1IntService();
		if (! FreeOPL3Timer1()) {
			printf("OPL3 timer 1 is not allocated...\n");
			exit(1);
			}
		RestoreOPL3Timer2IntService();
		if (! FreeOPL3Timer2()) {
			printf("OPL3 timer 2 is not allocated...\n");
			exit(1);
			}
		}

	RestoreMMATimer0IntService();
	if (! FreeMMATimer0()) {
		printf("MMA timer 0 is not allocated...\n");
		exit(1);
		}

	RestoreMMATimer1IntService();
	if (! FreeMMATimer1()) {
		printf("MMA timer 1 is not allocated...\n");
		exit(1);
		}

	RestoreMMATimer2IntService();
	if (! FreeMMATimer2()) {
		printf("MMA timer 2 is not allocated...\n");
		exit(1);
		}

	printf("Nbr interrupts: %ld\n", myCounter1);
	printf("Nbr interrupts: %ld\n", myCounter2);
	printf("Nbr interrupts: %ld\n", myCounter3);
	printf("Nbr interrupts: %ld\n", myCounter4);
	printf("Nbr interrupts: %ld\n", myCounter5);
	}

void	TestAvailableEvent()
	{
	long	end;
	long	period;
	WORD	err;

	myCounter1 = 0;
	period = 10000;

	err = SetAvailableTimerEvent(IntService1, period, TIME_PERIODIC);
	if (err) {
		printf("Error while trying to post a timed event:%x\n", err);
		return ;
		}

	WaitNextSecond();
	WaitNextSecond();
	WaitNextSecond();
	WaitNextSecond();
	WaitNextSecond();

	printf("ok\n");
	ResetAvailableTimerEvent();
	printf("Available timer: Nbr interrupts: %ld\n", myCounter1);
	}

void	TestSpecificEvent()
	{
	long	end;
	long	period;

	myCounter1 = 0;
	myCounter2 = 0;
	myCounter3 = 0;
	myCounter4 = 0;
	myCounter5 = 0;
	period = 10000;

	if (gssLevel == level2) {
		if (SetTimerEvent(OPL3Timer1, IntService1, period, TIME_PERIODIC)) {
			printf("Error while reseting OPL3Timer1\n");
			}
		if (SetTimerEvent(OPL3Timer2, IntService2, period, TIME_PERIODIC)) {
			printf("Error while reseting OPL3Timer2\n");
			}
		}
	if (SetTimerEvent(MMATimer0, IntService3, period, TIME_PERIODIC)) {
		printf("Error while setting MMATimer0\n");
		}
	if (SetTimerEvent(MMATimer1, IntService4, period, TIME_PERIODIC)) {
		printf("Error while setting MMATimer1\n");
		}
	if (SetTimerEvent(MMATimer2, IntService5, period, TIME_PERIODIC)) {
		printf("Error while setting MMATimer2\n");
		}

	WaitNextSecond();
	WaitNextSecond();
	WaitNextSecond();
	WaitNextSecond();
	WaitNextSecond();

	printf("ok\n");

	if (gssLevel == level2) {
		if (ResetTimerEvent(OPL3Timer1)) {
			printf("Error while resetting OPL3Timer1\n");
			}
		if (ResetTimerEvent(OPL3Timer2)) {
			printf("Error while resetting OPL3Timer2\n");
			}
		}

	if (ResetTimerEvent(MMATimer0)) {
		printf("Error while resetting MMATimer0\n");
		}
	if (ResetTimerEvent(MMATimer1)) {
		printf("Error while resetting MMATimer1\n");
		}
	if (ResetTimerEvent(MMATimer2)) {
		printf("Error while resetting MMATimer2\n");
		}

	printf("OPL3 #1 Nbr interrupts: %ld\n", myCounter1);
	printf("OPL3 #2 Nbr interrupts: %ld\n", myCounter2);
	printf("MMA  #0 Nbr interrupts: %ld\n", myCounter3);
	printf("MMA  #1 Nbr interrupts: %ld\n", myCounter4);
	printf("MMA  #2 Nbr interrupts: %ld\n", myCounter5);
	}

CallbackProc   IntService1()
	{
	myCounter1++;
	printf("OPL3 #1 Nbr interrupts: %ld\n", myCounter1);
	}

CallbackProc   IntService2()
	{
	myCounter2++;
		printf("OPL3 #2 Nbr interrupts: %ld\n", myCounter2);
	}

CallbackProc   IntService3()
	{
	myCounter3++;
	printf("MMA  #0 Nbr interrupts: %ld\n", myCounter3);
	}

CallbackProc   IntService4()
	{
	myCounter4++;
	printf("MMA  #1 Nbr interrupts: %ld\n", myCounter4);
	}

CallbackProc   IntService5()
	{
	myCounter5++;
	printf("MMA  #2 Nbr interrupts: %ld\n", myCounter5);
	}

/*
 *	Synopsis:		long	TickCount()
 *
 *	Description:	This service routine returns how many tick since ...
 *					Based on a library routine from Borland C. See manual
 *					page 63 .
 *
 * Return value:	Number of tick on a long
 *
 */

long TickCount()
	{
	long nTick;

	nTick = biostime(0, 0);		

	return nTick;
	}

/*
 *	Synopsis:		void	WaitNextSecond()
 *
 *	Description:	This service routine is used for precision while timing
 *						events with a PC clock. It waits for a tick transition
 *						then  move out ofthe routine. If the PC clock is stop then
 *						you  are there forever...
 *
 * Return value:	none
 *
 */

void	WaitNextSecond()
	{
		word delayIO = 0x0080;
		long innerIndex;
		long outerIndex;

		for (outerIndex = 0; outerIndex < 1000; ++outerIndex) {
			for (innerIndex = 0; innerIndex < 500; ++innerIndex) {
				inportb(delayIO);
				outportb(delayIO, 0);
			}
		}
	}

  

```
