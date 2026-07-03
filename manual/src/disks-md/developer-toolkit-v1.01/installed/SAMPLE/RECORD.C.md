# `developer-toolkit-v1.01/installed/SAMPLE/RECORD.C`

> UTF-8 rendering of a DOS-encoded (CP437 / CRLF) file. Byte-for-byte original: [`RECORD.C`](../../../../disks/developer-toolkit-v1.01/installed/SAMPLE/RECORD.C).

```c
/***************************************************************************
	 Record.c
 	
	 Records Ad Lib .SMP files, using a the Wave Driver in a double
	 buffering scheme, and direct-to-disk.
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

/*
 * Experiment with those values if you wish.
 * When pushing the sample rate and size too high, the file I/O throughput
 * may become slower than the sampling rate. This would result in audible
 *	clicks. This is dependant on CPU speed and disk speed.
 * 
 */

#define BLOCK_QTY	 	3
#define BUF_SIZE    	8192
#define myFormat		WAVE_FORMAT_PCM16
#define mySamplingFreq	22075
#define myTransferMode   WAVE_TRANSF_DMA


/***** OOPS! ****/

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
/* Points to the recording device */
static HWaveIn   lphWaveIn;			

/* Block headers for the 	
   recording blocks				 */
static WaveHdr     waveHeaders[BLOCK_QTY];
						  
/* Playback data storage for the  					
   recording blocks				 */
static char        buffer[BLOCK_QTY][BUF_SIZE];		/* Playback data storage area    */

/* Count of bytes written to disk */
static long	byteCount = 0;

/* Queue of blocks, released, to kkep them in sequence	*/
LpWaveHdr	filledQueue;
LpWaveHdr	freeQueue;


/*************************************************************************
A couple of macros and functions to manage lists. The list pointers, used
for the queues, are placed in dwUser field, and we need a lot of typecasting
***************************************************************************/
#define GetNext(h)	((LpWaveHdr) (h->dwUser))
#define SetNext(h, n) ( ((LpWaveHdr)h)->dwUser = (DWORD) n) 
 
/*************************************************************************
TerminateBuffer is the Callback routine for sampling recording.  
It just marks the buffer as having some data ready to write.
The dwUser field is used to maintain a queue of blocks filled and
ready to be witten to disk (filledQueue) and a queue of freeBlocks
(freeQueue)

Since this routine is called in interrupt time, we minimize the activities
performed within. The next routine, RecordLoop(), runs in the foreground 
and handles most of the work.
*************************************************************************/

static int  far	TerminateBuffer (HWaveIn dev, LpWaveHdr block, DWORD dummy)
	{
	int	retVal;
	LpWaveHdr	tailBlock;
	(void) dev;
	(void) dummy;

	/* Add the block to the end of a queue of filled blocks    		*/
	/* the queue prevents blocks from going out of sequence			*/
	SetNext(block, NULL);
	
	if (!filledQueue) {
		filledQueue = block;
		}
	else {
		tailBlock = filledQueue;
		while(GetNext(tailBlock))	tailBlock = GetNext(tailBlock);
		SetNext(tailBlock, block);
		}
   return(1);
	}


/*************************************************************************
WriteFilledBlocks puts all the filled blocks to disk.
The blocks are transferred from the filledQueue to the freeQueue. 

*************************************************************************/

int WriteFilledBlocks()

	{
	int 		bytesRead;
	int			i;
	LpWaveHdr	block;

	/* 
	 * Write all the blocks received so far. We used a queue to
	 * Make sure that we maintained the sequence
	 */
printf("<");
	if (filledQueue) {
printf("!");
		block = filledQueue;	
		filledQueue = GetNext(block);
		write(file, (void *)block->lpData, block->dwBytesRecorded);
		byteCount += block->dwBytesRecorded;
		/* Add to the free queue */
		SetNext(block, freeQueue);
		freeQueue = block;
		}
printf(">");

	return(1);
	}

/*************************************************************************
SendFreeBlocks queues back all of the free blocks to the wave driver.
We do not keep a queue of the busy blocks, since the WaveDriver maintains
one internally.
*************************************************************************/
int SendFreeBlocks()
	{
	LpWaveHdr	block;

	if (!freeQueue) return(0);
printf("{");

	while (freeQueue) {
printf("!");
		block = freeQueue;	  
		freeQueue = GetNext(block);
		block->dwBufferLength = BUF_SIZE;
		SetNext(block, NULL);
		WaveInAddBuffer(lphWaveIn, block, sizeof(WaveHdr));
		}
printf("}");
	return(1);
	}

/*************************************************************************
RecordLoop() runs in the foreground. 
It scans the data blocks for free blocks.
If a free block has been found, it fills it with data and queues it back 
for recording.

*************************************************************************/

int  RecordLoop ()
	{
	WriteFilledBlocks();

	if (kbhit()) {
		getch();
		WaveInReset(lphWaveIn);
		return(0);
		}

	SendFreeBlocks();
	return(1);
	}

/*************************************************************************
WaitForEnd() waits until all of the blocks have been released
by the WaveDriver.

If a block is released, write the data it contains to the driver.


*************************************************************************/

int WaitForEnd()

	{
	int i;
	int count;
	LpWaveHdr	block;

	do {
		WriteFilledBlocks();
		
		/* Scan the list of free blocks for a count of those */
		for	(count = 0, block = freeQueue; 
			 block != NULL; 
			 block = GetNext(block), count++);
		if (kbhit()) {
			getch();
			break;
			}
		}
		while(count < BLOCK_QTY);

	return(1);
	}

/*************************************************************************/

static int	WriteHeader(int file, WaveFormat *format)

	{
   WORD  bytesRead;
	FileHeader	hdr;

	hdr.majorVersion = 1;
   hdr.minorVersion = 1;
	strcpy(hdr.sig, "GOLD SAMPLE");
   hdr.sh.stereo = format->nChannels -1;
   hdr.sh.format = format->wFormatTag;
   hdr.sh.samplingFreq = format->samplingFreq;
	hdr.sh.nbBytes = byteCount;
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


	lseek(file, 0L, SEEK_SET);
	write(file, &hdr, sizeof(FileHeader));
	return(1);
	}

static void  PrepareBlocks()
	{
	int i;
	/* Initialize the queue pointers */
	freeQueue = NULL;
	filledQueue = NULL;

	/* Initialize the data blocks */
	for (i = 0; i < BLOCK_QTY; i++) {
   	memset ((void *) &waveHeaders[i], 0, sizeof (WaveHdr));
   	waveHeaders[i].lpData = (WaveDataP) buffer[i];
		waveHeaders[i].reserved = i;
		SetNext((&waveHeaders[i]), freeQueue);
		freeQueue = &waveHeaders[i];
	 	}
	}


/************************************************************************
                          Record a sample. 
**************************************************************************/

main (int argc, char *argv[])
	{
	WaveFormat  sFormat;	/* Format description for the file	*/
	int			result;

   int channel = 0;

   if (argc < 2) {
		puts ("Format:\n\trecord file\n");
		exit (1);
   	}

	printf("Recording to -%s-\n", argv[1]);
   file = open(argv[1], O_WRONLY | O_CREAT | O_BINARY, 
                         S_IREAD | S_IWRITE);

	if (file < 0) {
		puts ("Unable to open file");
		return(1);
    	}

	sFormat.wFormatTag = myFormat;
	sFormat.nChannels = 1;
	sFormat.samplingFreq = mySamplingFreq;
   sFormat.wTransMode =  myTransferMode;

	WriteHeader(file, &sFormat);

	if (InitControlDriver()) {
		printf("Can not link to control chip module\n");
		exit(1);
		}

	InitWaveDriver();
   result = WaveInOpen (&lphWaveIn, channel, &sFormat, TerminateBuffer, (long)file, 0L);
	if (result) {
		printf("Cannot open input device\n");
		exit(1);
		}

   PrepareBlocks();

	/* Queue first few blocks	*/
	SendFreeBlocks();

	/* 
	 * Ideally, WaveInStart should not be called until there are
	 * some buffers queued in.
	 */
	WaveInStart(lphWaveIn);
	
  	/* Queue blocks until user stops the recording. */
	byteCount = 0;
	printf("Recording, hit any key to stop\n");
   while (RecordLoop ());

	/* Wait until all blocks are released */
	WaitForEnd();

   WaveInClose (lphWaveIn);

	/* Rewrite header, to put byteCount in there	*/
	WriteHeader(file, &sFormat);

	close(file);
   CloseWaveDriver ();
	CloseControlDriver();
	return 0;
	}




```
