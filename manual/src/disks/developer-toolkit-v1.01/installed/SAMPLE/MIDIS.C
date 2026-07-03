
#ifdef TURBO
#pragma	hdrfile	midis.sym
#endif

#include	<stdio.h>
#include	<dos.h>

#include   "global.H"
#include   "Control.H"
#include   "Midi.H"

#ifdef	TURBO
#pragma	hdrstop

#undef inportb
#undef outportb
#endif

/****************************************************************************/

static enum	ProgMode {
	interr,
	polling
	};

/*
 *	Queue are used in this module to push data before being send
 */

#define queueLength	1024

/*
 *	Synopsis:		Queue, QueuePtr
 *							 
 *	Description: 	Most of the communication between the various levels
 *					of the ICI is done thru queues. A structure is given to
 *					facilitate the manipulation of queues.			
 */

typedef struct	Queue {
	int		head;
	int		tail;
	int		number;
	BYTE	array[queueLength];
	} Queue,  *QueuePtr;

/****************************************************************************/

int		ctrlChipIntNb;						// current ctrl chip vector
void   	(interrupt far *oldFunc)();

Queue	debug;

/****************************************************************************/

void	InstallDriverTestInterrupt2(void);
void   RestoreDriverTestInterrupt2(void);
void	Intelligent(BYTE data);

PRIVATE	void	QueueByte( QueuePtr queue, BYTE byte);
PRIVATE	BYTE	UnQueueByte( QueuePtr queue);
PRIVATE	void	ResetQueue( QueuePtr queue);
PRIVATE	WORD	GetQueueNumber( QueuePtr queue);

typedef	struct Note {
	BYTE	status;
	BYTE	pitch;
	BYTE	velo;
	} Note;

Note		song[100] = {
		{	0x90, 0x34, 0x40    },
		{	0x80, 0x34, 0x40    },
		{	0x90, 0x36, 0x40    },
		{	0x80, 0x36, 0x40    },
		{	0x90, 0x37, 0x40    },
		{	0x80, 0x37, 0x40    },
		{	0x90, 0x38, 0x40    },
		{	0x80, 0x38, 0x40    },
		{	0x90, 0x3b, 0x40    },
		{	0x80, 0x3b, 0x40    },
		{	0x90, 0x3d, 0x40    },
		{	0x80, 0x3d, 0x40    },
		{	0x90, 0x40, 0x40    },
		{	0x80, 0x40, 0x40    },
		{	0x90, 0x42, 0x40    },
		{	0x80, 0x42, 0x40    },
		{	0x90, 0x43, 0x40    },
		{	0x80, 0x43, 0x40    },
		{	0x90, 0x44, 0x40    },
		{	0x80, 0x44, 0x40    },
		{	0x90, 0x47, 0x40    },
		{	0x80, 0x47, 0x40    },
		{	0x90, 0x49, 0x40    },
		{	0x80, 0x49, 0x40    },
		{	0x90, 0x4c, 0x40    },
		{	0x80, 0x4c, 0x40    },
		{	0x90, 0x4e, 0x40    },
		{	0x80, 0x4e, 0x40    },
		{	0x90, 0x4f, 0x40    },
		{	0x80, 0x4f, 0x40    },
		{	0x90, 0x50, 0x40    },
		{	0x80, 0x50, 0x40    },
		{	0x90, 0x53, 0x40    },
		{	0x80, 0x53, 0x40    },
		{	0x90, 0x55, 0x40    },
		{	0x80, 0x55, 0x40    },
		{	0x90, 0x58, 0x40    },
		{	0x80, 0x58, 0x40    },
		{	0x80, 0x49, 0x40    },
		{	0x80, 0x4f, 0x40    },
		{	0x80, 0x37, 0x40    },
		{	0x80, 0x43, 0x40    },
		{	0x80, 0x58, 0x40    },
		{	0x80, 0x3d, 0x40    }};


/****************************************************************************/


void dispatch(WORD msg, DWORD param)
	{
	(void) msg;

	if (param != 0xFE) QueueByte(&debug, param);
	}

main()
	{
	BYTE	data;
	int		progMode = interr;
	int		i;
	long	j;

	if (InitControlDriver()) {
		printf("Can not link to control chip driver\n");
		exit(1);
		}

	printf("Example program running level %d\n", gssLevel);

	if (InitMidiDriver()) {
		printf("Can not init MIDI driver\n");
		exit(1);
		}

	ResetQueue(&debug);
	
	/*
	 *	In the resident drivers version use: AssignMidiDispatcher(dispatch);
	 *	but in the object version use SetMidiDispatcher(dispatch);
	 */

	SetMidiDispatcher(dispatch);	

   MaskMMAMidiOutInterrupt(0);
   MaskMMAMidiInInterrupt(0);
	MaskMMAOverrunError(0);

	printf("Transmitting a bunch of notes...\n");

	for (i = 0 ; i < 44 ; i += 2) {
		SendMMAMidiData(song[i].status);
		SendMMAMidiData(song[i].pitch);
		SendMMAMidiData(song[i].velo);
		for (j = 0 ; j < 25000 ; j++);

		SendMMAMidiData(song[i+1].status);
		SendMMAMidiData(song[i+1].pitch);
		SendMMAMidiData(song[i+1].velo);
		for (j = 0 ; j < 25000 ; j++);
		}

	printf("press any key to stop collecting\n");
	while (! kbhit()) {
		if (progMode != interr)	{
			if (GetMMAMidiInReady()) {
				QueueByte(&debug, ReadMMAMidiData());
				}
			}
		else {
			while( GetQueueNumber(&debug) ) {
				Intelligent(UnQueueByte(&debug));
				}
			}
		}
	getch();

   MaskMMAMidiOutInterrupt(1);
   MaskMMAMidiInInterrupt(1);
	MaskMMAOverrunError(1);

	ResetMidiDispatcher();

	CloseMidiDriver();
	CloseControlDriver();
	return 0;
   }

void	Intelligent(BYTE data)
	{
	static	int		mode = 0;

	/*
    *  Message             1st byte        2nd byte        3rd byte
    *  -------             --------        --------        --------
    *  Note off            $80 - $8F       $00 - $7F       $00 - $7F
    *                      $90 - $9F       $00 - $7F          $00
    *  Note on             $90 - $9F       $00 - $7F       $00 - $7F
    *  Note After Touch    $A0 - $AF       $00 - $7F       $00 - $7F
    *  Control Change      $B0 - $BF       $00 - $7F       $00 - $7F
    *  Program Change      $C0 - $CF       $00 - $7F  
    *  Ch After Touch      $D0 - $DF       $00 - $7F  
    *  Pitch Wheel         $E0 - $EF       $00 - $7F       $00 - $7F
	 */

	if (data == 0xF0) {
		mode = 1;
		printf("\n%02x ",data);
		return;
		}
	else {
		if (data == 0xF7) {
			mode = 0;
			printf("%02x\n",data);
			return;
			}
		}

	if ((mode == 0) && (data & 0x80)) printf("\n");
	printf("%02x ", data);
	}

/*
 * Synopsis:		void	QueueByte( queue, byte)     
 *
 *	Description: 	Places the given byte at the tail of the queue.
 *
 *	Argument:		QueuePtr    queue;                 
 *					BYTE        byte;                  
 *
 *	Return value:	none
 *     
 */                                   

PRIVATE	
void	QueueByte( QueuePtr queue, BYTE byte)

    {
    int tail;
    
    if (queue->number == queueLength)
        return;
    tail = queue->tail;
    queue->array[tail] = byte;
    if (++tail == queueLength)
        tail = 0;
    queue->tail = tail;
    
    queue->number++;
    }

/*
 * Synopsis:		BYTE    UnQueueByte( QueuePtr queue)
 *
 * Description: 	Unqueues the byte at the head of the specified queue
 *      			and returns it as a result.
 *
 *	Argument:		QueuePtr    queue
 *
 *	Return value:	Return oldest byte stored in the queue.
 */

PRIVATE	
BYTE	UnQueueByte( QueuePtr queue)
    {
    BYTE 	byte;
    int 	head;
    
    head = queue->head;
    byte = queue->array[head];

    if (++head == queueLength)
        head = 0;
    queue->head = head;
    queue->number--;
    
    return(byte);
    }

/*
 *	Synopsis:		ResetQueue( QueuePtr queue)
 *                          
 *	Description: 	Zeroes the specified queue.
 *
 *	Argument:		QueuePtr	queue
 *
 *	Return values:	none
 */

PRIVATE	
void	ResetQueue( QueuePtr queue)
    {
    queue->head = 0;
    queue->tail = 0;
    queue->number = 0;
    }

/*
 *	Synopsis:		GetQueueNumber( QueuePtr queue)
 *                                               
 *	Description:	Returns the current number of byte in queue.
 *
 *	Argument:  		QueuePtr    queue
 *
 *	Return value:	Number of bytes stored in this queue.
 */

PRIVATE	
WORD	GetQueueNumber( QueuePtr queue)
    {
    return(queue->number);
    }
