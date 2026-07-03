# `developer-toolkit-v1.01/installed/WAVE.H`

> UTF-8 rendering of a DOS-encoded (CP437 / CRLF) file. Byte-for-byte original: [`WAVE.H`](../../../disks/developer-toolkit-v1.01/installed/WAVE.H).

```c
/*
	WAVE.H
*/

#ifndef _Wave_
#define _Wave_

#ifndef _GLOBAL_H_
	#include "global.h"
#endif

#ifndef _cdecl
	#include "stddef.h"
#endif


#define WHDR_DONE		1
#define WHDR_BEGINLOOP	2
#define WHDR_ENDLOOP	4

#define WAVE_FORMAT_ADPCM4	0
#define WAVE_FORMAT_PCM8	1
#define WAVE_FORMAT_PCM12	2
#define WAVE_FORMAT_PCM16	3
#define WAVE_LAST_FORMAT	3

#define WAVE_FREQ44		0
#define WAVE_FREQ22		1
#define WAVE_FREQ11		2
#define WAVE_FREQ7			3
#define WAVE_FREQ5			4
#define WAVE_LAST_FREQ		4


/* Data transfer modes: */
#define WAVE_TRANSF_POLLING	0		/* data transfer mode by polling
										(application will freeze until sampling
										is finished) */
#define WAVE_TRANSF_INTERRUPT	1		/* data transfer mode by interrupt */
#define WAVE_TRANSF_DMA		2		/* data transfer mode by DMA */
#define WAVE_LAST_TRANSF		2

/* Flag for Open..() functions: */
#define WAVE_FORMAT_QUERY	1			/* query if device support format */

#define WAVE_STEREO_CENTER	0
#define WAVE_STEREO_LEFT		1
#define WAVE_STEREO_RIGHT	2
#define WAVE_LAST_STEREO		2

/* Error codes: */
#define WERR_BADDEVICEID	1			/* bad device ID */
#define WERR_ALLOCATED		2			/* device already opened */
#define WERR_STEREOBADCHANNEL	3		/* stereo device id must be # 0 */
#define WERR_STEREONEED2FREECHNL 4		/* stereo mode need two free channel */
#define WERR_INVALIDHANDLE	5			/* invalid device handle */
#define WERR_STILLPLAYING	6			/* Device is still playing a wave-form */
#define WERR_UNSUPPORTEDFORMAT 7		/* waveform format unsupported */
#define WERR_BADTRANSFERMODE	8		/* Bad or unsupported transfer mode */
#define WERR_BADPOSITION	9			/* bad selection (left/right) for channel output */

typedef unsigned  int 	Word;
typedef unsigned long  	DWord;
typedef DWord far 	*LPDWord;

typedef	char huge * WaveDataP;
typedef	DWord WaveSize;

typedef
	struct wavehdr_tag {
		WaveDataP lpData;		/* far ptr to waveform data buffer */
		WaveSize dwBufferLength;	/* length of the data buffer */
		WaveSize dwBytesRecorded;	/* when used in input, spec. how much data
									is in the buffer */
		DWord dwUser;			/* 32 bits user data */
		DWord dwFlags;			/*
								flag giving information about the data buffer:
								WHDR_DONE:
									set by device driver to indicate
									that it is finished with the data buffer
									and is returning it to the application;
								WHDR_BEGINLOOP:
									Specifies that this buffer is the first
									in a loop. Used only with output data buffer;
								WHDR_ENDLOOP:
									Specifies that this buffer is the last
									buffer in a loop. Used only with output
									data buffer

								Note:  To loop on a single block, specify
									both flags for the same block. Use the 
									'dwLoops' field in the WaveHdr structure
									for the first block in the loop to specify
									the number of loops.
								*/
		DWord dwLoops;			/* Specifies the number times that the loop
									is to played. Used only with output data
									buffer */
		struct wavehdr_tag far * lpNext;
								/* reserved, do not use */
		DWord reserved;			/* reserved, do not use */
	} WaveHdr, far * LpWaveHdr;


typedef
	struct waveformat_tag {
		Word wFormatTag;		/* Specifies the format type;
									WAVE_FORMAT_ADPCM4:	Format is ADPCM 4 bits
									WAVE_FORMAT_PCM8: format is PCM 8 bits
									WAVE_FORMAT_PCM12: format is PCM 12 bits
									WAVE_FORMAT_PCM16: format is PCM 16 bits
								*/
		Word nChannels;			/* Specifies the number of channels in the
									waveform data. Mono data uses 1 channel,
									stereo data uses 2 channels. */
		DWord samplingFreq;		/* Specifies the sampling rate,
									from 44.1K to 5.5125K
								*/
		DWord nAvgBytesPerSec;	/* Required average data transfer rate in
									bytes per second. */
		Word nBlockAlign;		/* block alignment of the data */
		Word wTransMode;		/* transfer mode: 
									WAVE_TRANSF_POLLING
									WAVE_TRANSF_INTERRUPT
									WAVE_TRANSF_DMA	*/
	} WaveFormat, far * LpWaveFormat;

typedef
	struct {
		Word wMid;		/* Manufacturer ID for the device driver */
		Word wPid;		/* Product ID for the waveform input device */
		Word vDriverVersion;	/* Version number of the device driver.
								The high byte is the major version, the low
								byte is the minor version. */
		char	szPname[ 20];	/* Specifies the product name in a
								NULL-terminated string */
		DWord dwFormats;	/* Specifies which standard formats are supported.
							The supported formats are specified with logical
							OR of the following flags:

							WAVE_FORMAT_1M08:	11.025K, mono, 8-bit
							...
							*/
		Word wChannels;		/* Specifies whether the device supports mono (1)
								or stereo (2) input. */
	} WaveInCaps, far * LpWaveInCaps;


typedef void far * HWaveIn;		/* handle to structure identifying an opened
									waveform input device */

typedef void far * HWaveOut;
typedef int (far * CallbackOutPtr)(HWaveOut dev, LpWaveHdr block, 
                                DWord dwCallBackData);
typedef int (far * CallbackInPtr)(HWaveIn dev, LpWaveHdr block, 
                               DWord dwCallBackData);


#ifdef __cplusplus
extern "C" {
#endif

/* external functions: */

WORD _cdecl InitWaveDriver();
WORD _cdecl CloseWaveDriver();
WORD _cdecl WaveOutOpen( HWaveOut far * lphWaveOut, WORD wDeviceId,
					LpWaveFormat lpFormat, CallbackOutPtr dwCallback,
					DWORD dwCallBackData, DWORD dwFlags);
WORD _cdecl WaveOutClose( HWaveOut hWaveOut);
WORD _cdecl WaveOutGetNumDevs();
WORD _cdecl WaveOutWrite( HWaveOut hWaveOut, LpWaveHdr lpWaveOutHdr, WORD wSize);
WORD _cdecl WaveOutSetVolume( HWaveOut hWaveOut, DWORD dwVolume);
WORD _cdecl WaveOutGetVolume( HWaveOut hWaveOut, LPDWord lpdwVolume);
WORD _cdecl WaveOutReset( HWaveOut hWaveOut);
WORD _cdecl WaveOutBreakLoop( HWaveOut hWaveOut);
WORD _cdecl WaveOutPause( HWaveOut hWaveOut);
WORD _cdecl WaveOutRestart( HWaveOut hWaveOut);
WORD _cdecl WaveInOpen( HWaveOut far * lphWaveOut, WORD wDeviceId,
					LpWaveFormat lpFormat, CallbackOutPtr dwCallback,
					DWORD dwCallBackData, DWORD dwFlags);
WORD _cdecl WaveInStart(HWaveIn hWaveIn);
WORD _cdecl WaveInAddBuffer( HWaveIn hWaveIn, LpWaveHdr lpWaveInHdr, WORD wSize);
WORD _cdecl WaveInClose( HWaveIn hWaveIn);
WORD _cdecl WaveInGetDevCaps( WORD wDeviceID, LpWaveInCaps lpCaps, WORD wSize);
WORD _cdecl WaveInGetNumDevs();

WORD _cdecl WaveInGetPosition(HWaveIn hWaveIn,	LPDWord wavePos);

WORD _cdecl WaveInOpen( HWaveIn far * lphWaveIn, WORD wDeviceID,
				LpWaveFormat lpFormat, CallbackInPtr dwCallBack,
				DWORD dwCallBackData, DWORD dwFlags);
WORD _cdecl WaveInReset( HWaveIn hWaveIn);
WORD _cdecl WaveInStart( HWaveIn hWaveIn);
WORD _cdecl WaveOutSetLeftRight( HWaveOut hWaveOut, WORD position);
void	far	WaveDrvInterruptEntry(void);
#ifdef __cplusplus
}
#endif

#endif

```
