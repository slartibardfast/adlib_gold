

/**************************************************************************
                   
Module name:   Control

Version:		1.01

Author:        Francois Rousseau

Date:          november 1991

Description:	In order to contribute to the establishment of the Gold
				Sound standar, Adlib is disclosing the protocol of its
				standard driver. This driver is to be used by independent 
				manufacturers of PC sound systems as a common base for the
				control features of hardware based on the Yamaha Magic chip
				set.

				For software developers, the use of a standard protocol for  
				control features will insure uniformsonic results across all
				platforms.

				The driver is written in standard C.

				Two approaches are offered in this function module:
				  -	a direct access to every read and write services. This
					makes almost 90 functions to use directly with only their 
					required arguments.
				  - a centralized approach that use only one routine that
				  	will then redirected the processinbg to an appropriate
					routine. This method is useful for TSR access thru a single
					interrupt routine. The desired service is then passed as 
					an extra argument.

				Access to the physical AdLib control chip is done thru those 
				two	routines: 
				  -	void	SetControlRegister(WORD reg, WORD val)
				  - WORD	GetControlRegister(int reg)

				An important behavior of this module is that it includes the
				interrupt processing of all sources on the AdLib Gold card.
				The interrupt decoding is centralized and decoded in the
				control chip then redirected to the specific source. Callback
				functions are used to redirected the processing to other
				modules. See interrupt routine
				  -	void   interrupt   far	ProcessInterrupt()

*****************************************************************************/

/****************************************************************************
								Module History
12/12/91		0.01
16/12/91		0.02
01/01/92		0.03
24/01/92		0.05
16/02/92		0.06
12/03/92       0.09
13/04/92       1.00
11/11/92       1.01
*****************************************************************************/


/****************************************************************************
                               Includes
*****************************************************************************/

#ifdef TURBO
#pragma	hdrfile	control.sym
#endif

#include   <stdio.h>
#include   <stdlib.h>
#include   <dos.h>

#include   "global.H"
#include   "control.H"
#include   "Interr.H"
#include   "timer.H"
#include   "midi.H"
#include   "wave.H"

#ifdef TURBO
#pragma	hdrstop
#undef		inportb					// Protection against fast access
#undef		outportb
#endif


/****************************************************************************
                             Definitions
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

extern unsigned char phantomControl = 1;
extern unsigned char gssLevel = 2;


/*
 *	Those are the io port for testing the timer driver. Those address are only
 *	defined in this module. Other modules must pass thru a service to access
 *	those address.
 */
       
WORD	baseAddress	   = 0x388;                
WORD   controlIoPort  = 0x38A;
WORD	mmaIoPort      = 0x38C;
WORD	opl3IoPort     = 0x388;
WORD   delayIO    = 0x0080;

/*
 * Set to 1 if you want to have printf messages
 */

#define PRINTF 0

/*
 *	The control chip includes 24 register
 */

#define	numberRegister				0x18

/****************************************************************************
                               Local Protyping
*****************************************************************************/

/****************************************************************************
									Routines
*****************************************************************************/

/*
 *	Synopsis:       SetControlRegister(int reg, WORD val)
 *
 *	Description:	Set register 'reg' of Adlib Control Chip to 'val'. All 
 *					access details are handled here.
 *
 *	Argument:		int	reg
 *						which register to write to
 *
 *					WORD	val
 *						which value towrite in register
 *
 * Returned value:	0	no error
 *					1	error
 *
 */

PUBLIC
WORD	SetControlRegister(WORD reg, WORD val)
	{
	if (gssLevel != level2) return 1;

	if (reg >= numberRegister) return(1);

	asm	pushf
	asm	cli

	if (phantomControl) outportb(controlIoPort, 0xFF);		/* disable OPL-III, enable control bank*/
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	outportb(controlIoPort, reg);			/* select control register 				*/
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	outportb(controlIoPort + 1, val);	/* set new value 								*/
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);

	/*
	 *  Reading the address port when the control chip has been triggered
	 *	returns the status.
	 *
	 *	Wait for RB & SB:
	 *		SB set indicates that the card is busy writing to a register.
	 *		RB set indicates that the card is busy writing its registers to 
	 *		permanent memory
	 */

	while (inportb(controlIoPort) & 0xC0) {	/* wait until control chip free 		*/
		outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	}

 	if (phantomControl) {
		outportb(controlIoPort, 0xfe);		/* re-enable OPL-III 						*/
		outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	}
	asm	popf

	return(0);
	}

/*
 *	Synopsis:		WORD	CtStoreConfigInPermMem()
 *
 *	Description:	This cause all control chip registers, in their current 
 *                 state, to be written to permanent memory. Bit ST is used.
 *
 *	Argument:		none.
 *
 *	Return Value:	1 if ok.
 */	

PUBLIC
WORD   CtStoreConfigInPermMem()
	{
	SetControlRegister(0x00, 0x02);   /* Note change between doc. version */

	return 1;
	}

/*
 *	Synopsis:		WORD	CtRestoreConfigFromPermMem()
 *
 *	Description:	All registers will be restored from permanent memory.
 *					Bit RT is used.
 *
 *	Argument:		none.
 *
 *	Return Value:	1 if ok.
 */	

PUBLIC
WORD	CtRestoreConfigFromPermMem()
	{
	SetControlRegister(0x00, 0x01);	        /* Note change between doc. version */

	/*
	 *	Include software delay
	 */
	return 1;
	}


/*
 *	Synopsis:		WORD	CtSetChannel0SampGain(WORD	value)	(LEFT)
 *					WORD	CtSetChannel1SampGain(WORD	value)	(RIGHT)
 *					WORD    CtGetChannel0SampGain()
 *                 WORD    CtGetChannel1SampGain()
 *
 *	Description:	Set the control gain of sampling channel 0. 256 different
 *					values possible giving a range from approximately 0.04 to
 *					10 times the input value. The exact gain is given by the 
 *					equation:	Gain = (registerValue * 10) / 256
 *					Linear gain.
 *
 *	Argument:		As described, value between 0 - 255.
 *
 *	Return Value:	1 if ok.
 */	

PUBLIC
WORD	CtSetChannel0SampGain(WORD value)
	{
	SetControlRegister(0x02, value);
	return 1;
	}

PUBLIC
WORD	CtSetChannel1SampGain(WORD value)
	{
	SetControlRegister(0x03, value);
	return 1;
	}

PUBLIC
WORD    CtGetChannel0SampGain()
   {
   return GetControlRegister(0x02);
   }

PUBLIC
WORD    CtGetChannel1SampGain()
   {
   return GetControlRegister(0x03);
   }


/*
 *	Synopsis:		WORD	CtSetChannel0SampFreq(WORD value)	(LEFT)
 *					WORD	CtSetChannel1SampFreq(WORD value)	(RIGHT)
 *                 WORD    CtGetChannel0SampFreq()
 *                 WORD    CtGetChannel1SampFreq()
 *
 *	Description:	Filter cutoff frequency is specified in multiples of 100 Hz.
 *					Affects both sampling and playback filters. 
 *
 *	Argument:		As described, value between 0 - 255.
 *
 *	Return Value:	1 if ok.
 */	

PUBLIC
WORD	CtSetChannel0SampFreq(WORD value)
	{
   value = value;

	return 0;
	}

PUBLIC
WORD	CtSetChannel1SampFreq(WORD value)
	{
   value = value;

	return 0;
	}

PUBLIC
WORD    CtGetChannel0SampFreq()
   {
	return 0;
   }

PUBLIC
WORD    CtGetChannel1SampFreq()
   {
	return 0;
   }


/*
 *	Synopsis:		WORD	CtSetChannel0FilterMode(WORD	value)	(LEFT)
 *					WORD	CtSetChannel1FilterMode(WORD	value)	(RIGHT)
 *                 WORD    CtGetChannel0FilterMode()
 *                 WORD    CtGetChannel1FilterMode()
 *
 *	Description:	The gold card uses antialiasing filters during sampling 
 *					and playback. The filter of channel 0 is connected at 
 *					the output of the MMA channel 0 (for playback) when this
 *					bit is 0 and at the input of channel 0 (for sampling) when
 *					this bit is 1.
 *
 *					This filter MUST be set in sample mode before sampling.
 *					This filter MUST be set in playback mode before playback.
 *
 *	Arguments:		0 = playback mode
 *					1 = sample mode
 *
 *	Return Value:	1 if ok.
 */	

PUBLIC
WORD	CtSetChannel0FilterMode(WORD value)
	{
	WORD	registerImage;

	registerImage = GetControlRegister(0x11);
	registerImage &= 0xFD;

	if (value & 0x01) value = 0x02;
	else value = 0x00;

	registerImage |= value;

	SetControlRegister(0x11, registerImage);
	return 1;
	}

PUBLIC
WORD	CtSetChannel1FilterMode(WORD value)
	{
	WORD	registerImage;

	registerImage = GetControlRegister(0x11);

	registerImage &= 0xFE;

	if (value & 0x01) value = 0x01;
	else value = 0x00;

	registerImage |= value;

	SetControlRegister(0x11, registerImage);
	return 1;
	}

PUBLIC
WORD    CtGetChannel0FilterMode()
   {
   return((GetControlRegister(0x11) & 0x02) >> 1);
   }

PUBLIC
WORD    CtGetChannel1FilterMode()
   {
   return(GetControlRegister(0x11) & 0x01);
   }

/*
 *	Synopsis:		WORD	CtStereoMonoAuxSamp(WORD	value)
 *                 WORD    CtGetStereoMonoAuxSamp()
 *
 *	Description:	The microphone and telephone inputs are monophonic sources
 *					and can only be sampled monophonically on channel 0.
 *					Normally the auxiliary inputs are sampled  in stereo  on 
 *					both channel 0 and 1 at the same time. This stereo audio
 *					input can be turned monophonic and sampled on channel 0 by
 *					setting this bit to 1.
 *
 *	Argument:		0 = auxiliary input is stereo
 *					1 = auxiliary input is mono
 *
 *	Return Value:	1 if ok.
 */	

PUBLIC
WORD	CtStereoMonoAuxSamp(WORD value)
	{
	WORD	registerImage;

	registerImage = GetControlRegister(0x11);

	registerImage &= 0xFB;

	if (value & 0x01) value = 0x04;
	else value = 0x00;

	registerImage |= value;

	SetControlRegister(0x11, registerImage);
	return 1;
	}

PUBLIC WORD    CtGetStereoMonoAuxSamp()
   {
   return((GetControlRegister(0x11) & 0x04) >> 2);
   }


/*
 *	Synopsis:		WORD	CtEnabDisabMicroOutput(WORD	value)
 *                 WORD    CtGetEnabDisabMicroOutput()
 *
 *	Description:	When using the microphone input and the normal loudspeaker
 *					outputs of the audio card, audio feedback could result. In
 *					normal mode, this bit is set to 0. When set to 1, the micro-
 *					phone signal is cut from the output of the card and only
 *					sent to the telephone output, eliminating possible causes of
 *					feeedback.
 *
 *	Argument:		0 = Microphone output enabled
 *					1 = Microphone output disabled
 *
 *	Return Value:	1 if ok.
 */	

PUBLIC
WORD	CtEnabDisabMicroOutput(WORD value)
	{
	WORD	registerImage;

	registerImage = GetControlRegister(0x11);

	registerImage &= 0xF7;

	if (value & 0x01) value = 0x08;
	else value = 0x00;

	registerImage |= value;

	SetControlRegister(0x11, registerImage);
	return 1;
	}

PUBLIC WORD    CtGetEnabDisabMicroOutput()
   {
   return((GetControlRegister(0x11) & 0x08) >> 3);
   }


/*
 *	Synopsis:		WORD	CtEnabDisabInternPcSpeak(WORD	value)
 *                 WORD    CtGetEnabDisabInternPcSpeak()
 *
 *
 *	Description:	This can enable the PC internal speaker signal to be mixed
 *					with the audio signals of a Gold card (directly, without
 *                 any	mixer volume control).
 *
 *	Argument:		0 = Disconnect internal PC speaker
 *					1 = Connect internal PC speaker
 *
 *	Return Value:	1 if ok.
 */	

PUBLIC
WORD	CtEnabDisabInternPcSpeak(WORD value)
	{
	WORD	registerImage;

	registerImage = GetControlRegister(0x11);

	registerImage &= 0xDF;

	if (value & 0x01) value = 0x20;
	else value = 0x00;

	registerImage |= value;

	SetControlRegister(0x11, registerImage);
	return 1;
	}

PUBLIC WORD    CtGetEnabDisabInternPcSpeak()
   {
   return((GetControlRegister(0x11) & 0x20) >> 5);
   }


/*
 *	Synopsis:		WORD	CtSelectInterruptLineNbr(WORD	value)
 *                 WORD    CtGetInterruptLineNbr()
 *
 *	Description:	Interrupt line is used by OPL3, MMA and telephone hardware.
 *
 *					Valid interrupt lines on an XT are IRQ3, IRQ4, IRQ5 and IRQ7.
 *
 *					Valid interrupt lines on an AT are IRQ3, IRQ4, IRQ5, IRQ7,
 *					IRQ10, IRQ11, IRQ12 and IRQ15.
 *
 *	Argument:		0 = IRQ3
 *					1 = IRQ4
 *					2 = IRQ5
 *					3 = IRQ7
 *					4 = IRQ10
 *					5 = IRQ11
 *					6 = IRQ12
 *					7 = IRQ15
 *
 *	Return Value:	1 if ok.
 */	

PUBLIC
WORD	CtSelectInterruptLineNbr(WORD	value)
	{
	WORD	registerImage;

	registerImage = GetControlRegister(0x13);

	registerImage &= 0xF8;
	registerImage |= (value & 0x07);

	SetControlRegister(0x13, registerImage);
	return 1;
	}

PUBLIC
WORD    CtGetInterruptLineNbr()
   {
   return(GetControlRegister(0x13) & 0x07);
   }


/*
 *	Synopsis:		WORD	CtEnabDisabInterrupt(WORD	value)
 *                 WORD    CtGetEnabDisabInterrupt()
 *
 *	Description:	Interrupt line is used by the OPL3, MMA and telephone
 *					hardware.
 *
 *	Argument:		0 = disable
 *					1 = enable
 *
 *	Return Value:	1 if ok.
 */	

PUBLIC
WORD	CtEnabDisabInterrupt(WORD	value)
	{
	WORD	registerImage;

	registerImage = GetControlRegister(0x13);

	registerImage &= 0xF7;

	if (value & 0x01) value = 0x08;
	else value = 0x00;

	registerImage |= value;
	SetControlRegister(0x13, registerImage);
	return 1;
	}

PUBLIC WORD    CtGetEnabDisabInterrupt()
   {
   return((GetControlRegister(0x13) & 0x08) >> 3);
   }


/*
 *	Synopsis:		WORD	CtSelectDMA0ChannelSampChan(WORD	value)	(LEFT)
 *					WORD	CtSelectDMA1ChannelSampChan(WORD	value)	(RIGHT)
 *                 WORD    CtGetDMA0ChannelSampChan()
 *                 WORD    CtGetDMA1ChannelSampChan()
 *
 *	Description:	Valid DMA channels are 0 - 7. Other channel numbers are
 *					reserved for future extensions.
 *
 *	Argument:		0 = DMA 0
 *					1 = DMA 1
 *					2 = DMA 2
 *					3 = DMA 3
 *					4 - 7 ...
 *
 *	Return Value:	1 if ok.
 */	

PUBLIC
WORD	CtSelectDMA0ChannelSampChan(WORD	value)
	{
	WORD	registerImage;

	registerImage = GetControlRegister(0x13);

	registerImage &= 0x8F;
	registerImage |= ((value & 0x07) << 4);
	SetControlRegister(0x13, registerImage);
	return 1;
	}

PUBLIC
WORD	CtSelectDMA1ChannelSampChan(WORD	value)
	{
	WORD	registerImage;

	registerImage = GetControlRegister(0x14);

	registerImage &= 0x8F;
	registerImage |= ((value & 0x07) << 4);
	SetControlRegister(0x14, registerImage);
	return 1;
	}


PUBLIC WORD    CtGetDMA0ChannelSampChan()
   {
   return((GetControlRegister(0x13) & 0x70) >> 4);
   }

PUBLIC WORD    CtGetDMA1ChannelSampChan()
   {
   return((GetControlRegister(0x14) & 0x70) >> 4);
   }


/*
 *	Synopsis:		WORD	CtEnabDisabDMA0SampChan(WORD value)	(LEFT)
 *					WORD	CtEnabDisabDMA1SampChan(WORD value)	(RIGHT)
 *                 WORD    CtGetEnabDisabDMA0SampChan()
 *                 WORD    CtGetEnabDisabDMA1SampChan()
 *
 *	Description:	Disable or enable use of DMA channel for sampling channel
 *					0.
 *
 *	Argument:		0 = disable
 *					1 = enable
 *
 *	Return Value:	1 if ok.
 */	

PUBLIC
WORD	CtEnabDisabDMA0SampChan(WORD value)
	{
	WORD	registerImage;

	registerImage = GetControlRegister(0x13);

	registerImage &= 0x7F;

	if (value & 0x01) value = 0x80;
	else value = 0x00;

	registerImage |= value;

	SetControlRegister(0x13, registerImage);
	return 1;
	}

PUBLIC
WORD	CtEnabDisabDMA1SampChan(WORD value)
	{
	WORD	registerImage;

	registerImage = GetControlRegister(0x14);

	registerImage &= 0x7F;

	if (value & 0x01) value = 0x80;
	else value = 0x00;

	registerImage |= value;
	SetControlRegister(0x14, registerImage);
	return 1;
	}

PUBLIC WORD    CtGetEnabDisabDMA0SampChan()
   {
   return((GetControlRegister(0x13) & 0x80) >> 7);
   }

PUBLIC WORD    CtGetEnabDisabDMA1SampChan()
   {
   return((GetControlRegister(0x14) & 0x80) >> 7);
   }


/*
 *	Synopsis:		WORD	CtSetRelocationAddress(WORD   value)
 *                 WORD    CtGetRelocationAddress()
 *
 *	Description:	Set ports addresses for  MMA, OPL3 and control chip.
 *
 *	Argument:		new IO addresse, value between 0 - 127, use a  multiple of
 *					8  to get the actual io port.
 *
 *	Return Value:	1 if ok.
 */	

WORD	level1BaseAddress = 0x388;	// Default address

PUBLIC
WORD	CtSetRelocationAddress(WORD value)
	{
	WORD	registerImage;

	if (gssLevel == level1) {
		level1BaseAddress = value;
	    CtSetControlDriverAddress(level1BaseAddress);
		return 1;
		}

	registerImage = (value>>3) & 0x7F;
	asm	pushf
	asm	cli

	if (phantomControl) {
		outportb(controlIoPort, 0xFF);
		outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	}
   outportb(controlIoPort, 0x15);
	 outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
   outportb(controlIoPort + 1, registerImage);
	 outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
   CtSetControlDriverAddress(registerImage * 8);
   while (inportb(controlIoPort) & 0xC0) {
		 outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	 }

   if (phantomControl) {
		outportb(controlIoPort, 0xfe);
		outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	 }
	asm	popf

	return 1;
	}

PUBLIC WORD    CtGetRelocationAddress()
   {
	if (gssLevel == level1) {
		return(level1BaseAddress << 3);
		}
   return((GetControlRegister(0x15) & 0x7F) << 3);
   }

/*
 *	Synopsis:		WORD	CtSetMixerLevelForFMLeft(WORD	value)
 *					WORD	CtSetMixerLevelForFMRight(WORD	value)
 *					WORD	CtSetMixerLevelForLeftSamplePb(WORD	value)
 *					WORD	CtSetMixerLevelForRightSamplePb(WORD	value)
 *					WORD	CtSetMixerLevelForAuxLeft(WORD	value)
 *					WORD	CtSetMixerLevelForAuxRight(WORD	value)
 *					WORD	CtSetMixerLevelForMicrophone(WORD	value)
 *					WORD	CtSetMixerLevelForTelephone(WORD	value)
 *					WORD	CtSetOutputVolumeLeft(WORD	value)
 *					WORD	CtSetOutputVolumeRight(WORD	value)
 *                 WORD    CtGetMixerLevelForFMLeft()
 *                 WORD    CtGetMixerLevelForFMRight()
 *                 WORD    CtGetMixerLevelForLeftSamplePb()
 *                 WORD    CtGetMixerLevelForRightSamplePb()
 *                 WORD    CtGetMixerLevelForAuxLeft()
 *                 WORD    CtGetMixerLevelForAuxRight()
 *                 WORD    CtGetMixerLevelForMicrophone()
 *                 WORD    CtGetMixerLevelForTelephone()
 *                 WORD    CtGetOutputVolumeLeft()
 *                 WORD    CtGetOutputVolumeRight()
 *
 *	Description:	As described  by the synopsis.
 *
 *	Argument:		0 - 255
 *
 *	Return Value:	1 if ok.
 */	

PUBLIC
WORD	CtSetMixerLevelForFMLeft(WORD	value)
	{
	SetControlRegister(0x09, value);
	return 1;
	}

PUBLIC
WORD	CtSetMixerLevelForFMRight(WORD value)
	{
	SetControlRegister(0x0A, value);
	return 1;
	}

PUBLIC
WORD	CtSetMixerLevelForLeftSamplePb(WORD	value)
	{
	SetControlRegister(0x0B, value);
	return 1;
	}

PUBLIC
WORD	CtSetMixerLevelForRightSamplePb(WORD value)
	{
	SetControlRegister(0x0C, value);
	return 1;
	}

PUBLIC
WORD	CtSetMixerLevelForAuxLeft(WORD	value)
	{
	SetControlRegister(0x0D, value);
	return 1;
	}

PUBLIC
WORD	CtSetMixerLevelForAuxRight(WORD	value)
	{
	SetControlRegister(0x0E, value);
	return 1;
	}

PUBLIC
WORD	CtSetMixerLevelForMicrophone(WORD	value)
	{
	SetControlRegister(0x0F, value);
	return 1;
	}

PUBLIC
WORD	CtSetMixerLevelForTelephone(WORD	value)
	{
	SetControlRegister(0x10, value);
	return 1;
	}

	/*
	 *	Last 6 bits used only
    *
    *  Meanings of values
    *
    *          db          D5-D0       argument      index
    *          --          -----       --------      -----
    *          6           0x3F      252 - 255         36
    *          4           0x3E      245 - 251         35
    *          2           0x3D      238 - 244         34
    *          0           0x3C      231 - 237         33
    *         -2           0x3B      224 - 230         32
    *         -4           0x3A      217 - 223         31
    *         -6           0x39      210 - 216         30
    *         -8           0x38      203 - 209         29
    *         -10          0x37      196 - 202         28
    *         -12          0x36      189 - 195         27
    *         -14          0x35      182 - 188         26
    *         -16          0x34      175 - 181         25
    *         -18          0x33      168 - 174         24
    *         -20          0x32      161 - 167         23
    *         -22          0x31      154 - 160         22
    *         -24          0x30      147 - 155         21
    *         -26          0x2F      140 - 146         20
    *         -28          0x2E      133 - 139         19
    *         -30          0x2D      126 - 132         18
    *         -32          0x2C      119 - 125         17
    *         -34          0x2B      112 - 118         16
    *         -36          0x2A      105 - 111         15
    *         -38          0x29       98 - 104         14
    *         -40          0x28       91 - 97          13
    *         -42          0x27       84 - 90          12
    *         -44          0x26       77 - 83          11
    *         -46          0x25       70 - 76          10
    *         -48          0x24       63 - 69          9
    *         -50          0x23       56 - 62          8
    *         -52          0x22       49 - 55          7
    *         -54          0x21       42 - 48          6
    *         -56          0x20       35 - 41          5
    *         -58          0x1F       28 - 35          4
    *         -60          0x1E       21 - 27          3
    *         -62          0x1D       14 - 20          2
    *         -64          0x1C        7 - 13          1
    *         -80          0x1B        0 - 6           0
    *          ..          ....        0 - 0           0
    *         -80          0x00        0 - 0           0
    *
    *
	 */

PUBLIC
WORD	CtSetOutputVolumeLeft(WORD value)
	{
	WORD	registerImage;


   value /= 7;
   value += 0x1B;      /* 27 */
	registerImage = 0xC0;
	registerImage |= value;

	SetControlRegister(0x04, registerImage);
	return 1;
	}

PUBLIC
WORD	CtSetOutputVolumeRight(WORD	value)
	{
	WORD	registerImage;

	/*
	 *	Last 6 bits used only
	 */

   value /= 7;
   value += 0x1B;      /* 27 */
	registerImage = 0xC0;
	registerImage |= value;

	SetControlRegister(0x05, registerImage);
	return 1;
	}

PUBLIC WORD    CtGetMixerLevelForFMLeft()
   {
   return(GetControlRegister(0x09));
   }

PUBLIC WORD    CtGetMixerLevelForFMRight()
   {
   return(GetControlRegister(0x0A));
   }

PUBLIC WORD    CtGetMixerLevelForLeftSamplePb()
   {
   return(GetControlRegister(0x0B));
   }

PUBLIC WORD    CtGetMixerLevelForRightSamplePb()
   {
   return(GetControlRegister(0x0C));
   }

PUBLIC WORD    CtGetMixerLevelForAuxLeft()
   {
   return(GetControlRegister(0x0D));
   }

PUBLIC WORD    CtGetMixerLevelForAuxRight()
   {
   return(GetControlRegister(0x0E));
   }

PUBLIC WORD    CtGetMixerLevelForMicrophone()
   {
   return(GetControlRegister(0x0F));
   }

PUBLIC WORD    CtGetMixerLevelForTelephone()
   {
   return(GetControlRegister(0x10));
   }

PUBLIC WORD    CtGetOutputVolumeLeft()
   {
   WORD    retVal;

   retVal = GetControlRegister(0x04) & 0x3F;
   retVal -=0x1B;
   retVal *= 7; 
   return retVal;
   }

PUBLIC WORD    CtGetOutputVolumeRight()
   {
   WORD    retVal;

   retVal = GetControlRegister(0x05) & 0x3F;
   retVal -=0x1B;
   retVal *= 7; 
   return retVal;
   }


/*
 *	Synopsis:		WORD	CtSetOutputBassLevel(WORD	value)
 *					WORD	CtSetOutputTrebleLevel(WORD	value)
 *                 WORD    CtGetOutputBassLevel()
 *                 WORD    CtGetOutputTrebleLevel()
 *
 *	Description:	Negative values decreases treble, positive numbers
 *					increase bass. 0 does not alter sound.
 *
 *	Argument:		Range between -128 & 127.
 *
 *	Return Value:	1 if ok.
 */
	
/*
 *          db          D5-D0       argument      index
 *          --          -----       --------      -----
 *          15           F         240 - 255        15
 *          15           E         224 - 239        14
 *          15           D         208 - 223        13
 *          15           C         192 - 207        12
 *          15           B         176 - 191        11
 *          12           A         160 - 175        10
 *           9           9         144 - 159        9
 *           6           8         128 - 143        8
 *           3           7         112 - 127        7
 *           0           6          96 - 111        6
 *          -3           5          80 - 95         5
 *          -6           4          64 - 79         4
 *          -9           3          48 - 63         3
 *          -12          2          32 - 47         2
 *          -12          1          16 - 31         1
 *          -12          0           0 - 15         0
 *         
 */

PUBLIC
WORD	CtSetOutputBassLevel(WORD	value)
	{
	WORD	registerImage;

	/*
	 *	Last 4 bits used only
	 */

   value /= 16;

	registerImage = 0xF0;
	registerImage |= value;
	SetControlRegister(0x06, registerImage);
	return 1;
	}

PUBLIC
WORD	CtSetOutputTrebleLevel(WORD	value)
	{
	WORD	registerImage;

	/*
	 *	Last 4 bits used only
	 */

   value /= 16;

	registerImage = 0xF0;
	registerImage |= value;

	SetControlRegister(0x07, registerImage);
	return 1;
	}

PUBLIC WORD    CtGetOutputBassLevel()
   {
   return((GetControlRegister(0x06) & 0x0F) * 16);
   }

PUBLIC WORD    CtGetOutputTrebleLevel()
   {
   return((GetControlRegister(0x07) & 0x0F) * 16);
   }


/*
 *	Synopsis:		WORD	CtEnabDisabOutputMuting(WORD	value)
 *                 WORD    CtGetEnabDisabOutputMuting()
 *
 *	Description:	As it says...
 *
 *	Argument:		0 = disable
 *					1 = enable
 *
 *	Return Value:	1 if ok.
 */	

PUBLIC
WORD	CtEnabDisabOutputMuting(WORD value)
	{
	WORD	registerImage;

	registerImage = GetControlRegister(0x08);

	registerImage &= 0xDF;

	if (value & 0x01) value = 0x20;
	else value = 0x00;

	registerImage |= value;

	SetControlRegister(0x08, registerImage);
	return 1;
	}

PUBLIC WORD    CtGetEnabDisabOutputMuting()
   {
   return((GetControlRegister(0x08) & 0x20) >> 5);
   }


/*
 *	Synopsis:	 	WORD	CtSelectSCSIInterruptNumber(WORD	value)
 *                 WORD    CtGetSCSIInterruptNumber()
 *
 *	Description: 	Valid interrupt lines on an XT are IRQ3, IRQ4, IRQ5 and
 *					IRQ7. 
 *
 *					Valid interrupt lines on an AT are IRQ3, IRQ4, IRQ5, IRQ7,
 *					IRQ10, IRQ11, IRQ12 and IRQ15.
 *
 *	Argument:		0 = IRQ3
 *					1 = IRQ4
 *					2 = IRQ5
 *					3 = IRQ7
 *					4 = IRQ10
 *					5 = IRQ11
 *					6 = IRQ12
 *					7 = IRQ15
 *
 *	Return Value:	1 if ok.
 */	

PUBLIC
WORD	CtSelectSCSIInterruptNumber(WORD value)
	{
	WORD	registerImage;

	registerImage = GetControlRegister(0x16);

	registerImage &= 0xF8;
	registerImage |= (value & 0x07);

	SetControlRegister(0x16, registerImage);
	return 1;
	}

PUBLIC WORD    CtGetSCSIInterruptNumber()
   {
   return(GetControlRegister(0x16) & 0x07);
   }


/*
 *	Synopsis:		WORD	CtEnabDisabSCSIInterrupt(WORD	value)
 *	                WORD    CtEnabDisabSCSIDMA(WORD	value)
 *	                WORD    CtGetEnabDisabSCSIInterrupt()
 *	                WORD    CtGetEnabDisabSCSIDMA()
 *
 *	Description:	As it says...
 *
 *	Argument:		0 = disable
 *					1 = enable
 *
 *	Return Value:	1 if ok.
 */	

PUBLIC
WORD	CtEnabDisabSCSIInterrupt(WORD value)
	{
	WORD	registerImage;

	registerImage = GetControlRegister(0x16);

	registerImage &= 0xF7;
	if (value & 0x01) value = 0x08;
	else value = 0x00;

	registerImage |= value;

	SetControlRegister(0x16, registerImage);
	return 1;
	}

PUBLIC
WORD	CtEnabDisabSCSIDMA(WORD value)
	{
	WORD	registerImage;

	registerImage = GetControlRegister(0x16);

	registerImage &= 0x7F;
	if (value & 0x01) value = 0x80;
	else value = 0x00;

	registerImage |= value;

	SetControlRegister(0x16, registerImage);
	return 1;
	}

PUBLIC WORD    CtGetEnabDisabSCSIInterrupt()
   {
   return((GetControlRegister(0x16) & 0x08) >> 3);
   }

PUBLIC WORD    CtGetEnabDisabSCSIDMA()
   {
   return((GetControlRegister(0x16) & 0x80) >> 7);
   }


/*
 *	Synopsis:		WORD	CtSelectSCSIDMAChannel(WORD value)
 *                 WORD    CtGetSCSIDMAChannel()
 *
 *	Description:	Valid DMA channels are 0 - 3. Other channel numbers are
 *						reserved for future extensions.
 *
 *	Argument:		0 = DMA 0
 *					1 = DMA 1
 *					2 = DMA 2
 *					3 = DMA 3
 *                 4 - 7 ...
 *
 *	Return Value:	1 if ok.
 */	

PUBLIC
WORD	CtSelectSCSIDMAChannel(WORD value)
	{
	WORD	registerImage;

	registerImage = GetControlRegister(0x16);

	registerImage &= 0x8F;
	registerImage |= ((value & 0x07) << 4);

	SetControlRegister(0x16, registerImage);
	return 1;
	}

PUBLIC WORD    CtGetSCSIDMAChannel()
   {
   return((GetControlRegister(0x16) & 0x70) >> 4);
   }


/*
 *	Synopsis:		WORD	CtSetSCSIRelocationAddress(WORD	value)
 *                 WORD    CtGetSCSIRelocationAddress()
 *
 *	Description:	Set ports addresses for SCSI controller
 *
 *	Argument:		new IO addresse, value between 0 - 127, use a  multiple of
 *					8  to get the actual io port.
 *
 *	Return Value:	1 if ok.
 */	

PUBLIC
WORD	CtSetSCSIRelocationAddress(WORD value)
	{
	WORD	registerImage;

	registerImage = 0x00;

	registerImage |= ((value>>3) & 0x7F);

	SetControlRegister(0x17, registerImage);
	return 1;
	}

PUBLIC WORD    CtGetSCSIRelocationAddress()
   {
	WORD registerImage;
	registerImage = GetControlRegister(0x17);
	registerImage <<= 3;
   return(registerImage);
   }


/*****************************************************************************/

/*
 *	Synopsis:		WORD	CtSetHangUpPickUpTelephoneLine(WORD	value)
 *	            	WORD	CtGetHangUpPickUpTelephoneLine(WORD	value)
 *
 *	Description:	As described
 *						
 *	Argument:		0: Disconnect telephone line
 *					1: Connect telephone line
 *
 *	Return Value:	1 if ok.
 */	

PUBLIC
WORD	CtSetHangUpPickUpTelephoneLine(WORD value)
	{
	SetControlRegister(0x01, (value & 0x01));
	return 1;
	}

PUBLIC
WORD	CtGetHangUpPickUpTelephoneLine()
	{
	return (GetControlRegister(0x01) & 0x01);
	}

/*
 *	Synopsis:		WORD	CtSelectOutputSources(WORD value)
 *                 WORD    CtGetOutputSources()
 *
 *	Description:	On the Adlib Gold 1000, Gold 2000 and Gold 2000 MC cards,
 *					mixing and volume control is performed in two stages. First,
 *					all sources are sent to a stereo mixer. Then, the stereo
 *					output of the mixer is fed into the final volume control
 *					circuitry. At that stage, the mixer output channels can be 
 *					mixed in the following fashion:
 *						
 *	Argument:		0 =	left mixer channel to left output & right mixer channel
 *					 	to right output
 *					1 = left mixer channel to both left and right outputs
 *					2 = right mixer channel to both left and right outputs
 *
 *	Return Value:	1 if ok.
 */	

PUBLIC
WORD	CtSelectOutputSources(WORD value)
	{
	WORD	registerImage;

	registerImage = GetControlRegister(0x08);

	switch(value & 0x03) {
		case 0:
			value = 0x06;
			break;
		case 1:
			value = 0x02;
			break;
		case 2:
			value = 0x04;
			break;
		case 3:
			value = 0x00;
			break;
		}
		
	registerImage &= 0xF8;
	registerImage |= value;

	SetControlRegister(0x08, registerImage);
	return 1;
	}

PUBLIC WORD    CtGetOutputSources()
   {
   return(GetControlRegister(0x08) & 0x07);
   }


/*
 *	Synopsis:		WORD	CtSelectOutputMode(WORD value)
 *                 WORD    CtGetOutputMode()
 *
 *	Description:	As described
 *						
 *	Argument:		0 =	Forced mono
 *					1 = linear stereo
 *					2 = pseudo stereo
 *					3 = spatial stereo
 *
 *	Return Value:	1 if ok.
 */	

PUBLIC
WORD	CtSelectOutputMode(WORD value)
	{
	WORD	registerImage;

	registerImage = GetControlRegister(0x08);

	registerImage &= 0xE7;
	registerImage |= ((value & 0x03) << 3);

	SetControlRegister(0x08, registerImage);
	return 1;
	}

PUBLIC WORD    CtGetOutputMode()
   {
   return((GetControlRegister(0x08) & 0x18) >> 3);
   }


/*
 *	Synopsis:		WORD	CtSetSurroundingPreset(WORD	value)
 *                 WORD    CtGetSurroundingPreset()
 *
 *	Description:	A surround Option can be added to the Adlib Gold card.
 *					This parameter stores a surround preset number in the card's
 *					memory for future reference by the surround driver. However,
 *					it is not the responsibility of the control driver to program
 *					the actual surround hardware.
 *					
 *					Descriptive names will be given for the surround presets. 
 *					Manufacturers and software developpers will then be able to 
 *					provide surround drivers to closely match the presets used
 *					by AdLib.
 *						
 *	Argument:		value between 0 - 255
 *
 *	Return Value:	1 if ok.
 */	

PUBLIC
WORD	CtSelectSurroundingPreset(WORD value)
	{
   value = value;

	return 0;
	}

PUBLIC WORD    CtGetSurroundingPreset()
   {
	return 0;
   }


/*****************************************************************************/


/*
 *	Synopsis:			WORD	GetControlRegister(int   reg)
 *
 *	Description:		Return value stored on register 'reg' of Adlib 
 *						Control Chip.
 *
 *	Argument:			int	reg
 *						    which register to write to
 *
 * Returned value:	Returns the WORD at the register position
 *
 */

PUBLIC	WORD	
GetControlRegister(int reg)
	{
	WORD val;


   if (reg == -1) {
		asm	pushf
		asm	cli
		/* disable OPL-III, enable control bank*/
   	if (phantomControl) {
			outportb(controlIoPort, 0xFF);
			outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
		}
		val = inportb(controlIoPort);	    /* get status 	   */
		outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
		/* re-enable OPL-III 			*/	 	
		if (phantomControl) {
			outportb (controlIoPort, 0xFE);
			outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
		}
		asm	popf
		return ((WORD)val);
		}

	if (reg >= numberRegister) return 0;

	asm	pushf
	asm	cli

	/* disable OPL-III, enable control bank*/
	if (phantomControl) {
		outportb(controlIoPort, 0xFF);
		outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	}
	outportb(controlIoPort, reg);			/* select control register	*/
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	val = inportb(controlIoPort +1);		/* get current value 	  	*/
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
 	/* re-enable OPL-III 			*/
	if (phantomControl) {
		outportb (controlIoPort, 0xFE);
		outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	}

	asm	popf

	return ((WORD)val);
	}

/*
 *	Synopsis:		WORD	CtGetDriverInformation()
 *
 *	Description:	bit 0-7:	version number.
 *					bit 8-15:	0 means Adlib
 *
 *	Argument:		none.
 *
 *	Return Value:	As described
 */	

PUBLIC
WORD	CtGetDriverInformation()
	{
	return 0;
	}

/*
 *	Synopsis:		WORD	CtGetBoardIdentificationCode()
 *
 *	Description:	bit 0-3  = board identification code
 *
 *					0 -	Gold 2000
 *					1 -	Gold 1000
 *					2 -	Gold 2000 MC
 *
 *	Argument:		none.
 *
 *	Return Value:	As described
 */	

PUBLIC
WORD	CtGetBoardIdentificationCode()
	{
	BYTE	reg;

	reg = GetControlRegister(0x00);
	return (reg & 0x0F);
	}

/*
 *	Synopsis:		WORD	CtGetBoardOptions()
 *
 *	Description:	bit 0-3  (0 = not present, 1 = installed)
 *
 *					bit  0 - Telephone
 *					bit  1 - Surround
 *					bit  2 - SCSI
 *					bit  3 - Currently unused
 *
 *	Argument:		none.
 *
 *	Return Value:	As described
 */	

PUBLIC
WORD	CtGetBoardOptions()
	{
	BYTE	reg;

	reg = GetControlRegister(0x00);
	reg = (reg & 0x70) >> 4;
	return (~reg & 0x07);
	}

/*
 *	Synopsis:		WORD	CtGetControllerStatus()
 *
 *	Description:	bit 0 -	equals 1 when an OPL3 interrupt is pending
 *					bit 1 -	equals 1 when an MMA interrupt is pending
 *					bit 2 -	equals 1 when an telephone interrupt is pending
 *					bit 3 -	equals 1 when a SCSI interrupt is pending
 *					bit 6 -	equals 1 when the Control Chip is currently
 *							occupied writing a value to the Mixer Chip or
 *							the Volume Control Chip.
 *
 *							The bit is polled by all set functions,  prior
 *							to writing to the registers, to make sure that the 
 *							Control Chip is free to proceed with another 
 *							operation.
 *
 *					bit 7	Set to 1 when the Control Chip is busy writing its
 *							internal registers to the external EEPROM chip.
 *							This bit must be polled after activating the "Store
 *							configuration" sequence to make sure that the
 *							Control Chip is free to proceed with another 
 *							operation.
 *
 *	Argument:		none.
 *
 *	Return Value:	As described.
 */
	
PUBLIC
WORD	CtGetControllerStatus()
	{
	return GetControlRegister(-1);
	}

/*****************************************************************************/

/*
 *	Synopsis:		WORD	CtGetRingTelephoneStatus()
 *
 *	Description:	bit 0:	"Ring signal" (0- no ring, 1- ring)
 *
 *	Argument:		none.
 *
 *	Return Value:	As described.
 */

PUBLIC
WORD	CtGetRingTelephoneStatus()
	{
	BYTE	reg;

	reg = GetControlRegister(0x01);
	return( reg >> 1);
	}

/*****************************************************************************/

/*
 *	Synopsis:		WORD    CtSelectInterruptRoutine()
 *
 *	Description:	This routine will install the default interrupt routine
 *					on the associated vector choosen in register 13 via
 *					the interrupt number (IRQ3 = 0, ...)
 *
 *	Argument:		value: not used
 *
 *	Return Value:	As described.
 */

PUBLIC
WORD    CtSelectInterruptRoutine()
	{
	return(1);
	}

/*
 *	Synopsis:		WORD    CtGetInterruptRoutine()
 *
 *	Description:	This routine returns the corresponding interrupt number
 *					associated with the content of register 13.
 *
 *	Argument:		none.
 *
 *	Return Value:	Return the corresponding interrupt number.
 */

PRIVATE	BYTE vecs[] = { 11, 12, 13, 15, 0x72, 0x73, 0x74, 0x77 };

PUBLIC
WORD    CtGetInterruptRoutine()
	{
	WORD	interr;

	interr = vecs[CtGetInterruptLineNbr()];
	return	interr;			
	}

/*
 *	Synopsis:		WORD    CtGetGoldCardPresence()
 *
 *	Description:	Return 1 if any Gold card is found.
 *
 *	Argument:		none.
 *
 *	Return Value:	0		not AdLib Gold card found in PC
 *					1		gss level 1 card found
 *					2 		gss level 2 card found
 */

PUBLIC
WORD    CtGetGoldCardPresence()
	{
	WORD	prev;

	asm	pushf
	asm	cli

	/* 
    * First see if MMA and OPL3 are at specified address
    */
	outportb(mmaIoPort, 0x0B);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	inportb(0x20);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	inportb(0x20);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	prev = inportb(mmaIoPort + 1);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	inportb(0x20);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	inportb(0x20);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
  outportb(mmaIoPort + 1, 0x5A);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	inportb(0x20);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	inportb(0x20);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	if (inportb(mmaIoPort + 1) != 0x5A) {
		asm popf
		gssLevel = levelNoCard;
		outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
		return(0);			/* No Gold card found! */
       }

	outportb(mmaIoPort + 1, prev);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	/* 
    * Then check if level 2 mixer is present, and at which level.
    * It can be either as phantom at baseAddress, or at direct at 
    * baseAddess + 8.
    */

	/* First at baseAddress + 8 */

	outportb(baseAddress + 8, 0x09);		
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
   prev = inportb(baseAddress + 9);
	 outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
   outportb(baseAddress + 9, 0x54);
	 	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	inportb(0x20);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	inportb(0x20);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	if ((inportb(baseAddress + 9) & 0x7C) == 0x54) {
			 outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
       outportb(baseAddress + 9, prev);
			 	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
		asm popf;
		phantomControl = 0;
		controlIoPort = baseAddress + 8;
		gssLevel = level2;
       return(2);			  				
		}

	/* Otherwise, as phantom 	*/
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	outportb(baseAddress + 2, 0xFF);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	outportb(baseAddress + 2, 0x09);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);	  
	prev = inportb(baseAddress + 3);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	outportb(baseAddress + 3, 0x54);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	inportb(0x20);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	inportb(0x20);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);

	if ((inportb(baseAddress + 3) & 0x7C) == 0x54) {
			 outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
       outportb(baseAddress + 3, prev);
			 outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
 		outportb (baseAddress + 2, 0xFE);	  
		outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
		asm popf;
		phantomControl = 1;
		controlIoPort = baseAddress + 2;
		gssLevel = level2;
       return(2);			  				
		}

	/* 
	 * No control chip found. This is a level 1 card
	 */

	asm popf
	gssLevel = level1;
	return(1);
	}


/*
 *	Synopsis:		WORD	CtProgramSurroundPreset(BYTE *ptrData)
 *
 *	Description:	This routine will store a preset into the surround module.
 *					The preset is defined by a 31 bytes array passed as
 *					argument.
 *
 *	Argument:		BYTE	*ptrData
 *						pointer to the aray of 31 bytes
 *
 *	Return Value:	0	no error
 *					1	error		no surround module
 */

PUBLIC
WORD	CtProgramSurroundPreset(BYTE *ptrData)
	{
	WORD	addr, data, cmd;
	int 	i, k;

	/*
	 *	Check if there is a surround module installed
	 */

	if (! (CtGetBoardOptions() & 0x02)) return 1;

	for (i = 0; i < 31; i++) {
		cmd = 0;					/* clock LOW, A0 LOW */
		addr = i;
		for(k = 7; k >= 0; k--) {
			cmd &= ~2;				/* clock LOW */
			SetControlRegister( 0x18, cmd);
			cmd = (cmd & ~1) | ((addr >> k) & 1);
			SetControlRegister( 0x18, cmd);
			cmd |= 2;				/* clock HIGH */
			SetControlRegister( 0x18, cmd);
			}

		/*
		 *	Put A0 to 1 to latch the chip.
		 */

		cmd |= 4;
		SetControlRegister(0x18, cmd);

		data = ptrData[ i];
		for( k = 7; k >= 0; k--) {
			cmd &= ~2;				/* clock LOW */
			SetControlRegister(0x18, cmd);
			cmd = (cmd & ~1) | ((data >> k) & 1);
			SetControlRegister(0x18, cmd);
			cmd |= 2;				/* clock HIGH */
			SetControlRegister(0x18, cmd);
			}
		/*
		 *	Put A0 to 0 to latch the chip.
		 */

		cmd &= ~4;
		SetControlRegister(0x18, cmd);
		}

	return 0;
	}

/*
 *	Synopsis:		CallbackProc TempMIDIDoNothing()
 *
 *	Description:	Used as a default MIDI interrupt processing routine.
 *
 * Arguments:		none.
 *
 *	Returned value: None
 *                 
 */

PRIVATE
CallbackProc TempMIDIDoNothing()
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
	asm	mov	mmaStatus, bl
#endif

   /*******************************************************************
    *                  Tranmission interrupt
    *******************************************************************/

	/*
	 *	Fill the transmission FIFO
	 */

   if (mmaStatus & 0x08) {
		}

   /*******************************************************************
    *			                  Reception interrupt
    *******************************************************************/

   if (mmaStatus & 0x04) {
		outportb(mmaIoPort, 0x0E);
		outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
		inportb(mmaIoPort + 1);
		outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
       }

   /*******************************************************************
    *       		           	Overrun
    *******************************************************************/

   if (mmaStatus & 0x80) {
		CtSetMMAReg0DBits(0x0A);	// Reset MIDI circuit
		CtResetMMAReg0DBits(0x0A);
       }
	}
/*****************************************************************************/


/*
 *	Synopsis:		WORD	SetControlDriverAddress(WORD destPort)
 *
 *	Description:	The gold card can be relocated with a software command. To
 *					make sure we get back to it the user can specify where
 *					the card should be found. This address is specified on the
 *					command line when installing the control driver.
 *
 *	Argument:		WORD	destPort
 *
 *	Return Value:	1 if ok.
 */	

PUBLIC
WORD	CtSetControlDriverAddress(WORD destPort)
	{
	baseAddress = destPort;
	if (phantomControl) controlIoPort = destPort+ 2;
	else				controlIoPort = destPort + 8; 
	mmaIoPort      = destPort + 4;
	opl3IoPort     = destPort;

	return 1;
	}

/**************************************************************************/

/*
 *	This MMA register 0x0D is used by both the wave driver and the MIDI
 *	driver. This driver assume the responsability of the writing to it.
 */

BYTE		mmaReg0D = 0x00;

/*
 *	Some support has been added to share a common write-only register 
 *	between the WAve driver and the MIDI drive. The register is the
 *	MMA 0x0D.
 */


/*
 *	Synopsis:		WORD    CtGetMMAReg0D()
 *
 *	Description:	Return the content of register # 0x0D of MMA.
 *
 *	Argument:		none.
 *
 *	Return Value:	content of register.
 */

PUBLIC
WORD	CtGetMMAReg0D()
	{
	return((WORD)mmaReg0D);
	}

/*
 *	Synopsis:		WORD    CtSetMMAReg0DBits(BYTE serie)
 *
 *	Description:	Write into register # 0x0D of MMA. The bit set in the serie
 *					are set to 1 in the MMA register.
 *
 *	Argument:		All bits in the register to set to 1.
 *
 *	Return Value:	content of register after operation.
 */

PUBLIC
WORD	CtSetMMAReg0DBits(BYTE serie)
	{
	mmaReg0D |= serie;
	asm	pushf
	asm	cli
	outportb(mmaIoPort, 0x0D);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	outportb(mmaIoPort + 1, mmaReg0D);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	asm	popf
	return((WORD)mmaReg0D);
	}

/*
 *	Synopsis:		WORD    CtResetMMAReg0DBits(BYTE serie)
 *
 *	Description:	Write into register # 0x0D of MMA. All bit set in the
 *					mask will be cleared in the destination register.
 *
 *	Argument:		All bits in the register to reset to 0.
 *
 *	Return Value:	content of register after operation.
 */

PUBLIC
WORD	CtResetMMAReg0DBits(BYTE serie)
	{
	mmaReg0D &= ~serie;
	asm	pushf
	asm	cli
	outportb(mmaIoPort, 0x0D);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	outportb(mmaIoPort + 1, mmaReg0D);
	outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0); outportb(delayIO, 0);
	asm	popf
	return((WORD)mmaReg0D);
	}


WORD	Hexa(char *s)
	{
	int				len, v, c;
	unsigned	int	base = 1;
	unsigned	int	retVal = 0;


	len = strlen(s);
	if (! len) return(0);

	while (len--) {
		c = s[len];
		if (isxdigit(c)) {
			if ((c >= 'a') && (c <= 'f')) {
				v = 10 + (c - 'a');
				}
			else if ((c >= 'A') && (c <= 'F')) {
				v = 10 + (c - 'A');
				}
			else v = (c - '0');
			}
		retVal = retVal + (v * base);
		base *= 16;
		}
	return retVal;
	}

/*
 *	Synopsis:		int	InitControlDriver()
 *
 *	Description:	Initialisation of control driver.
 *
 *	Argument:		no arguments
 *
 *	Return Value:	0	no error
 *					1	error		no gold card
 */

WORD	InitControlDriver()
	{
	char	*s;

	/*
	 *	Must be executed in bit level because some functions requires
	 *	the port address of the MMA and OPL3
	 */

	s = getenv("GOLD");

	if (! ((s != NULL) AND (isxdigit(s[0])) AND 
	   (isxdigit(s[1])) AND (isxdigit(s[2])))) {
#if PRINTF
	   	printf("Control: Environment variable for Gold address not valid\n");
#endif
	   	s = "388";
		}

	CtSetControlDriverAddress(Hexa(s));

	if (!CtGetGoldCardPresence()) {
		return 1;
       }
						
	if (gssLevel == level1) CtSetRelocationAddress(Hexa(s));

	/*
	 * Disable ctrl chip interrupt for a moment while
	 * accessing it.
	 */

	if (gssLevel == level2) CtEnabDisabInterrupt(0);

	InitInterruptService(Hexa(s));

	/*
	 * Make sure that all the modules who have access to the MMA register
	 * be setto the same default values.
	 */

	CtSetMMAReg0DBits(0x3F); 
	CtResetMMAReg0DBits(0x0A); 

	/*
	 *	Enable ctrl chip interrupt
	 */
	if (gssLevel == level2) CtEnabDisabInterrupt(1);

	return(0);
	}

WORD CloseControlDriver(void)

	{
	RemoveInterruptService();
   return(0);
	}
	
