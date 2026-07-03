/***************************************************************************
	 Playback.c
 	
	 Plays back Ad Lib .SMP files, using a the Wave Driver in a double
	 buffering scheme.
****************************************************************************/


#include	<stdio.h>
#include	<stdlib.h>
#include	<fcntl.h>
#include	<malloc.h>
#include	<sys\types.h>
#include	<sys\stat.h>
#include	<io.h>
#include	<conio.h>
#include	<string.h>

#include	"Global.h"
#include	"control.h"
#include	"wave.h"

#ifdef	TURBO
#undef inportb
#undef outportb
#endif

#define BLOCK_QTY	 7
#define BUF_SIZE    8192

#define DWORD      DWord

/* Header of a Gold Sample */
typedef
	struct {
		unsigned format;			/* data format:
									WAVE_FORMAT_ADPCM4:	Format is ADPCM 4 bits
									WAVE_FORMAT_PCM8: format is PCM 8 bits
									WAVE_FORMAT_PCM12: format is PCM 12 bits
									WAVE_FORMAT_PCM16: format is PCM 16 bits
									*/
		char	 stereo;			/* 0: mono, 1: stereo */
		char	 filler1;           /* 0 fro now */
		unsigned long	samplingFreq;	/* sampling frequency, in Hertz */
		unsigned long	nbBytes;	/* sample size, in bytes */
		unsigned long	startLoop;	/* offset of loop point, from start of sample */
		unsigned long	loopLength;	/* nombre of bytes of loop */
		unsigned loopCount;			/* # of time to do the loop, 0 if none */
		char filler2[ 8];			/* spares set to 0 for now*/
	} SampleHdr;

/* File format of AdLib SMP files: */
typedef struct {
		char	sig[ 12];			/* == "GOLD SAMPLE "   */
		char	majorVersion;		/* file version, major */
		char	minorVersion;		/* file version, minor */
		SampleHdr	sh;
	/*  char	data[].....	*/
	} FileHeader;


static int file;
/* Points to the playback device */
static HWaveOut    lphWaveOut;			

/* Block headers for the 	
   playback blocks				 */
static WaveHdr     waveHeaders[BLOCK_QTY];
						  
/* Playback data storage for the  					
   playback blocks				 */
static char        buffer[BLOCK_QTY][BUF_SIZE];		/* Playback data storage area    */


/*************************************************************************
TerminateBuffer is the Callback routine for sampling playback.  
It just marks the buffer as unused.
We use the dwUser data storage area to mark this. It is set to 0 when the
buffer is not in use, otherwise, it is set to 1.

Since this routine is called in interrupt time, we minimize the activities
performed within. The next routine, Play_Loop(), runs in the foreground 
and handles most of the work.

Notice that we do not use here the complex queuing mechanism used in
the recording module. Proper sequencing of the blocks is not a 
problem here.
*************************************************************************/

static int  far	TerminateBuffer (HWaveOut dev, LpWaveHdr block, DWORD hfile)
	{
	int	retVal;
	(void) dev;
	(void) hfile;

	block->dwUser = 0;
	return (1);
	}


/*************************************************************************
PlayLoop() runs in the foreground. 
It scans the data blocks for free blocks.
If a free block has been found, it fills it with data and queues it back 
for playback.

*************************************************************************/

int  PlayLoop ()
	{
	int 		bytesRead;
	int			i;
	LpWaveHdr	block;
	int			flagPlaying = 1;


	/* 
	 * Search for a unused playback block, and queue it back when found.
    * Playback starts as soon as a block is queued in.
	 */
printf("{");		
	for	(i = 0; (i < BLOCK_QTY) && flagPlaying; i++) {
printf("@");
		block = &waveHeaders[i];
		if (block->dwUser == 0) {
			bytesRead = read(file, (void *)block->lpData, BUF_SIZE);
			if (bytesRead != BUF_SIZE) flagPlaying = 0;	    /* end of file */
			block->dwBufferLength = bytesRead;
			block->dwUser = 1;						/* Marks as used */
printf("#");
			WaveOutWrite (lphWaveOut, block, sizeof(WaveHdr));
			}

		if (kbhit() && getch() == 0x1b) {
			flagPlaying = 0;
			}
    	}
printf("}\n");
	return (flagPlaying);
	}


/*************************************************************************
WaitForEnd() waits until all of the blocks have been released
by the WaveDriver.

*************************************************************************/
 
int WaitForEnd()

	{
	int i;
	int count;

	do {
		count = 0;
		for	(i = 0; (i < BLOCK_QTY); i++) 
			if (waveHeaders[i].dwUser == 0) count++;

		if (kbhit() && getch() == 0x1b)  count = BLOCK_QTY;
		}
		while(count < BLOCK_QTY);

	return(1);
	}


/*************************************************************************/

static int	ReadHeader(int file, WaveFormat *format)

	{
   WORD  bytesRead;
	FileHeader	hdr;

	bytesRead = (WORD) read(file, &hdr, sizeof(FileHeader));
	if( bytesRead != sizeof(FileHeader) ) {
       close(file);
		return(1);
		 }

	if (hdr.majorVersion != 1) {
       return(1);
       }

   switch (hdr.minorVersion) {
       case 0:
			if (strncmp(hdr.sig, "GOLD SAMPLE", 11)) {
	       		return(1);
	       		}
	   		break;

       case 1:
			if (strncmp(hdr.sig, "GOLD SAMPLE", 11)) {
	       		return(1);
	       		}
	  		 break;

       default: 
	   		return(1);
       }

   format->nChannels  = hdr.sh.stereo + 1;
   format->wFormatTag = hdr.sh.format;
   format->samplingFreq = hdr.sh.samplingFreq;

   switch (hdr.sh.format) {
       case    WAVE_FORMAT_ADPCM4:
	        format->nAvgBytesPerSec = format->samplingFreq / 2;
	        format->nBlockAlign = 1;
	        break;
       case    WAVE_FORMAT_PCM8:
	   		format->nAvgBytesPerSec = format->samplingFreq;
	   		format->nBlockAlign = 1;
	   		break;

       case    WAVE_FORMAT_PCM12:
	   		format->nAvgBytesPerSec = format->samplingFreq * 2;
	   		format->nBlockAlign = 2;
	   		break;

       case    WAVE_FORMAT_PCM16:
	   		format->nAvgBytesPerSec = format->samplingFreq * 2;
	   		format->nBlockAlign = 2;
	   		break;

       default: 
	   		format->nAvgBytesPerSec = format->samplingFreq;
	   		format->nBlockAlign = 1;
	   		break;
       }

   format->wTransMode = WAVE_TRANSF_INTERRUPT;

   return(0);
	}

static void  PrepareBlocks()
	{
	int i;
	/* Initialize the data blocks */
	for (i = 0; i < BLOCK_QTY; i++) {
   	memset ((void far *)&waveHeaders[i], 0, sizeof (WaveHdr));
   	waveHeaders[i].lpData = (WaveDataP) buffer[i];
		waveHeaders[i].dwUser = 0;
	 	}
	}


/************************************************************************
                          Play back a sample. 
**************************************************************************/

main (int argc, char *argv[])
	{
	WaveFormat  sFormat;	/* Format description for the file	*/

   int channel = 0;

   if (argc < 2) {
		puts ("Format:\n\tplayback file\n");
		exit (1);
   	}

	file = open(argv[1], O_BINARY | O_RDONLY);

	if (file < 0) {
		puts ("Unable to open file");
		return(1);
    	}

	ReadHeader(file, &sFormat);

	if (InitControlDriver()) {
		printf("Can not link to control chip module\n");
		exit(1);
		}

	InitWaveDriver();
putch('@');
   WaveOutOpen (&lphWaveOut, channel, &sFormat, TerminateBuffer, (long)file, 0L);

   PrepareBlocks();

  	/* Queue blocks until entire sample has been queued. */
printf("Enter play loop\n");
   while (PlayLoop ());

	/* Wait until all blocks are released 		*/

	WaitForEnd();


   WaveOutClose (lphWaveOut);

   CloseWaveDriver ();
	CloseControlDriver();

	return(0);
	}

