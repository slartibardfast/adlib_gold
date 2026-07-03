# `developer-toolkit-v1.01/installed/WAVE.C`

> UTF-8 rendering of a DOS-encoded (CP437 / CRLF) file. Byte-for-byte original: [`WAVE.C`](../../../disks/developer-toolkit-v1.01/installed/WAVE.C).

```c

/*
	WAVE.C

	MMA Sampling driver.

	Marc Savary, sept-91
*/

/***************************************************************************
 										Includes
 ***************************************************************************/
#include "stdio.h"
#include "stdlib.h"
#include "dos.h"

#include "global.h"
#include "dma.h"
#include "control.h"
#include "interr.H"
#include "wave.h"

/***************************************************************************
 										defines
 ***************************************************************************/

#define MAJOR_VER		0
#define MINOR_VER		1

#define DEVICE_CLOSED	1
#define DEVICE_INPUT	2
#define DEVICE_OUTPUT	4

#define STATUS_WORKING		0	/* device currently working */
#define STATUS_STOPPED		1	/* device stopped by user */
#define STATUS_PAUSED		2	/* device paused by user */
#define STATUS_OUT_OF_BUFFER 3	/* no more buffer while playing/recording */

#define WAVE_OUT_MAGIC		23917	/* magic number output */
#define WAVE_IN_MAGIC		23918	/* magic number input */

#define PLAY_FIFO_SELECT	5		/* flag enabled when less than 32 bytes in fifo */
#define PLAY_FIFO_SIZE		96		/* transfer size, play-back */
#define REC_FIFO_SELECT	3		/* interrupt when more than 64 bytes in FIFO */
#define REC_FIFO_SIZE		64		/* transfer size, record */

#define BREAK_LOOP		1
#define NO_BREAK		0

#define DEF_VOLUME		0xffffffffL
#define DEF_STEREO	WAVE_STEREO_CENTER

/***************************************************************************
 										macros
 ***************************************************************************/

#undef inportb
#undef outportb

#ifdef DRVRES
void        	_Cdecl outportb( int __portid, unsigned char __value );
unsigned char 	_Cdecl inportb( int __portid );

#define	outp(a,d)				outportb(a,(unsigned char)d)	
#define	inp(p)					inportb(p)		
#endif

#define IntOff()	{ _asm{ pushf } \
					_asm{ cli } \
					 }
#define IntOn()	{ _asm { popf }}

#define ResetSampling( n) { \
	WriteMMA( n, 9, 0x80);	\
	inp( 0x21);	/* delay */	\
	inp( 0x21);				\
	WriteMMA( n, 9, 0);		\
	}


/***************************************************************************
 										Structures
 ***************************************************************************/


typedef
	struct {
		Word	wMagic;			/* magic number to verify validity */
		Word deviceId;			/* the first entry of array is 0,
									the second entry is 1 */
		Word wDeviceMode;		/* device mode:
									DEVICE_CLOSED		closed
									DEVICE_INPUT		open for input
									DEVICE_OUTPUT		open for output
			
					*/
		Word wDeviceStatus;		/* device status:
								STATUS_WORKING device currently working
								STATUS_STOPPED device stopped by user
								STATUS_PAUSED device paused by user
								STATUS_OUT_OF_BUFFER no more buffer while playing/recording
								*/

		Word	format;			/* waveform format: 
									WAVE_FORMAT_ADPCM4
									WAVE_FORMAT_PCM8
									WAVE_FORMAT_PCM12
									WAVE_FORMAT_PCM16
								*/
		Word	stereo;			/* mono = 0, stereo = 1 */
		Word	freqCode;		/* frequency select #, from 0 to 3 incl. */
		Word	transferMode;	/*
									WAVE_TRANSF_POLLING
									WAVE_TRANSF_INTERRUPT
									WAVE_TRANSF_DMA
								*/
		Word	leftRight;		/* WAVE_STEREO_LEFT,
									WAVE_STEREO_CENTER,
									WAVE_STEREO_RIGHT
								*/

		int (far * dwCallBack)();	/* user call back procedure */
		DWord dwCallBackData;	/* user data for call back proc. */
		Word	wDmaNr;			/* if DMA mode, DMA channel # to use */

		LpWaveHdr lpBlockList;	/* list of data block to be processed */
		LpWaveHdr lpFrstLpBlk;	/* first looping block ptr */
		Word	wLoopCount;		/* loop down counter */
		Word	started;		/* flag to indicate that PCM/ADPCM has been started */
		long	bytesToProcess;	/* # of byte still to play/record for the
									current block */
		WaveDataP processData;	/* ptr to current data in play/record buffer */
		DWord	dmaTrSize;		/* last programmed DMA transfer size */
		DWord	dwVolume;		/* wave volume: high word == left channel,
									low word == right channel;
									if mono, only right channel is defined */
		DWord	position;		/* the total byte count processed since Open()... */
	} WDev, far * DevicePtr;

/***************************************************************************
 										Variables
 ***************************************************************************/


static WDev	near waveDevices[ 2];

static int int_index;				/* audio interrupt # (Gold #), -1 if none */
static unsigned char oldIrqMask;
static unsigned intCAddr;
static int near intAT;
static unsigned near mma_io;
static unsigned near ctrl_io;
static unsigned char mmaVolL;		/* MMA mixer volume */

static unsigned char near reg9[ 2], near regA[ 2], near regC[ 2];


/* code interne du MMA pour les frequence d'echantillonage en fonction
	du mode PCM ou ADPCM; -1 signifie que la combinaison est impossible */
								  /* 44  22  11   7   5   Khz */
static signed char freqsAdpcm[] = { -1,  0,  1,  2,  3 };		/* ADPCM */
static signed char freqsPcm[] =   {  0,  1,  2,  3, -1 };		/* PCM */



	/* harware interrupt #: */
static char ints[] = { 3, 4, 5, 7, 10, 11, 12, 15 };
	/* hardware interrupt mask for 8259 controler: */
static unsigned char intMasks[] = { 1 << 3, 1 << 4, 1 << 5, 1 << 7,
							1 << 10 -8, 1 << 11 -8, 1 << 12 -8, 1 << 15 -8};
	/* software interrupt #: */
static unsigned char vecs[] = { 11, 12, 13, 15, 0x72, 0x73, 0x74, 0x77 };

static	void (interrupt far * oldVect)();	/* vecteur intial */
static int volatile dmaStatus;


/***************************************************************************
							Function prototypes
 ***************************************************************************/
static CallbackProc	IrqAudioProc();
static void StopSampling(DevicePtr dev);
static void OutOfBuffSampling(DevicePtr dev);
static void PauseSampling(DevicePtr dev);
static void	StartPlaying(DevicePtr dev);
static void PlayPolling(DevicePtr dev);
static void PlayBlockPolling(DevicePtr dev);
static void StartPlayRecInt(DevicePtr dev, int play);
static void	RecordPolling(DevicePtr dev);
static void RecordBlockPolling(DevicePtr dev);
static int FindFreqCode(DWord freq);
static void PrepareForSampling(int channel, int freq, int play, int format, 
								       int dmaMode, int stereo);
static void TerminateSampling(DevicePtr dev);
static void DoInputInterrupt(DevicePtr dev);
static void	DoPlayInterrupt(DevicePtr dev);
static int DoDataDMA(DevicePtr dev);
static void	GoToNextBlock(DevicePtr dev, int breakFlag);
static void WriteMMA( int channel, int reg, int data);
static int  ReadMMA(int channel, int reg);
static int 	ReadStatusMM1(void );
static int	FIFO_int(Word wFifoNr);

/***************************************************************************
 										Implementation
 ***************************************************************************/

/*
	To be called once in order to execute low level Wave Driver
	initialisations.
*/
WORD InitWaveDriver()
	{
	unsigned char byte;
	int dmaNr;

   /*
    *  We must tell the control chip driver what is our entry routine
    *  address.
    */

	SetDriverCallback(ADLIB_WAVE_DRIVER_ID, IrqAudioProc);

	/* 
	 * Now, lets initialize our stuff 
	 */

	dmaStatus = 0;

	mma_io = CtGetRelocationAddress() + 4;
	if (mma_io == 4) mma_io = 0x38c;

	WriteMMA( 0, 8, 0x70);		/* STAND-BY off, T2 T1 T0 masked */
	WriteMMA( 0, 0xD, 0x35);	/* mask all in D register */

	ResetSampling( 0);
	ResetSampling( 1);

	inp( mma_io);				/* clear status flags */

	regC[ 0] = 0;
	regC[ 1] = 0;
	WriteMMA( 0, 0xC, regC[ 0]);		/* clear channel 0 register C */
	WriteMMA( 1, 0xC, regC[ 1]);		/* clear channel 1 register C */

	reg9[ 0] = 0x60;			/* device 0 au centre */
	reg9[ 1] = 0x60;			/* device 1 au centre */
	WriteMMA( 0, 9, reg9[ 0]);
	WriteMMA( 1, 9, reg9[ 1]);
	WriteMMA( 0, 10, 0xff);		/* volume maximum */
	WriteMMA( 1, 10, 0xff);		/* volume maximum */

	waveDevices[ 0].wDeviceMode = DEVICE_CLOSED;
	waveDevices[ 0].wDeviceStatus = STATUS_STOPPED;
	waveDevices[ 0].deviceId = 0;
	waveDevices[ 0].wMagic = 0;

	if (CtGetEnabDisabDMA0SampChan()) {
		dmaNr = CtGetDMA0ChannelSampChan();
		}
	else dmaNr = -1;

	waveDevices[ 0].wDmaNr = dmaNr;

	waveDevices[ 1].wDeviceMode = DEVICE_CLOSED;
	waveDevices[ 1].wDeviceStatus = STATUS_STOPPED;
	waveDevices[ 1].deviceId = 1;
	waveDevices[ 1].wMagic = 0;

	if (CtGetEnabDisabDMA1SampChan()) {
		dmaNr = CtGetDMA1ChannelSampChan();
		}
	else dmaNr = -1;

	waveDevices[ 1].wDmaNr = dmaNr;
   return(0);
	}



/*
	To be called before returning to DOS if InitWaveDriver() has
	been called.
*/
WORD CloseWaveDriver()
	{
	ResetSampling( 0);
	ResetSampling( 1);
	ResetDriverCallback(ADLIB_WAVE_DRIVER_ID);
   return(0);
	}


/*
	This routine is called by hardware (interrupt) to manage interrupt
	dispatching of Adllib Gold Card
*/

static CallbackProc	IrqAudioProc()
	{
	unsigned char 	mmaStatus;

	_asm	mov		mmaStatus, bl
   _asm	push	ds
	_asm	push	es

	_asm 	mov		ax, ds
	_asm 	mov		es, ax

	if (mmaStatus & 0x01) FIFO_int(0);
	else if(mmaStatus & 0x02)  FIFO_int(1);

	_asm pop es
   _asm pop ds
	}




/*
	This function open a specified waveform output device for playback.

	Use 'WaveOutGetNumDevs()' to determine the number of waveform output
	devices present in the system. The device id specified by 'wDeviceId'
	varies from 0 to one less than the number of devices present.

	The format of the call back function 'dwCallBack()' is
	"int CallBack( HWaveOut dev, LpWaveHdr block, DWord dwCallBackData)".
	This function is called by the driver each time it has finished
	with a buffer.
*/
Word WaveOutOpen( lphWaveOut, wDeviceId, lpFormat, dwCallBack,
					dwCallBackData, dwFlags)
	HWaveOut far * lphWaveOut;
	Word wDeviceId;		/* device number (channel number): 0 to 1 */
	LpWaveFormat lpFormat;
	CallbackOutPtr dwCallBack;
	DWord dwCallBackData;
	DWord dwFlags;	/* flags for opening device:
						WAVE_FORMAT_QUERY:
							if this flag is specified, the device driver will
							determine if it supports the given format, but will
							not actually open the device. */
	{
	DevicePtr dev;
	unsigned freq;
	int code;

	if( wDeviceId > 1)
		return WERR_BADDEVICEID;
	dev = &waveDevices[ wDeviceId];
	if( dev->wDeviceMode != DEVICE_CLOSED)
		return WERR_ALLOCATED;
	if( lpFormat->nChannels > 1)
		if( wDeviceId != 0)
			return WERR_STEREOBADCHANNEL;
		else if( waveDevices[ 1].wDeviceMode != DEVICE_CLOSED)
			return WERR_STEREONEED2FREECHNL;

	freq = FindFreqCode( lpFormat->samplingFreq);
	if( freq > WAVE_LAST_FREQ)
		return WERR_UNSUPPORTEDFORMAT;
	dev->format = lpFormat->wFormatTag;
	if( (unsigned)dev->format > WAVE_LAST_FORMAT)
		return WERR_UNSUPPORTEDFORMAT;
	dev->stereo = lpFormat->nChannels > 1 ? 1 : 0;
	if( dev->format == WAVE_FORMAT_ADPCM4)
		code = freqsAdpcm[ freq];
	else
		code = freqsPcm[ freq];
	if( code == -1)
		return WERR_UNSUPPORTEDFORMAT;
	if( lpFormat->wTransMode > WAVE_LAST_TRANSF)
		return WERR_BADTRANSFERMODE;
	if( dwFlags & WAVE_FORMAT_QUERY)
		return 0;	/* Just return succes code */

	dev->transferMode = lpFormat->wTransMode;
	dev->freqCode = code;
	dev->wDeviceMode = DEVICE_OUTPUT;
	dev->wDeviceStatus = STATUS_OUT_OF_BUFFER;
	dev->dwCallBack = dwCallBack;
	dev->dwCallBackData = dwCallBackData;
	dev->lpBlockList = NULL;
	dev->wLoopCount = 0;
	dev->wMagic = WAVE_OUT_MAGIC;
	dev->dmaTrSize = 0;
	dev->position = 0;

	WaveOutSetVolume( dev, DEF_VOLUME);
	WaveOutSetLeftRight( dev, DEF_STEREO);
	*lphWaveOut = dev;
	return 0;
	}




/*
	This function closes the specified waveform output device.

	If the device is still playing a waveform, the close operation will fail.
	Use WaveOutReset() to terminate waveform playback before calling 
	WaveOutClose().
*/
Word WaveOutClose( hWaveOut)
	HWaveOut hWaveOut;
	{
	DevicePtr wDev;

	wDev = hWaveOut;
	if( wDev->wMagic != WAVE_OUT_MAGIC)
		return WERR_INVALIDHANDLE;
	if( wDev->lpBlockList != NULL)
		return WERR_STILLPLAYING;

	wDev->wDeviceMode = DEVICE_CLOSED;
	wDev->wMagic = 0;
	return 0;
	}



/*
	This function retrieves the number of waveform output devices present
	in the system.
*/
Word WaveOutGetNumDevs()
	{
	return 2;
	}



/*
	This function sends a data block to the specified waveform
	output device.

	Unless the devices is paused by calling 'WaveOutPause()', playback
	begin when the first data block is esnt to the device.
*/
Word WaveOutWrite( hWaveOut, lpWaveOutHdr, wSize)
	HWaveOut hWaveOut;		/* Handle to a opened waveform device */
	LpWaveHdr lpWaveOutHdr;	/* far ptr to a 'WaveHdr' structure containing
								information about the data block */
	Word wSize;			/* specifies the size of the 'WaveHdr' structure */
	{
	DevicePtr dev;
	LpWaveHdr prev, wave;

	(void)wSize;

	dev = hWaveOut;
	if( dev->wMagic != WAVE_OUT_MAGIC)
		return WERR_INVALIDHANDLE;

	IntOff();
	if( dev->lpBlockList == NULL) {
		dev->lpBlockList = lpWaveOutHdr;
		dev->bytesToProcess = dev->lpBlockList->dwBufferLength;
		dev->processData = dev->lpBlockList->lpData;
		}
	else {
       for( prev = wave = dev->lpBlockList; wave != NULL;
										prev = wave, wave = wave->lpNext)
			;
		prev->lpNext = lpWaveOutHdr;
		}
	lpWaveOutHdr->lpNext = NULL;
	IntOn();

	if( dev->wDeviceStatus == STATUS_OUT_OF_BUFFER)
		StartPlaying( dev);
	return 0;
	}



/*
	This function sets the volume of a waveform output device.
*/
Word WaveOutSetVolume( hWaveOut, dwVolume)
	HWaveOut hWaveOut;		/* Identifies the waveform output device */
	DWord dwVolume;			/*
							Specifies the new volume setting. The high order
							word contains the left channel setting, and the
							low order word contains the right channel setting.
							0xffff represent full volume, 0 is silence. If a
							device does not support both left and right volume,
							only the right channel value is used.
							*/
	{
	DevicePtr wDev;
	
	wDev = hWaveOut;
	if( wDev->wMagic != WAVE_OUT_MAGIC)
		return WERR_INVALIDHANDLE;

	wDev->dwVolume = dwVolume;
	if( !wDev->stereo) {
		regA[ wDev->deviceId] = dwVolume >> 8;
		WriteMMA( wDev->deviceId, 0xA, regA[ wDev->deviceId]);
		}
	else {
		regA[ 0] = dwVolume >> 24;		/* left volume */
		regA[ 1] = dwVolume >> 8;		/* right volume */
		WriteMMA( 0, 0xA, regA[ 0]);
		WriteMMA( 1, 0xA, regA[ 1]);
		}
	return 0;
	}



/*
	This function queries the current volume setting of a waveform 
	output device.
*/
Word WaveOutGetVolume( hWaveOut, lpdwVolume)
	HWaveOut hWaveOut;
	LPDWord lpdwVolume;
	{
	DevicePtr wDev;
	
	wDev = hWaveOut;
	if( wDev->wMagic != WAVE_OUT_MAGIC)
		return WERR_INVALIDHANDLE;

	*lpdwVolume = wDev->dwVolume;
	return 0;
	}



/*
	This function select on which side the channel will
	output. This is possible only for monophonic channel. Stereophonic
	channel are always outputed on left & right.
*/	
Word WaveOutSetLeftRight( hWaveOut, leftRight)
	HWaveOut hWaveOut;
	Word leftRight;			/* WAVE_STEREO_LEFT,
								WAVE_STEREO_CENTER,
								WAVE_STEREO_RIGHT */
	{
	DevicePtr dev;
	unsigned char byte;
								/*  center left right */
	static unsigned char bits[] = { 0x60, 0x20, 0x40 };
	
	dev = hWaveOut;
	if( dev->wMagic != WAVE_OUT_MAGIC)
		return WERR_INVALIDHANDLE;

	if( leftRight > WAVE_LAST_STEREO)
		return WERR_BADPOSITION;

	IntOff();
	if( dev->stereo) {
		leftRight = WAVE_STEREO_CENTER;
		byte = reg9[ 0] & ~0x60;
		byte |= 0x20;
		reg9[ 0] = byte;
		WriteMMA( 0, 9, byte);

		byte = reg9[ 1] & ~0x60;
		byte |= 0x40;
		reg9[ 1] = byte;
		WriteMMA( 1, 9, byte);
		}
	else {
		byte = reg9[ dev->deviceId] & ~0x60;
		byte |= bits[ leftRight];
		reg9[ dev->deviceId] = byte;
		WriteMMA( dev->deviceId, 9, byte);
		}
	IntOn();
	dev->leftRight = leftRight;
	return 0;
	}


/*
	This function stops playback on a given waveform output device and reset
	the current position to 0. All pending playback buffers are marked as
	done and returned to the application.
*/
Word WaveOutReset( hWaveOut)
	HWaveOut hWaveOut;		/* Specifies a handle to the waveform output
							device that is to be reset */
	{
	DevicePtr wDev;
	LpWaveHdr block;
	
	wDev = hWaveOut;
	if( wDev->wMagic != WAVE_OUT_MAGIC)
		return WERR_INVALIDHANDLE;

	StopSampling( wDev);
	if( wDev->transferMode == WAVE_TRANSF_DMA)
		/* mask DMA controler channel: */
		outp( dmaSingleMaskRegister, dmaSingleMaskSet | wDev->wDmaNr);

	/* mark data buffer as DONE and return to application: */
	for( block = wDev->lpBlockList; block != NULL; block = block->lpNext) {
		block->dwFlags |= WHDR_DONE;
		(*wDev->dwCallBack)( (HWaveOut)wDev, block, wDev->dwCallBackData);
		}
	wDev->lpBlockList = NULL;

	return 0;
	}



/*
	This function breaks a loop on a given waveform output device and allow
	playback to continue with the next block in the driver list.
*/
Word WaveOutBreakLoop( hWaveOut)
	HWaveOut hWaveOut;
	{
	DevicePtr dev;
	LpWaveHdr block;
	
	dev = hWaveOut;
	if( dev->wMagic != WAVE_OUT_MAGIC)
		return WERR_INVALIDHANDLE;
	if( dev->wLoopCount == 0)		/* nothing is looping */
		return 0;

	IntOff();

	TerminateSampling( dev);
	GoToNextBlock( dev, BREAK_LOOP);
	StartPlaying( dev);

	IntOn();
	return 0;
	}



/*
	This function pauses playback on the specified waveform output device.
	The current playback position is saved. Use WaveOutRestart() to resume
	playback from the current playback position.

	Calling this function when the output is already paused will have no
	effect 0 will be returned.
*/
Word WaveOutPause( hWaveOut)
	HWaveOut hWaveOut;
	{
	DevicePtr dev;
	Word count;
	int dmaOff;
	
	dev = hWaveOut;
	if( dev->wMagic != WAVE_OUT_MAGIC)
		return WERR_INVALIDHANDLE;
	if( dev->wDeviceStatus == STATUS_WORKING)
		PauseSampling( dev);

	return 0;
	}




/*
	This function restarts a paused waveform output device.
*/
Word WaveOutRestart( hWaveOut)
	HWaveOut hWaveOut;
	{
	DevicePtr dev;
	
	dev = hWaveOut;
	if( dev->wMagic != WAVE_OUT_MAGIC)
		return WERR_INVALIDHANDLE;
	if( dev->wDeviceStatus == STATUS_PAUSED)
		StartPlaying( dev);
	return 0;
	}



/*
	This function sends an input buffer to a waveform input device. When
	the buffer is filled, it is sent back to the application.
*/
Word WaveInAddBuffer( hWaveIn, lpWaveInHdr, wSize)
	HWaveIn hWaveIn;	/* Handle to the input waveform device */
	LpWaveHdr lpWaveInHdr;	/* far ptr to struct. that identifies buffer */
	Word wSize;			/* Specifies the size of the WaveHdr struture */
	{
	DevicePtr dev;
	LpWaveHdr wave;

	(void) wSize;	
	dev = hWaveIn;
	if( dev->wMagic != WAVE_IN_MAGIC)
		return WERR_INVALIDHANDLE;

	IntOff();
	if( dev->lpBlockList == NULL) {
		dev->lpBlockList = lpWaveInHdr;
		dev->bytesToProcess = dev->lpBlockList->dwBufferLength;
		dev->processData = dev->lpBlockList->lpData;
		}
	else {
		for(wave = dev->lpBlockList; wave->lpNext != NULL; wave= wave->lpNext);
		wave->lpNext = lpWaveInHdr;
		}
	lpWaveInHdr->lpNext = NULL;
	lpWaveInHdr->dwBytesRecorded = 0;
	IntOn();

	if( dev->wDeviceStatus == STATUS_OUT_OF_BUFFER)
		return WaveInStart( hWaveIn);
	return 0;
	}



/*
	This function close the specified waveform input device.

	If there are input buffers that have been sent with WaveInAddBuffer(),
	and haven't been returned to the application, the close operstion will
	fail. Call WaveInReset() to mark all pending buffers as done.
*/	 
Word WaveInClose( hWaveIn)
	HWaveIn hWaveIn;
	{
	DevicePtr dev;
	
	dev = hWaveIn;
	if( dev->wMagic != WAVE_IN_MAGIC)
		return WERR_INVALIDHANDLE;
	if( dev->lpBlockList != NULL)
		return WERR_STILLPLAYING;

	dev->wDeviceMode = DEVICE_CLOSED;
	dev->wMagic = 0;
	return 0;
	}



/*
	This function queries a specified waveform input device to determine
	its capabilities.

	Use WaveInGetNumDevs to determine the number of waveform input devices
	present in the system. The device ID specified by wDeviceID varies from
	zero to one less than the number of devices present. Only wSize bytes
	(or less) of information will be copied to the location pointed to by
	lpCaps.
*/
Word WaveInGetDevCaps( wDeviceID, lpCaps, wSize)
	Word wDeviceID;	/* waveform input devive to be queried */
	LpWaveInCaps lpCaps;	/* Ptr to a WaveInCaps structure. This structure
							is filled with information about teh capabilities
							of the device. */
	Word wSize;		/* size of the WaveInCaps structure */
	{
	WaveInCaps wc;

	if( wDeviceID > 1)
		return WERR_BADDEVICEID;
	wc.wMid = 0;
	wc.wPid = 0;
	wc.vDriverVersion = (MAJOR_VER << 8) + MINOR_VER;
	strcpy( wc.szPname, "ADLIB-GOLD");
	wc.dwFormats = -1;
	wc.wChannels = 2;
	memmove( (char *)lpCaps, (char *) &wc, min( wSize, sizeof wc));
	return 0;
	}



/*
	This function return the number of waveform input device.
*/
Word WaveInGetNumDevs()
	{
	return 2;
	}
	



/*
	This function returns the number of samples recorded since last WaveInOpen
	or WaveInReset()...
*/
Word WaveInGetPosition( hWaveIn, wavePos)
	HWaveIn hWaveIn;
	LPDWord wavePos;
	{
	DevicePtr dev;
	long pos, delta, samples;
	int dmaOff;
	unsigned count;
	
	dev = hWaveIn;
	if( dev->wMagic != WAVE_IN_MAGIC)
		return WERR_INVALIDHANDLE;

	pos = dev->position;
	delta = 0;
	if( dev->wDeviceStatus == STATUS_WORKING) {
		if( dev->transferMode == WAVE_TRANSF_INTERRUPT)
			delta = dev->lpBlockList->dwBufferLength - dev->bytesToProcess;
		else if( dev->transferMode == WAVE_TRANSF_DMA) {
			dmaOff = dev->wDmaNr << 1;
			IntOff();
			outp( dmaBytePointerRegister, 0);	/* clear byte pointer */
			count = inp( dmaWordCountRegister +dmaOff);	/* get low byte count down reg. */
			count += inp( dmaWordCountRegister +dmaOff) << 8;	/* high byte c. d. reg */
			count++;
			delta = dev->dmaTrSize - count;
			IntOn();
			}
		}
	pos += delta;

	switch( dev->format) {
		case WAVE_FORMAT_ADPCM4:
				samples = pos << 1;
				break;
		case WAVE_FORMAT_PCM8:
				samples = pos;
				break;
		case WAVE_FORMAT_PCM12:
		case WAVE_FORMAT_PCM16:
				samples = pos >> 1;
		}
	*wavePos = samples;

	return 0;
	}




/*
	This function opens a specified waveform input device for recording.

	Use WaveInGetNumDevs() to determine the number of wavefomr input devices
	present in the system. THe device ID specified by wDeviceID varies from
	zero to one less the number of devices present.

*/
Word WaveInOpen( lphWaveIn, wDeviceID, lpFormat, dwCallBack, dwCallBackData,
				dwFlags)
	HWaveIn far * lphWaveIn;	/* ptr to ptr. This location is filled with
							a handle identifying the opened waveform input
							device. */
	Word wDeviceID;			/* Identifies the waveform input device to be
							opened. */
	LpWaveFormat lpFormat;	/* Specifies a ptr to a WaveFormat data structure
							that identifies the desired format for recording
							waveform data. */
	int (far *dwCallBack)();/* Specifies the address of a callback function
							that is called during waveform recording to precess
							messages related to the progress of recording. */
	DWord dwCallBackData;		/* Specifies a 32 bits user data that is passed
							to the callback function. */
	DWord dwFlags;			/* Specifies flags for opening device:

							WAVE_FORMAT_QUERRY:	If this flag is specified, the
								device will determine if it supports the given
								format, but will not actually open the device.
							*/
	{
	DevicePtr dev;
	int code;
	unsigned freq;

	if( wDeviceID > 1)
		return WERR_BADDEVICEID;
	dev = &waveDevices[ wDeviceID];
	if( dev->wDeviceMode != DEVICE_CLOSED)
		return WERR_ALLOCATED;
	if( lpFormat->nChannels > 1)
		if( wDeviceID != 0)
			return WERR_STEREOBADCHANNEL;
		else if( waveDevices[ 1].wDeviceMode != DEVICE_CLOSED)
			return WERR_STEREONEED2FREECHNL;

	freq = FindFreqCode( lpFormat->samplingFreq);
	if( freq > WAVE_LAST_FREQ)
		return WERR_UNSUPPORTEDFORMAT;
	dev->format = lpFormat->wFormatTag;
	if( (unsigned)dev->format > WAVE_LAST_FORMAT)
		return WERR_UNSUPPORTEDFORMAT;
	dev->stereo = lpFormat->nChannels > 1 ? 1 : 0;
	if( dev->format == WAVE_FORMAT_ADPCM4)
		code = freqsAdpcm[ freq];
	else
		code = freqsPcm[ freq];
	if( code == -1)
		return WERR_UNSUPPORTEDFORMAT;
	if( !( lpFormat->wTransMode == WAVE_TRANSF_INTERRUPT
	|| lpFormat->wTransMode == WAVE_TRANSF_DMA))
		return WERR_BADTRANSFERMODE;
	if( dwFlags & WAVE_FORMAT_QUERY)
		return 0;	/* Just return succes code */

	dev->freqCode = code;
	dev->transferMode = lpFormat->wTransMode;
	dev->wDeviceMode = DEVICE_INPUT;
	dev->wDeviceStatus = STATUS_STOPPED;
	dev->dwCallBack = dwCallBack;
	dev->dwCallBackData = dwCallBackData;
	dev->lpBlockList = NULL;
	dev->wLoopCount = 0;
	dev->wMagic = WAVE_IN_MAGIC;
	dev->dmaTrSize = 0;
	dev->position = 0;
	*lphWaveIn = dev;
	return 0;
	}
								


/*
	This function stops input on a given waveform input device and resets
	the current position to 0. All pending buffers are marked done and
	returned to the application.
*/
Word WaveInReset( hWaveIn)
	HWaveIn hWaveIn;
	{
	DevicePtr dev;
	LpWaveHdr block;
	Word count, recorded;
	int dmaOff;

	dev = hWaveIn;
	if( dev->wMagic != WAVE_IN_MAGIC)
		return WERR_INVALIDHANDLE;

	StopSampling( dev);

	/* mark data buffer as DONE and return to application: */
	for( block = dev->lpBlockList; block != NULL; block = block->lpNext) {
		block->dwFlags |= WHDR_DONE;
		(*dev->dwCallBack)( (HWaveOut)dev, block, dev->dwCallBackData);
		}
	dev->lpBlockList = NULL;
	dev->position = 0;

	return 0;
	}




/*
	This function starts input on a specified waveform input device.

	Buffers are returned to application when full or when WaveInReset()
	is called. "dwBytesRecorded" field in the header will contain the actual
	length of data. If there are no buffers in the queue, the data is
	thrown away without notification to the application and input will
	continue. Calling this function when input is already started will
	have no effect.
*/
Word WaveInStart( hWaveIn)
	HWaveIn hWaveIn;		/* Ptr to the waveform input device to be started */
	{
	DevicePtr dev;

	dev = hWaveIn;
	if( dev->wMagic != WAVE_IN_MAGIC)
		return WERR_INVALIDHANDLE;

	dev->wDeviceStatus = STATUS_WORKING;
	dev->started = 0;
	dev->bytesToProcess = dev->lpBlockList->dwBufferLength;
	PrepareForSampling( dev->deviceId, dev->freqCode, 0, dev->format,
						dev->transferMode == WAVE_TRANSF_DMA, dev->stereo);
	switch( dev->transferMode) {
		case WAVE_TRANSF_DMA:
			DoDataDMA( dev);
			break;
		case WAVE_TRANSF_INTERRUPT:
			StartPlayRecInt( dev, 0);
			break;
		default:	/* WAVE_TRANSF_POLLING */
			RecordPolling( dev);
			break;
		}	

	return 0;
	}




/*	The next function may be called by interrupt, so disable stack checking...*/
#pragma check_stack( off)

static 
void StopSampling(DevicePtr dev)

	{
	dev->wDeviceStatus = STATUS_STOPPED;
	TerminateSampling( dev);
	}


/*
	Arreter le sampling, et mettre le status a "STATUS_OUT_OF_BUFFER"
*/
static 
void OutOfBuffSampling(DevicePtr dev)
	{
	dev->wDeviceStatus = STATUS_OUT_OF_BUFFER;
	TerminateSampling( dev);
	}



#pragma check_stack()



static 
void PauseSampling(DevicePtr dev)
	{
	dev->wDeviceStatus = STATUS_PAUSED;
	TerminateSampling( dev);
	}


static void
StartPlaying(DevicePtr dev)
	{
	dev->wDeviceStatus = STATUS_WORKING;
	PrepareForSampling( dev->deviceId, dev->freqCode, 1, dev->format,
						dev->transferMode == WAVE_TRANSF_DMA, dev->stereo);
	switch( dev->transferMode) {
		case WAVE_TRANSF_DMA:
			DoDataDMA( dev);
			break;
		case WAVE_TRANSF_INTERRUPT:
			StartPlayRecInt( dev, 1);
			break;
		default:	/* WAVE_TRANSF_POLLING */
			PlayPolling( dev);
			break;
		}	
	}



/*

*/
static 
void PlayPolling(DevicePtr dev)
	{
	while( dev->lpBlockList != NULL) {
		PlayBlockPolling( dev);
		GoToNextBlock( dev, NO_BREAK);
		}

	OutOfBuffSampling( dev);
	}



static 
void PlayBlockPolling(DevicePtr dev)
	{
	WaveDataP pData;
	LpWaveHdr lpBlock;
	WaveSize dwBLen;
	Word channel;
	unsigned char data;
	int i, stat;

	lpBlock = dev->lpBlockList;
	channel = dev->deviceId;
	pData = lpBlock->lpData;
	dwBLen = lpBlock->dwBufferLength;

	if( !dev->started) {
		/* send 128 first bytes of signal: */
		for( i = 0; i < 128 && i < dwBLen; i++) {
			WriteMMA( channel, 0xB, *pData++);
			dwBLen--;
			if( dev->stereo) {
				WriteMMA( 1, 0xB, *pData++);
				dwBLen--;
				}
			}
		IntOff();
		reg9[ channel] |= 1;
		if( dev->stereo) {
			reg9[ 1] |= 1;
			WriteMMA( 1, 9, reg9[ 1]);
			}
		WriteMMA( channel, 9, reg9[ channel]);
		regC[ channel] &= ~2;		/* unmask flag */
		WriteMMA( channel, 0xC, regC[ channel]);
		IntOn();
		dev->started = 1;
		}

	while( dwBLen > 0) {
		stat = ReadStatusMM1();
		if( (stat & (1 << channel))) {	/* FIFO flag */
GetControlRegister(-1);
			for( i = 0; i < PLAY_FIFO_SIZE & dwBLen > 0; i++) {
				if( dev->stereo) {
					WriteMMA( 0, 0xB, *pData++);
					WriteMMA( 1, 0xB, *pData++);
					dwBLen -= 2;
					}
				else {
					WriteMMA( channel, 0xB, *pData++);
					dwBLen--;
					}
				}
			}
		}
	}






/*
	Start le sampling (ADP-ST = 1). Appele la routine d'interruption
	pour transferer la premiere batch de samples. Ensuite, l'interrupt
	genere par le MMA fera le reste.

	prepareForSampling() doit etre appele avant cette fonction.

	play specifies playback if 1, record if 0.
*/
static 
void StartPlayRecInt(DevicePtr dev, int play)
	{
	unsigned smpChnl;

	smpChnl = dev->deviceId;

	if( play)
		DoPlayInterrupt( dev);

	IntOff();
	reg9[ smpChnl] |= 1;
	if( dev->stereo) {
		reg9[ 1] |= 1;
		WriteMMA( 1, 9, reg9[ 1]);
		}
	WriteMMA( smpChnl, 9, reg9[ smpChnl]);

	regC[ smpChnl] &= ~2;		/* unmask FIFO */
	WriteMMA( smpChnl, 0xC, regC[ smpChnl]);

	dev->started = 1;
	IntOn();
	}





/*
	Cette fonction enregistre par polling tous les block contenus
	dans la liste du driver, et les retourne un a un a l,'application,
	au fur et a mesure de leur remplissage.
*/
static void
RecordPolling(DevicePtr dev)
	{

	while( dev->lpBlockList != NULL) {
		RecordBlockPolling( dev);
		GoToNextBlock( dev, NO_BREAK);
		}

	OutOfBuffSampling( dev);
	}



static 
void RecordBlockPolling(DevicePtr dev)
	{
	WaveDataP rData;
	LpWaveHdr lpBlock;
	WaveSize dwBLen;
	Word channel;
	unsigned char data;
	int i, stat;

	lpBlock = dev->lpBlockList;
	channel = dev->deviceId;
	rData = lpBlock->lpData;
	dwBLen = lpBlock->dwBufferLength;

	if( !dev->started) {
		IntOff();
		reg9[ channel] |= 1;
		if( dev->stereo) {
			reg9[ 1] |= 1;
			WriteMMA( 1, 9, reg9[ 1]);
			}
		WriteMMA( channel, 9, reg9[ channel]);
		IntOn();
		dev->started = 1;
		}

	while( dwBLen > 0) {
		stat = ReadStatusMM1();
		if( stat & 0x80) {
/*			printf( "\nOverrun!!! -record");		*/
			break;
			}
		else if( (stat & (1 << channel))) {	/* FIFO flag */
			for( i = 0; i < PLAY_FIFO_SIZE & dwBLen > 0; i++) {
				if( dev->stereo) {
					*rData++ = ReadMMA( 0, 0xB);
					*rData++ = ReadMMA( 1, 0xB);
					dwBLen -= 2;
					}
				else {
					*rData++ = ReadMMA( channel, 0xB);
					dwBLen--;
					}
				}
			}
		}
	}




/*
	Determine le code de frequence correspondant pour le chip MMA:

	retourne WAVE_FREQ44 ... WAVE_FREQ5
*/
static int FindFreqCode(DWord freq)
	{						 /* 44K    22K   11K    7K, 5 K */
	static DWord ranges[] = { 33075L, 16537, 9987, 6237 };
	int i;

	for( i = 0; i < 4; i++)
		if( freq >= ranges[ i])
			break;
	return i;
	}


	

/*
	INTERRUPT ROUTINES:

	These routines are subject to be called by interrupt.
*/


/* Disable stack checking for interrupt routines: */
#pragma check_stack( off)


/*
	Programmer le MMA pour le sampling (play & record). Il ne
	restera qu'a faire une start (ADP-ST) et demasker MSK-FIF si necessaire.
*/
static 
void PrepareForSampling(int channel, int freq, int play, int format, 
								int dmaMode, int stereo)
	{
	unsigned char byte, reg11;
	int i;

	ResetSampling( channel);
	if( stereo)
		ResetSampling( 1);

	byte = 2;		/* mask fifo */
	/* interrupt level selection: */
	byte |= play ? PLAY_FIFO_SELECT << 2 : REC_FIFO_SELECT << 2;
	byte |= ((format -1) & 3) << 5;		/* PCM format */
	if( dmaMode) {
		byte |= 1;
		if( stereo)
			byte |= 0x80;
		}
	regC[ channel] = byte;
	WriteMMA( channel, 0xC, regC[ channel]);
	for( i = 0; i < 8; i++)	/* pour eviter des problemes de dephasage 16 bits */
		WriteMMA( channel, 0xB, 0);
	if( stereo) {
		/* same as channel 0 but MSK-FIF = 1 & FIFO level select = 6 (16 bytes) */
		regC[ 1] = regC[ 0] | 2 | ( 6 << 2);
		WriteMMA( 1, 0xC, regC[ 1]);
		for( i = 0; i < 8; i++)	/* pour eviter des problemes de dephasage 16 bits */
			WriteMMA( 1, 0xB, 0);
		}

	reg9[ channel] &= ~0x9F;	/* clear: ADP-RST, FS1:FS0, PCM, PLY/REC-, ADP-ST */
	reg9[ channel] |= freq << 3;	/* set freq. bits */
	reg9[ channel] |= format == WAVE_FORMAT_ADPCM4 ? 0 : 4; /* PCM/ADPCM mode */
	if( play)
		reg9[ channel] |= 2;

	if( stereo) {
		reg9[ 1] = (reg9[ 0] & ~0x60) | (reg9[ 1] & 0x60);
		WriteMMA( 1, 9, reg9[ 1]);
		}
	WriteMMA( channel, 9, reg9[ channel]);

	if (stereo) CtStereoMonoAuxSamp(0);
	else CtStereoMonoAuxSamp(1);
	if (! play) {
		mmaVolL = CtGetMixerLevelForLeftSamplePb();
		CtSetMixerLevelForLeftSamplePb(0x80);
		CtSetChannel0FilterMode(1);
		CtSetChannel1FilterMode(1);
		}
	}



/*
	Termine le sampling (playback ou record) sur le canal de sampling
	correspondant au device 'dev'. S'il sagit d'un device stereo,
	les deux canaux sont stoppes.
*/
static 
void TerminateSampling(DevicePtr dev)
	{
	int channel;
	int dmaOff;
	unsigned count;
	long done;

	channel = dev->deviceId;

	IntOff();

	/* mask interrupt flag: */
	regC[ channel] |= 2;
	WriteMMA( channel, 0xC, regC[ channel]);

	/* stop MMA: */
	reg9[ channel] &= ~1;
	WriteMMA( channel, 9, reg9[ channel]);
	if( dev->stereo) {
		reg9[ 1] &= ~1;
		WriteMMA( 1, 9, reg9[ 1]);
		}
	IntOn();
	dev->started = 0;

	if( dev->transferMode == WAVE_TRANSF_DMA) {
	/* mask DMA controler channel: */
		outp( dmaSingleMaskRegister, dmaSingleMaskSet | dev->wDmaNr);
		dmaOff = dev->wDmaNr << 1;
	/* update byte count to process: */

		outp( dmaBytePointerRegister, 0);	/* clear byte pointer */
		count = inp( dmaWordCountRegister +dmaOff);	/* get low byte count down reg. */
		count += inp( dmaWordCountRegister +dmaOff) << 8;	/* high byte c. d. reg */
		count++;
		done = dev->dmaTrSize - count;
		dev->bytesToProcess -= done;
		if( DEVICE_INPUT == dev->wDeviceMode &&	dev->lpBlockList != NULL)
			dev->lpBlockList->dwBytesRecorded += done;
		dev->dmaTrSize = 0;
		dmaStatus |= inp( dmaStatusRegister);
		dmaStatus &= ~(1 << dev->wDmaNr);
		}

	if( DEVICE_INPUT == dev->wDeviceMode) {
		CtSetMixerLevelForLeftSamplePb(mmaVolL);
		CtSetChannel0FilterMode(0);
		CtSetChannel1FilterMode(0);
		}
	}




/*
	Cette fonction est responsable du transfert par interrupt
	en mode RECORD.
*/

static 
void DoInputInterrupt(DevicePtr dev)
	{
	int channel;
   int	dataReg;
   int count;
   int transferSize;
   char far *iData;

	channel = dev->deviceId;
	dataReg = mma_io + (channel ? 3: 1);
  
	/* Calculate the number of bytes to read */
	transferSize = REC_FIFO_SIZE;
   if (dev->stereo) transferSize <<= 1;
   if (transferSize > dev->bytesToProcess)
       transferSize = dev->bytesToProcess;

	// set fifo level to 0 temporary, to avoid false interrupt
   _asm	pushf   /*** DEBUG ***/
   _asm	cli     /*** DEBUG ***/
   outp(mma_io, 0x0C);
   outp(dataReg, regC[channel] | (0x07 << 2)); // select level 0 bytes 

	// Empty the MMA Buffer		
	iData = (char far *)dev->processData;
   outp(mma_io, 0x0B);

	// This case for 8-bit transfers
	if (dev->format  < WAVE_FORMAT_PCM12) {
   	if (dev->stereo) {
           count = transferSize >> 1;
       	while (count-=2) {
           	*iData++ = inp(dataReg);
               *iData++ = inp(dataReg + 2);
               }
			}
		else {
           count = transferSize;
			while(count--) *iData++ = inp(dataReg);
           }
		}
	// This case for 16 bit transfers
	else { /* dev->format  >= WAVE_FORMAT_PCM12 */
   	if (dev->stereo) {
           count = transferSize >> 2;
       	while (count--) {
           	*iData++ = inp(dataReg);
           	*iData++ = inp(dataReg);
               *iData++ = inp(dataReg + 2);
               *iData++ = inp(dataReg + 2);
               }
			}
		else {
           count = transferSize >> 1;
			while(count--) {
				*iData++ = inp(dataReg); 
				*iData++ = inp(dataReg);
               }
           }
		}

	// Update pointers

	dev->processData = iData;
	dev->lpBlockList->dwBytesRecorded += transferSize;
	dev->bytesToProcess -= transferSize;
		
	// restore interrupts
	_asm	popf    

	// Check if we need to prepare next block

	if( dev->bytesToProcess <= 0) {
		GoToNextBlock( dev, NO_BREAK);
		if( dev->lpBlockList == NULL) {
			OutOfBuffSampling( dev);
			}
       }


   _asm	pushf
   _asm	cli
   outp(mma_io, 0x0C);
   outp(dataReg, regC[channel]); // restore original FIFO level
   _asm 	popf
	}


static void
DoPlayInterrupt(DevicePtr dev)
	{
	int channel;
   int	dataReg;
   int count;
   int transferSize;
   char far *iData;

	channel = dev->deviceId;
	dataReg = mma_io + (channel ? 3: 1);
  
	/* Calculate the number of bytes to read */
	transferSize = PLAY_FIFO_SIZE;
   if (dev->stereo) transferSize <<= 1;
   if (transferSize > dev->bytesToProcess)
       transferSize = dev->bytesToProcess;

	// set fifo level to 0 temporary, to avoid false interrupt
   _asm	pushf   /*** DEBUG ***/
   _asm	cli     /*** DEBUG ***/
   outp(mma_io, 0x0C);
   outp(dataReg, regC[channel] | (0x07 << 2)); // select level 0 bytes 

	// Fill the MMA Buffer		
	iData = (char far *)dev->processData;
   outp(mma_io, 0x0B);

	// This case for 8-bit transfers
	if (dev->format  < WAVE_FORMAT_PCM12) {
   	if (dev->stereo) {
           count = transferSize >> 1;
       	while (count-=2) {
           	outp(dataReg, *iData++);
           	outp(dataReg+2, *iData++);
               }
			}
		else {
           count = transferSize;
			while(count--) outp(dataReg, *iData++);
           }
		}
	// This case for 16 bit transfers
	else { /* dev->format  >= WAVE_FORMAT_PCM12 */
   	if (dev->stereo) {
           count = transferSize >> 2;
       	while (count--) {
           	outp(dataReg, *iData++);
           	outp(dataReg, *iData++);
           	outp(dataReg+2, *iData++);
           	outp(dataReg+2, *iData++);
               }
			}
		else {
           count = transferSize >> 1;
			while(count--) {
           	outp(dataReg, *iData++);
           	outp(dataReg, *iData++);
               }
           }
		}

	// Update pointers

	dev->processData = iData;
	dev->bytesToProcess -= transferSize;
		
	// restore interrupts
	_asm	popf    

	// Check if we need to prepare next block

	if( dev->bytesToProcess <= 0) {
		GoToNextBlock( dev, NO_BREAK);
		if( dev->lpBlockList == NULL) {
			OutOfBuffSampling( dev);
			}
       }


   _asm	pushf
   _asm	cli
   outp(mma_io, 0x0C);
   outp(dataReg, regC[channel]); // restore original FIFO level
   _asm 	popf
	}



/*
	Cette routine, appelee par interruption, gere l'ecriture et la lecture
	de samples par DMA.
*/
static int 
DoDataDMA(DevicePtr dev)
	{
	int i;
	int cmdByte, page;
	unsigned char stat;
	unsigned long addr;
	unsigned int offset;
	int dmaChannel, dmaChnlOffset;
	char far * ptr;
	unsigned int size;		/* DMA transfer size */
   unsigned char highByte;
	unsigned long longSize;
	int smpChannel;

	static unsigned char dmaPages[] = {
		dmaPageRegister0, dmaPageRegister1, dmaPageRegister2, dmaPageRegister3 };

	dev->bytesToProcess -= dev->dmaTrSize;
	dev->processData += dev->dmaTrSize;

	if( dev->wDeviceMode == DEVICE_INPUT)
		dev->lpBlockList->dwBytesRecorded += dev->dmaTrSize;
	dev->dmaTrSize = 0;
	if( dev->bytesToProcess <= 0) {
		GoToNextBlock( dev, NO_BREAK);
		if( dev->lpBlockList == NULL) {
			OutOfBuffSampling( dev);
			return 0;
			}
		}

	ptr = (char far *)dev->processData;
	addr = ((unsigned long)FP_SEG( ptr) << 4) + FP_OFF( ptr);
	offset = addr & 0xffff;
	longSize = (long)~offset +1;		/* from start addr to end of page */
	if( longSize > dev->bytesToProcess) 
		longSize = dev->bytesToProcess;
	size = longSize -1;
	dev->dmaTrSize = longSize;

	page = addr >> 16;
	dmaChannel = dev->wDmaNr;
	dmaChnlOffset = dmaChannel << 1;

	/* disable dma controler: */

	outp( dmaSingleMaskRegister, dmaSingleMaskSet | dmaChannel);

	/* Program & start DMA controler: */
	cmdByte = dmaModeSingleTransfer | dmaChannel;
	cmdByte |= (dev->wDeviceMode == DEVICE_OUTPUT) ? dmaModeCycleRead : dmaModeCycleWrite;
	outp( dmaModeRegister, cmdByte);
	outp( dmaBytePointerRegister, 0);
	outp( dmaBaseAddressRegister +dmaChnlOffset, offset); 

   highByte = ((unsigned int)offset) >> 8;
	outp( dmaBaseAddressRegister +dmaChnlOffset, highByte); 
	outp( dmaPages[ dmaChannel], page);
	outp( dmaWordCountRegister +dmaChnlOffset, size); 

   highByte = ((unsigned int)size) >> 8;
	outp( dmaWordCountRegister +dmaChnlOffset, highByte); 

	/* Enable DMA controler... */
	outp( dmaSingleMaskRegister, dmaChannel);

	if( !dev->started) {
		IntOff();
		smpChannel = dev->deviceId;
		reg9[ smpChannel] |= 1;			/* ADP start = ON */
		WriteMMA( smpChannel, 9, reg9[ smpChannel]);

		regC[ smpChannel] &= ~2;		/* unmask FIFO */
		WriteMMA( smpChannel, 0xC, regC[ smpChannel]);

		dev->started = 1;
		IntOn();
		}
	return 1;		/* OK */
	}



/*
	Le block courant (lpBlockList) vient d'etre termine. On retourne
	a l'application le block, puis on avance au prochain.
	Tient compte des flags de looping. 
*/
static void
GoToNextBlock(DevicePtr dev, int breakFlag)
	/* breakFlag;		 termine la 'loop' si vrai */
	{
	DWord flags;
	LpWaveHdr block;
	int ok;
	int inputMode;

	flags = dev->lpBlockList->dwFlags;
	inputMode = dev->wDeviceMode == DEVICE_INPUT;
	if( inputMode)
		flags &= ~(WHDR_BEGINLOOP | WHDR_ENDLOOP);

	if( flags & WHDR_BEGINLOOP) {
		if( dev->wLoopCount == 0) {
			dev->wLoopCount = dev->lpBlockList->dwLoops;
			dev->lpFrstLpBlk = dev->lpBlockList;
			}
		}

	if( flags & WHDR_ENDLOOP || breakFlag) {
		if( --dev->wLoopCount > 0 && !breakFlag)
			dev->lpBlockList = dev->lpFrstLpBlk;
		else {
			for( ok = 1, block = dev->lpFrstLpBlk; ok; block = block->lpNext) {
				block->dwFlags |= WHDR_DONE;
				(*dev->dwCallBack)( (HWaveOut)dev, block, dev->dwCallBackData);
				ok = !(block->dwFlags & WHDR_ENDLOOP);
				}
			dev->lpBlockList = block;
			dev->wLoopCount = 0;
			}
		}
	else {
		if( dev->wLoopCount == 0) {
			dev->lpBlockList->dwFlags |= WHDR_DONE;
			(*dev->dwCallBack)( (HWaveOut)dev, dev->lpBlockList,
													dev->dwCallBackData);
			}
		dev->position += inputMode	? dev->lpBlockList->dwBytesRecorded
									: dev->lpBlockList->dwBufferLength;
		dev->lpBlockList = dev->lpBlockList->lpNext;
		}

	if( dev->lpBlockList != NULL) {
		dev->processData = dev->lpBlockList->lpData;
		dev->bytesToProcess = dev->lpBlockList->dwBufferLength;
		}
	}



/*
	Output data byte 'data' to register 'reg' of channel
	'channel' of MMA.
	
	No return error.
*/
static void
WriteMMA( int channel, int reg, int data)
	{
	int io;

	IntOff();
	io = mma_io + (channel << 1);
	outp( io++, reg);

	outp( io, data);

	IntOn();
	}


static int
ReadMMA(int channel, int reg)
	{
	int io;
   int res;

	io = mma_io + (channel << 1);
	IntOff();
	outp( io++, reg);

   res = inp(io);

	IntOn();
	return res;
	}



static int 
ReadStatusMM1(void )
	{
	inp( mma_io);				/* clear status flags */
	return inp( mma_io);
	}





/*
	Cette routine est appelee par le driver d'interruption
	lorsqu'un interruption provenant du MMA concerne les buffer
	de FIFO.
*/
static int
FIFO_int(Word wFifoNr)
	{
	DevicePtr dev;
	int bit;

	dev = &waveDevices[ wFifoNr];
	if( dev->wDeviceStatus !=  STATUS_WORKING)
		return 0;

	if( dev->transferMode == WAVE_TRANSF_INTERRUPT)
		switch( dev->wDeviceMode) {
			case DEVICE_OUTPUT:
				DoPlayInterrupt( dev);
				break;
			case DEVICE_INPUT:
				DoInputInterrupt( dev);
				break;
			}
	else if( dev->transferMode == WAVE_TRANSF_DMA) {	/* DMA mode */
		IntOff();
#ifdef TURBO
		dmaStatus |= inportb( dmaStatusRegister);
#else
		dmaStatus |= inp( dmaStatusRegister);
#endif
		bit = 1 << dev->wDmaNr;
		if( dmaStatus & bit) {
			dmaStatus ^= bit;
			DoDataDMA( dev);
			}
		else
			;	/* false interrupt */
		IntOn();
		}
	else
		;		/* polling */
   return 1;
	}

#pragma check_stack()



```
