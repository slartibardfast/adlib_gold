
/**************************************************************************

Module name:   FMS.c

Version:		1.01

Author:        Francois Rousseau

Date:          December 1991
                   
Description:   This module is used as a link to the stay-resident Sync
				driver.

*****************************************************************************/

/****************************************************************************
							Module History
21/16/91		0.03
*****************************************************************************/

/****************************************************************************
                               Includes
*****************************************************************************/
#ifdef	TURBO
#pragma	hdrfile	fms.sym
#endif

#include	<stdio.h>
#include	<dos.h>

#include   "global.H"
#include   "Control.H"
#include   "FM.H"

#ifdef	TURBO
#pragma	hdrstop
#undef inportb
#undef outportb
#endif

/****************************************************************************
								Local Variables
*****************************************************************************/

void	SendByte(BYTE val);

BYTE	piano_2op[] = {
	0x01,0x4F,0xF1,0x53,0x80,0x00,
	0x11,0x00,0xD2,0x74,0x80,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,
	0x06,0x10,0x00,0x00
	};

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

#ifndef MICROSOFT
	nTick = biostime(0, 0);		
#else
	_bios_timeofday( 0, &nTick);
#endif

	return nTick;
	}

/*
 *	Synopsis:		void	WaitNextTick()
 *
 *	Description:	This service routine is used for precision while timing
 *						events with a PC clock. It waits for a tick transition
 *						then  move out ofthe routine. If the PC clock is stop then
 *						you  are there forever...
 *
 * Return value:	none
 *
 */

void	WaitNextTick()
	{
	long last;

#ifndef MICROSOFT
	last = biostime(0, 0);	 
	while (biostime(0, 0) == last);
#else
	long new;

	_bios_timeofday( 0, &last);
	new = last;

	while (last == new) {
		_bios_timeofday(0, &new);
		}
#endif
	}

main()
	{
	int 	note;
	int 	voice;
	long	end;


	if (InitControlDriver()) {
		printf("Can not link to control chip driver\n");
		exit(1);
		}

	InitFMDriver();
	Set4opMaskOpl3(0);
	SetPercModeOpl3(0);

 	for (voice = 0; voice < 19; voice++) 
		PresetOpl3(voice, (TIMBRE *)piano_2op);
	note = 60;
	voice = 0;

	printf( "\nPlaying notes on YMF262.. Press a key to stop");

	while (! kbhit()) {
		NoteOnOpl3(voice, note);

		WaitNextTick();
		end = TickCount() + 20;
		while (TickCount() < end);

		NoteOffOpl3(voice);
		voice = (voice + 1) % 19;
		if (voice == 17) {
			voice = 18;		/* skip 17 */
			}
		note++;
		if (voice == 0)	{
			note = 60;
			}
		}

	CloseFMDriver();
	CloseControlDriver();
	RemoveInterruptService();

	return(0);
	}
