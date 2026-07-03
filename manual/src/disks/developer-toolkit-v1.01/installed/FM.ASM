
; OPL-III Low Level Driver
;
; Marc Savary, 24-jan-91
;
;


	INCLUDE MODEL2.MAC	; define memory model & macros


; 	Public functions defined in this module:

; InitFMDriver( ioAddr)	   	Initialize OPL-III chip
; CloseFMDriver()		Reset to original YM3812 mode
; Set4opMaskOpl3( msk)	   	Enable/Disable 4-op voice mask
; SetPercModeOpl3( state)  	Enable/Disable perc mode
; SetGlobalOpl3( nSel, amD, vibD, pBRange) Change some globals of OPL-III
; PresetOpl3( voice, dP)   	Change preset of voice
; LevelOpl3( voice, level) 	Change volume of voice
; NoteOnOpl3( voice, note) 	Do a note-on on voice
; NoteOffOpl3( voice)	   	Do a note-off on voice
; PitchBendOpl3( voice, pBend)	Change pitch bend
; LeftRightOpl3( voice, lR)	Change Left/Right/Center attribute of voice


;	PRESET FORMAT:
;
; 4 times these first 6 bytes (for slot 0 to 3)
;	byte  0: a v e k m m m m	; a=AM, v=VIB, e=EG-TYP, k=KSR, m=MULTI
;	byte  1: k k l l l l l l	; k=KSL, l=LEVEL
;	byte  2: a a a a d d d d	; a=AR, d=DR
;	byte  3: s s s s r r r r	; s=SL, r=RR
;	byte  4: d - - - - w w w	; d=slot defined, w=WAVE-SELECT
;	byte  5: - - - - - - - -	; unused, == 0
;
; plus the following bytes:
;	byte 24: c4 - - - f f f c	; c4=second bit for CONNECT,
;					; f=FEED-BACK, c=first bit for CONNECT
;	byte 25: - - p 2 4 n n n	; p= perc preset; 2= 2-op preset,
;					; 4= 4-op preset, n= perc # (BD=0,
;					; SD=1, TOM=2, CYMB=3, HH=4)
;	byte 26: t t t t t t t t	; t= sign integer transposition in half-tone
;	byte 27: - - - - - - - -	; unused, == 0
; total = 28 bytes


;	       voice numbers
;     bank     4-op <->  2-op    percs
;-------------------------------------------------
;	0	0	 0,  1	  -
;	0	2	 2,  3	  -
;	0	4	 4,  5	  -
;-------------------------------------------------
;	1	6	 6,  7	  -
;	1	8	 8,  9	  -
;	1      10	10, 11	  -
;-------------------------------------------------
;	1	-	12	  -
;	1	-	13	  -
;	1	-	14	  -
;-------------------------------------------------
;	0	-	15   	  15 (BD)
;	0	-	16   	  16 (HH),  17 (SD)
;	0	-	18   	  18 (TOM), 19 (CYMB)



PRESET_OPR_SIZE	equ	6	; 6 bytes per operators, 4 op.
PRESET_CONNECT	equ	24	; offset in preset data of Feed-Back & connect
PRESET_TYPE	equ	25
PRESET_TRANSP	equ	26	; .... transposition
PRESET_SPARE	equ	27	; dernier byte libre du preset


; field description of 25th byte (PRESET_TYPE) of a preset array...
PERC_BIT	equ	32	; ON if percussion preset
TWO_OP_BIT	equ	16	; ON if 2 op. (melodic) preset
FOUR_OP_BIT	equ	8	; ON if 4 op. preset
PERC_NUM_MSK	equ	7	; mask of bit field for percussion # (0 - 4)

FIRST_PERC	equ	15
BD		equ	15
HH		equ	16
SD		equ	17
TOM		equ	18
CYMB		equ	19
LAST_PERC	equ	19
LAST_VC		equ	19

TYPE_DISABLE	equ	80h	; values for 'typeVc' array ... voice disable
TYPE_4OP	equ	8	; ... voice 4-op
TYPE_2OP	equ	16	; ... voice 2-op
TYPE_PERC	equ	32	; ... voice perc

MAX_LOG_VOL	equ	3fh	; maximum volume (attenuation), logarithmic

nb_notes	equ	96
octave		equ	12
nb_table_demi_ton equ	octave
nb_step_pitch	equ	16
log_nb_step_pitch equ	4
table_size 	equ	(nb_step_pitch * nb_table_demi_ton)
log_pitch 	equ	8
DELTA_PITCH	equ	-12	; convertion factor from MIDI C4 = 60 to internal C4 = 48
TOM_PITCH	equ	(60-24)	; two octave below MIDI C4 is the best!!
TOM_TO_HH	equ	7	; 7 half-tone between TOM & HH ... 19 ?




; initialized data segment:
	DATAS

 IF 0
; temporaire... pour le code de debug:
COMM NEAR _map0: BYTE: 256	; accessible to C environment
COMM NEAR _map1: BYTE: 256	; ...
 ENDIF


; slots offset for each voices: 2 slots per voices, -1 means 'not used';
; 4-op voices use two lines:
vcToSlot db	00h, 03h	; #0 4-op, 2-op
	db	08h, 0bh	; #1 2-op
	db	01h, 04h	; #2 4-op, 2-op
	db	09h, 0ch	; #3 2-op
	db	02h, 05h	; #4 4-op, 2-op
	db	0ah, 0dh	; #5 2-op
	db	20h, 23h	; #6 4-op, 2-op
	db	28h, 2bh	; #7 2-op
	db	21h, 24h	; #8 4-op, 2-op
	db	29h, 2ch	; #9 2-op
	db	22h, 25h	; #10 4-op, 2-op
	db	2ah, 2dh	; #11 2-op

	db	30h, 33h	; #12 2-op
	db	31h, 34h	; #13 2-op
	db	32h, 35h	; #14 2-op

	db	10h, 13h	; #15 2-op, BD
	db	11h, 14h	; #16 2-op, HH
	db	14h, -1		; #17 SD
	db	12h, 15h	; #18 2-op, TOM
	db	15h, -1		; #19 CYMB


; channel offset for each voices; -1 means 'not used',
; 4-op voices use two lines:
vcToChannel db	00h		; #0 4-op, 2-op
	db	03h		; #1 2-op
	db	01h		; #2 4-op, 2-op
	db	04h		; #3 2-op
	db	02h		; #4 4-op, 2-op
	db	05h		; #5 2-op
	db	20h		; #6 4-op, 2-op
	db	23h		; #7 2-op
	db	21h		; #8 4-op, 2-op
	db	24h		; #9 2-op
	db	22h		; #10 4-op, 2-op
	db	25h		; #11 2-op

	db	26h		; #12 2-op
	db	27h		; #13 2-op
	db	28h		; #14 2-op
	
	db	06h		; #15 BD
	db	07h		; #16 HH
	db	-1		; #17 SD
	db	08h		; #18 TOM
	db	-1		; #19 CYMB



; mask des slots carrier: 1: 1 iere slot, 2: 2 ieme slot, 4: 3 ieme slot, 
;			  8: 4 ieme slot
carMsk4op db	8, 1+8, 2+8, 1+4+8	; for 4-op voices only
carMsk2op db	2, 1+2, 2, 1+2		; for 2-op melodic & perc voices

leftRightBits db 30h, 20h, 10h		; bits for center, left & right
percBits db	10h, 1, 8, 4, 2		; BD, HH, SD, TOM, CYMB


; Table de convertion lineaire -> logarithmique (attenuation), 128 valeurs
; f(0) = 63
; f(A) = (-20 * log10( A/128)) / 0.75		==> [1 <= A <= 127]
linToLog db	63, 56, 48, 43, 40, 37, 35, 33
	db	32, 30, 29, 28, 27, 26, 25, 24, 24
	db	23, 22, 22, 21, 20, 20, 19, 19, 18
	db	18, 18, 17, 17, 16, 16, 16, 15, 15
	db	15, 14, 14, 14, 13, 13, 13, 12, 12
	db	12, 12, 11, 11, 11, 11, 10, 10, 10
	db	10,  9,  9,  9,  9,  9,  8,  8,  8 
	db 	 8,  8,  8,  7,  7,  7,  7,  7,  6
	db	 6,  6,  6,  6,  6,  6,  5,  5,  5
	db	 5,  5,  5,  5,  4,  4,  4,  4,  4
	db	 4,  4,  3,  3,  3,  3,  3,  3,  3
	db	 3,  2,  2,  2,  2,  2,  2,  2,  2
	db	 2,  1,  1,  1,  1,  1,  1,  1,  1
	db	 1,  1,  0,  0,  0,  0,  0,  0,  0
	db	 0,  0,  0


; 10 bits F-Num table for Adlib board
; containing 12 notes (from 32.8 Hz to 65.6 Hz), with 16 steps between each note.
; If the sign bit is on, 1 must be added to the block octave information
; 
; each value is compute by the formula:
; fN = (2**20 x Fmus) / (3.58e6 / 72)
; fN is shift right until is below 1024 (10 bits)
;
; here is the C code to generate table:

;#define CLOCK			3579545L /* 14.31818e6 /4 */
;#define F_SAMPL		(CLOCK /72)	/* chip sampling freq */
;#define NB_NOTES		96
;#define OCTAVE			12
;#define NB_TABLE_DEMI_TON OCTAVE
;#define NB_STEP_PITCH	16		/* 16 pas d'un ton a l'autre */
;#define LOG_NB_STEP_PITCH 4		/* LOG2( NB_STEP_PITCH) */
;#define TABLE_SIZE (NB_STEP_PITCH * NB_TABLE_DEMI_TON)
;#define LOG_PITCH		8	/* LOG2( TABLE_SIZE) */
;#define FREQ_DO (double)261.6256	/* reference C4 */
;
;#define xexpy( x, y) (exp( y * log( x)))	/* return x**y */
;
;GenTable()
;	{
;	double freq;
;	double range;
;	long fNum, fN10;
;	int block;
;	int i, value;
;
;	range = 2.0;
;	for( i = 0; i < TABLE_SIZE; i++) {
;		freq = xexpy( range, (double)i / TABLE_SIZE);
;		freq *= FREQ_DO;
;		fNum = ((long)((long)1 << 20) * freq) /F_SAMPL;
;		for( block = 0, fN10 = fNum; fN10 >= 1024; fN10 >>= 1, block++)
;			;
;		/* block range from 3 to 4 */
;		fN10 = (fNum + (1 << block -1)) >> block; /* round to 0.5 */
;		block = 3 - block;
;		value = fN10 | (block << 10);
;		printf( "%05xH, ", value);
;		if( i % 10 == 9)
;			printf( "\n");
;		}
;	printf( "\n\n");
;	}


fNumTbl dw 02b2H, 02b4H, 02b7H, 02b9H, 02bcH, 02beH, 02c1H, 02c3H, 02c6H, 02c9H
	dw 02cbH, 02ceH, 02d0H, 02d3H, 02d6H, 02d8H, 02dbH, 02ddH, 02e0H, 02e3H 
	dw 02e5H, 02e8H, 02ebH, 02edH, 02f0H, 02f3H, 02f6H, 02f8H, 02fbH, 02feH 
	dw 0301H, 0303H, 0306H, 0309H, 030cH, 030fH, 0311H, 0314H, 0317H, 031aH 
	dw 031dH, 0320H, 0323H, 0326H, 0329H, 032bH, 032eH, 0331H, 0334H, 0337H 
	dw 033aH, 033dH, 0340H, 0343H, 0346H, 0349H, 034cH, 034fH, 0352H, 0356H 
	dw 0359H, 035cH, 035fH, 0362H, 0365H, 0368H, 036bH, 036fH, 0372H, 0375H 
	dw 0378H, 037bH, 037fH, 0382H, 0385H, 0388H, 038cH, 038fH, 0392H, 0395H 
	dw 0399H, 039cH, 039fH, 03a3H, 03a6H, 03a9H, 03adH, 03b0H, 03b4H, 03b7H 
	dw 03bbH, 03beH, 03c1H, 03c5H, 03c8H, 03ccH, 03cfH, 03d3H, 03d7H, 03daH 
	dw 03deH, 03e1H, 03e5H, 03e8H, 03ecH, 03f0H, 03f3H, 03f7H, 03fbH, 03feH 
	dw 0fe01H, 0fe03H, 0fe05H, 0fe07H, 0fe08H, 0fe0aH, 0fe0cH, 0fe0eH, 0fe10H, 0fe12H
	dw 0fe14H, 0fe16H, 0fe18H, 0fe1aH, 0fe1cH, 0fe1eH, 0fe20H, 0fe21H, 0fe23H, 0fe25H 
	dw 0fe27H, 0fe29H, 0fe2bH, 0fe2dH, 0fe2fH, 0fe31H, 0fe34H, 0fe36H, 0fe38H, 0fe3aH 
	dw 0fe3cH, 0fe3eH, 0fe40H, 0fe42H, 0fe44H, 0fe46H, 0fe48H, 0fe4aH, 0fe4cH, 0fe4fH 
	dw 0fe51H, 0fe53H, 0fe55H, 0fe57H, 0fe59H, 0fe5cH, 0fe5eH, 0fe60H, 0fe62H, 0fe64H 
	dw 0fe67H, 0fe69H, 0fe6bH, 0fe6dH, 0fe6fH, 0fe72H, 0fe74H, 0fe76H, 0fe79H, 0fe7bH 
	dw 0fe7dH, 0fe7fH, 0fe82H, 0fe84H, 0fe86H, 0fe89H, 0fe8bH, 0fe8dH, 0fe90H, 0fe92H 
	dw 0fe95H, 0fe97H, 0fe99H, 0fe9cH, 0fe9eH, 0fea1H, 0fea3H, 0fea5H, 0fea8H, 0feaaH 
	dw 0feadH, 0feafH 


; integer division & modulo 12 table (0 - 95)
divMod12 db 00, 01, 02h, 03h, 04h, 05h, 06h, 07h, 08h, 09h, 0ah, 0bh
	db 10h, 11h, 12h, 13h, 14h, 15h, 16h, 17h, 18h, 19h, 1ah, 1bh
	db 20h, 21h, 22h, 23h, 24h, 25h, 26h, 27h, 28h, 29h, 2ah, 2bh
	db 30h, 31h, 32h, 33h, 34h, 35h, 36h, 37h, 38h, 39h, 3ah, 3bh
	db 40h, 41h, 42h, 43h, 44h, 45h, 46h, 47h, 48h, 49h, 4ah, 4bh
	db 50h, 51h, 52h, 53h, 54h, 55h, 56h, 57h, 58h, 59h, 5ah, 5bh
	db 60h, 61h, 62h, 63h, 64h, 65h, 66h, 67h, 68h, 69h, 6ah, 6bh
	db 70h, 71h, 72h, 73h, 74h, 75h, 76h, 77h, 78h, 79h, 7ah, 7bh

	EDATAS



; uninitialized data segment:
	DATA?S

typeVc		db	20 DUP (?)	; 80h: disable; 8: 4op, 16: 2op, 32: perc
nbOpVc		db	20 DUP (?)	; nb opr per voice: 4, 2, 1
levelVc		db	20 DUP (?)	; voice level, log. attenuation
transpVc	db	20 DUP (?)	; voice transpose, +/- half-tones
noteVc		db	20 DUP (?)	; voice note #, 12 - 107
pitchBendVc	dw	20 DUP (?)	; voice pitch bend, 0 - 3fff, 2000 = normal

carMaskVc	db	20 DUP (?)	; carrier mask of each voices
c0RegVc		db	20 DUP (?)	; [EXT-OUT, STEREO, FB, Connect], [Connect]
b0RegVc		db	20 DUP (?)	; Bx register for for each voices
bdReg		db	(?)		; AM-D, VIB-D, RHY, BD, SD, TOM, CY, HH
mask4op		db	(?)		; bits for 4-op connection register
kslLevSlots	db	(20h+16h) DUP (?) ; values of each KSL-LEVEL regs

pitchRange	dw	(?)		; pitch bend range 1 - 12
opl3_io		dw	(?)		; OPL3 I/O address

	EDATA?S




EXTERN _CtGetRelocationAddress

DGROUP	GROUP	_BSS, _DATA

	ASSUME	DS: DGROUP



	CODES	opl3



; !!!
;	InitFMDriver( ioAddr)
;		unsigned ioAddr;	/* IO Base Addr of OPL-III
;
BEGIN	_InitFMDriver
	push	bp
	mov	bp, sp

	MCALL	_CtGetRelocationAddress
	mov	opl3_io, ax

; Low-level init:
	call	LowLevelInit

; Enable perc mode:
	mov	ax, 1
	push	ax
	MCALL	_SetPercModeOpl3

; enable all 4-op voices:
	mov	ax, 03fh		; mask for all 4-op voices ON
	push	ax
	MCALL	_Set4opMaskOpl3
	add	sp, 4

	pop	bp
	ret
_InitFMDriver	ENDP




;	CloseFMDriver()
;
; Set the OPL-III chip in YM3812 mode. To be called before
; quitting application
;
BEGIN _CloseFMDriver

	call	LowLevelInit

	mov	dx, opl3_io
	add	dx, 2
	mov	ax, 0005H		; reset NEW bit at addr 5
	call	OutOpl3
	ret
_CloseFMDriver ENDP




; Low-level init...
LowLevelInit PROC NEAR
	push	si
	push	di

; clear some variables...
	mov	bx, 20-1
sm_loop:
	mov	levelVc[ bx], 0		; max level
	mov	transpVc[ bx], 0	; no transposition
	mov	b0RegVc[ bx], 0		; all channel OFF
	mov	noteVc[ bx], 0		; note value OFF
	mov	c0RegVc[ bx], 30H	; ext 1 & 0 OFF, left & right ON
	dec	bx
	jge	sm_loop

	mov	bx, (20-1)*2
sm_lp2:
	mov	pitchBendVc[ bx], 02000h ; normal pitch bend
	sub	bx, 2
	jge	sm_lp2



; clear all OPL-III registers, except register 5
	mov	dx, opl3_io
	mov	ch, 2
sm_lp4:
	mov	ax, 0105H		; set NEW bit of bank to 1
	call	OutOPL3
sm_suit1:
	mov	cl, 0f5h
sm_lp3:
	xor	ah, ah
	mov	al, cl
	cmp	al, 5			; register 5 ?
	jne	sm_ok1
; register 5:
	cmp	ch, 1			; test if second bank
	jne	sm_ok1			; skip if not
	inc	ah		; set low bit (NEW) of second bank to 1
sm_ok1:
	call	OutOPL3
	dec	cl
	jne	sm_lp3
	add	dx, 2
	dec	ch
	jne	sm_lp4

; set output left and right;
	mov	dx, opl3_io
	mov	ax, 2001h		; wave-form enable, register 1 (3812 only)
	call	OutOPL3
	mov	ch, 2
sm_lp6:
	mov	cl, 9
sm_lp5:
	mov	ah, 030h		; ext-out 1, 0 OFF; Left & Right ON
	mov	al, cl
	add	al, 0c0h-1
	call	OutOPL3
	dec	cl
	jne	sm_lp5
	add	dx, 2
	dec	ch
	jne	sm_lp6


; set release-rate of each slots to 15:
	mov	dx, opl3_io		; first bank
	mov	ch, 2
re_lp7:
	mov	si, offset CS: slotOffsets
	mov	cl, 18
re_lp8:
	mov	ah, 15			; release-rate = 15
	mov	al, cs:[si]
	inc	si
	add	al, 80H
	call	OutOPL3
	dec	cl
	jne	re_lp8
	add	dx, 2			; next bank
	dec	ch
	jne	re_lp7


; perc-mode == OFF, 4-op voices disabled:
	xor	ax, ax
	mov	bdReg, al		; AM & VIB depth = 0, RHY = 0, perc bits = 0
	mov	dx, opl3_io
	mov	al, 0bdh
	call	OutOPL3
	xor	ah, ah
	mov	mask4op, ah		; disable all 4 op voices
	mov	al, 4
	add	dx, 2
	call	OutOPL3

; first 15 2-op melodic voices:
	mov	bx, 14
	mov	al, TYPE_2OP
	mov	ah, 2
re_lp1:
	mov	typeVc[ bx], al		; TYPE_2OP
	mov	nbOpVc[ bx], ah		; 2 op per voice
	dec	bx
	jge	re_lp1

; perc slots: melodic mode
	mov	typeVc+15, TYPE_2OP
	mov	nbOpVc+15, 2
	mov	typeVc+16, TYPE_2OP
	mov	nbOpVc+16, 2
	mov	typeVc+17, TYPE_PERC + TYPE_DISABLE	; SD
	mov	nbOpVc+17, 1
	mov	typeVc+18, TYPE_2OP
	mov	nbOpVc+18, 2
	mov	typeVc+19, TYPE_PERC + TYPE_DISABLE	; CYMB
	mov	nbOpVc+19, 1

	pop	di
	pop	si
	ret

slotOffsets:
	db	0, 1, 2, 3, 4, 5
	db	8, 9, 0Ah, 0Bh, 0Ch, 0Dh
	db	10h, 11h, 12h, 13h, 14h, 15h

LowLevelInit ENDP




; Disable/enable melodic voices 15, 16, 18; enable/disable percussive
; voices 15 - 19
;
;	SetPercModeOpl3( state)
;		int state;
;
SPM_ struc
	dw	(?)	; old bp
	db	CPSIZE DUP (?)	; return addr
spm_state	dw	(?)	; 0: melodic, else perc
SPM_ ends

BEGIN	_SetPercModeOpl3
	push	bp
	mov	bp, sp
	push	si


	cmp	[ bp].spm_state, 0	; test if perc. mode
	jne	spm_perc
; melodic mode:
	mov	typeVc+15, TYPE_2OP
	mov	typeVc+16, TYPE_2OP
	mov	nbOpVc+16, 2
	mov	typeVc+17, TYPE_PERC + TYPE_DISABLE	; SD
	mov	typeVc+18, TYPE_2OP
	mov	nbOpVc+18, 2
	mov	typeVc+19, TYPE_PERC + TYPE_DISABLE	; CYMB

	xor	ah, ah			; clear RHY bit
	jmp	spm_suit2

; percussif mode:
spm_perc:
	mov	typeVc+15, TYPE_PERC	; BD
	mov	typeVc+16, TYPE_PERC	; HH
	mov	nbOpVc+16, 1
	mov	typeVc+17, TYPE_PERC	; SD
	mov	typeVc+18, TYPE_PERC	; TOM
	mov	nbOpVc+18, 1
	mov	typeVc+19, TYPE_PERC	; CYMB

; preset frequency for TOM & HH (they control the last 4 perc)
	mov	al, TOM_PITCH		; TOM pitch is FIXED!!
	mov	noteVc+TOM, al
	mov	ax, TOM
	push	ax			; voice
	call	near ptr UpdateFreqOpl3
	add	sp, 2
	mov	al, TOM_PITCH +TOM_TO_HH ; HH pitch is FIXED too!!
	mov	noteVc+HH, al
	mov	ax, HH
	push	ax
	call	near ptr UpdateFreqOpl3
	add	sp, 2

; initialise MULTI of HH & CYMB perc voices to 1, because they influence
; the timbre of SD, TOM, HH, CYMB:
	mov	dx, opl3_io
	mov	ax, 0131h		; set MULTI of HH slot to 1
	call	OutOpl3
	mov	ax, 0135h		; set MULTI of CYMB slot to 1
	call	OutOpl3
	
	mov	ah, 32			; set RHY bit	

spm_suit2:
	mov	dx, opl3_io
	mov	al, bdReg
	and	al, 0c0h
	or	ah, al
	mov	bdReg, ah
	mov	al, 0bdH
	call	OutOPL3			; and send to chip

	
	pop	si
	pop	bp
	ret
_SetPercModeOpl3 ENDP




; Enable/disable 4-op voices. There is a maximum of 6 4-op voices.
; Each 4-op voice use 2 2-op voices. The 6 low bits of 'mask'
; specifies if corresponding voice (from bit-0 to bit 5) is enable or
; disable. Enabling one 4-op voice will automaticaly disable two 2-op voices.
;
; Set4opMaskOpl3( mask)
;	unsigned mask;
;
BEGIN _Set4opMaskOpl3
s4_ struc
s4_local	dw	(?)		; old BP
		db	CPSIZE DUP (?)	; ret. addr
s4_mask		dw	(?)		; mask, 6 low bits
s4_ ends

	push	bp
	IF s4_local NE 0
	sub	sp, s4_local
	ENDIF
	mov	bp, sp
	push	si

	mov	ax, [ bp].s4_mask
	and	ax, 3fH			; keep low 6 bits
	mov	mask4op, al

	mov	cx, 6
	xor	si, si
s4_lp1:
	shr	al, 1			; test low bit
	jnc	s4_off
; ON:
	mov	typeVc[ si], TYPE_4OP
	mov	nbOpVc[ si], 4
	mov	typeVc[ si +1], TYPE_2OP + TYPE_DISABLE
	jmp	s4_suit
s4_off:
	mov	typeVc[ si], TYPE_2OP
	mov	nbOpVc[ si], 2
	mov	typeVc[ si +1], TYPE_2OP
	
s4_suit:
	add	si, 2
	loop	s4_lp1


; send 4-op bits to OPL-III:
	mov	dx, opl3_io
	add	dx, 2
	mov	ah, mask4op
	mov	al, 4
	call	OutOPL3	

	pop	si
	IF s4_local
	add	sp, s4_local
	ENDIF
	pop	bp
	ret
_Set4opMaskOpl3 ENDP





; SetGlobalOpl3( nSel, amD, vibD, pBRange)
;	int nSel, amD, vibD;
;	int pBRange;
BEGIN _SetGlobalOpl3
sg_ struc
sg_local	dw	(?)		; old BP
		db	CPSIZE DUP (?)	; ret. addr
sg_nSel		dw	(?)		; Note-Sel
sg_amD		dw	(?)		; AM-Depth
sg_vibD		dw	(?)		; VIB-Depth
sg_pBR		dw	(?)		; pitch bend range 1 - 12
sg_ ends

	push	bp
	IF sg_local NE 0
	sub	sp, sg_local
	ENDIF
	mov	bp, sp

	mov	dx, opl3_io
	mov	ax, [bp].sg_nSel
	or	ax, ax
	je	sg_ok1
; noteSel ON:
	mov	ah, 040h
sg_ok1:
	mov	al, 8
	call	OutOPL3

	xor	ah, ah
	mov	cx, [bp].sg_amD
	or	cx, cx
	je	sg_ok2
; am-Depth ON
	mov	ah, 080h
sg_ok2:
	mov	cx, [bp].sg_vibD
	or	cx, cx
	je	sg_ok3
; vib-Depth ON
	or	ah, 040h
sg_ok3:
	mov	al, bdReg
	and	al, NOT 0c0h		; clear AM & VIB bits
	or	al, ah			; set AM & VIB bits value
	mov	bdReg, al
	mov	ah, al
	mov	al, 0bdh		; register addr
	call	OutOPL3

; set the new pitch bend range
	mov	ax, [ bp].sg_pBR
	cmp	ax, 12
	jle	sg_ok4
	mov	ax, 12
	jmp	sg_ok5
sg_ok4:
	cmp	ax, 1
	jge	sg_ok5
	mov	ax, 1
sg_ok5:
	mov	pitchRange, ax

	IF sg_local NE 0
	add	sp, sg_local
	ENDIF
	pop	bp
	ret
_SetGlobalOpl3 ENDP




;	PresetOpl3( voice, dP)
;		0 <= int voice <= 19
;		char dP[ 28];
po3_ struc
po3_prmOff	dw	(?)
po3_count	db	(?)
		db	(?)		; filler
po3_local	dw	(?)		; old BP
		db	CPSIZE DUP (?)	; ret. addr
po3_voice	dw	(?)
po3_dP		db	DPSIZE DUP (?)	; far pointer to preset data
po3_ ends

BEGIN _PresetOpl3

	push	bp
	IF po3_local NE 0
	sub	sp, po3_local
	ENDIF
	mov	bp, sp
	push	si
	push	di

	mov	bx, [bp].po3_voice
	cmp	bx, LAST_VC
	jbe	po3_9
po3_10:
	jmp	po3_bad
po3_9:
	mov	al, typeVc[ bx]
	test	al, TYPE_DISABLE	; verify if voice is valid
	jne	po3_10

	test	al, TYPE_4OP
	je	po3_1
; voix 4op
	mov	si, offset DGROUP: carMsk4op
	jmp	po3_suite

po3_1:
; voix 2-op ou Perc
	mov	si, offset DGROUP: carMsk2op

po3_suite:
	mov	al, levelVc[ bx]	; get voice volume (log)
	xor	ah, ah
	push	ax

	IF LDATA
	les	di, dword ptr [bp].po3_dP
	ELSE
	mov	di, word ptr [bp].po3_dP
	push	ds
	pop	es
	ENDIF

	mov	bl, es:[ di +PRESET_CONNECT]	; get first connection bit
	mov	bh, bl
	mov	cl, 1
	and	bl, cl
	rol	bh, cl
	and	bh, cl
	rol	bh, cl			; second connection bit
	or	bl, bh			; algo #
	xor	bh, bh
	mov	dh, [si+bx]		; get carrier mask value

	mov	bx, [bp].po3_voice
	cmp	nbOpVc[ bx], 1		; test if HH to CYMB voice..
	jne	po3_2b
; > BD:	one slot voice ==> carrier mask = 1
	mov	dh, 1
po3_2b:
	mov	carMaskVc[ bx], dh	; save mask to array


	mov	al, nbOpVc[ bx]
	mov	[bp].po3_count, al
	xor	cx, cx
	mov	[bp].po3_prmOff, cx
po3_loop:
	mov	bx, [bp].po3_prmOff
	lea	si, [di+bx]
	mov	al, es:[ si+4]		; get 'defined bit'
	or	al, al			; test sign bit
	jns	po3_nextSlot
	push	es
	push	si			; data pointer

	mov	al, dh
	and	ax, 1			; test carrier bit
	push	ax

	mov	bx, [bp].po3_voice
	shl	bx, 1
	mov	si, offset DGROUP: vcToSlot
	add	si, cx			; + slot number
	mov	al, [ si +bx]		; get slot offset
	xor	ah, ah
	push	ax

	call	SendOper
	add	sp, 4*2

po3_nextSlot:
	add	byte ptr [bp].po3_prmOff, PRESET_OPR_SIZE
	shr	dh, 1
	inc	cx
	dec	[bp].po3_count
	jg	po3_loop

; all slots have been sent...
	pop	ax			; clean up stack

	mov	bx, [bp].po3_voice
	mov	al, es:[ di +PRESET_TRANSP]	; transposition
	mov	transpVc[ bx], al

	mov	dx, opl3_io
	mov	al, es:[ di +PRESET_CONNECT]	; get FB & connect
	and	al, 0fh
	mov	ah, c0RegVc[ bx]
	and	ah, 0f0h
	or	ah, al
	mov	c0RegVc[ bx], ah	; save to array
	mov	al, vcToChannel[ bx]	; get first offset for C0-C8 registers
	or	al, al			; test sign bit for validity
	js	po3_5			; no channel...
	cmp	al, 020H
	jl	po3_4
; second bank
	sub	al, 020h
	add	dx, 2
po3_4:
	add	al, 0c0H
	call	OutOPL3
	
	cmp	nbOpVc[ bx], 4
	jne	po3_5			; only one connect bit

	mov	al, es:[ di + PRESET_CONNECT]	; get second connect bit
	rol	al, 1
	and	al, 1
	mov	ah, c0RegVc[ bx +1]
	and	ah, 0f0h
	or	ah, al
	mov	c0RegVc[ bx +1], ah	; save to array
	mov	al, vcToChannel[ bx +1]	; get second offset for C0-C8 registers
	or	al, al
	js	po3_5
	and	al, NOT 020H		; clear first/second bank indicator
	add	al, 0c0H		; add offset of C0-C8 reg.
	call	OutOPL3
po3_5:

po3_bad:
	pop	di
	pop	si
	IF po3_local NE 0
	add	sp, po3_local
	ENDIF
	pop	bp
	ret
_PresetOpl3 ENDP






;	LevelOpl3( voice, level)
;		0 <= voice <= 19
;		0 <= level <= 127 (linear)
BEGIN _LevelOpl3
lev_s struc
lev_local dw	(?)		; old bp
	db	CPSIZE DUP (?)	; return addr
lev_voice dw	(?)
lev_level dw	(?)
lev_s ends

	push	bp
	IF lev_local NE 0
	sub	sp, lev_local
	ENDIF
	mov	bp, sp
	push	si
	push	di

	mov	bx, [bp].lev_voice
	cmp	bx, LAST_VC
	ja	lev_bad
	test	typeVc[ bx], TYPE_DISABLE
	jne	lev_bad			; jump if voice is invalid
	mov	ch, levelVc[ bx]
	shl	bx, 1
	lea	di, vcToSlot[ bx]

	mov	bx, [bp].lev_level
	cmp	bx, 127			; level is MIN( level, 127)
	jbe	lev_ok
; overflow:
	mov	bx, 127
lev_ok:
	mov	al, linToLog[ bx]	; convert volume to log. value
	mov	bx, [bp].lev_voice
	mov	levelVc[ bx], al	; set new voice level
	mov	ch, al			; preserve voice level
	
	mov	cl, carMaskVc[ bx]	; get voice carrier mask
	mov	bl, nbOpVc[ bx]		; get number of slots for this voice
	mov	si, bx			; loop counter
lev_lp:
	mov	bl, [di]		; get slot offset
	inc	di			; point to next slot
	or	bl, bl			; test if valid slot
	js	lev_next		; skip if not
	shr	cl, 1			; test if carrier slot
	jnc	lev_next		; skip if not
	xor	bh, bh
	mov	al, kslLevSlots[ bx]	; get slot KSL-LEVEL
	mov	ah, al
	and	ah, 3fh			; keep Total-Level bits only
	add	ah, ch			; + logarithm voice volume
	cmp	ah, MAX_LOG_VOL		; test if overflow
	jbe	lev_ok2			; skip if yes
; overflow
	mov	ah, MAX_LOG_VOL		; set maximum volume
lev_ok2:
	and	al, NOT 3fh		; keep KSL
	or	ah, al			; add to resulting level
	mov	dx, opl3_io
	mov	al, bl			; slot offset
	and	bl, 20h			; test if second bank
	je	lev_ok3			; skip if not
; bank 2
	and	al, NOT 20h
	add	dx, 2			; second bank
lev_ok3:
	add	al, 40h			; KSL, TOTAL-LEVEL register offset
	call	OutOPL3
lev_next:
	dec	si			; loop count --
	jne	lev_lp

lev_bad:
lev_ret:
	pop	di
	pop	si
	IF lev_local NE 0
	add	sp, lev_local
	ENDIF
	pop	bp
	ret
_LevelOpl3 ENDP





;	NoteOnOpl3( voice, note);
;		0 <= voice <= 19
;		12 <= note <= 107
;
BEGIN _NoteOnOpl3
nop3 struc
nop3_local 	dw	(?)		; old bp
		db	CPSIZE DUP (?)	; return addr
nop3_voice 	dw	(?)
nop3_note	dw	(?)
nop3 ends

	push	bp
	IF nop3_local NE 0
	sub	sp, nop3_local
	ENDIF
	mov	bp, sp

	mov	bx, [bp].nop3_voice
	cmp	bx, LAST_VC
	ja	nop3_bad
	mov	al, typeVc[ bx]
	test	al, TYPE_DISABLE	; voice valid ?
	jne	nop3_bad

	test	al, TYPE_PERC
	jne	nop3_perc		; it's a percussion voice
; melodic voice:
	or	b0RegVc[ bx], 20H	; set KON bit
	mov	ax, [bp].nop3_note
	mov	noteVc[ bx], al
	push	bx			; voice
	call	near ptr UpdateFreqOpl3
	add	sp, 2
	jmp	nop3_ret

nop3_perc:
	cmp	bx, BD			; Base Drum ??
	jg	nop3_others
nop3_bd:
	mov	al, byte ptr [bp].nop3_note
	mov	noteVc[ bx], al
	push	bx			; voice
	call	near ptr UpdateFreqOpl3
	add	sp, 2

nop3_others:
nop3_pbits:
; others (SD, CYMB, HH) + (BD, TOM)
	mov	ah, percBits[ bx - FIRST_PERC]
	or	ah, bdReg
	mov	bdReg, ah
	mov	al, 0bdh
	mov	dx, opl3_io
	call	OutOPL3

nop3_bad:
nop3_ret:
	IF nop3_local NE 0
	add	sp, nop3_local
	ENDIF
	pop	bp
	ret
_NoteOnOpl3 ENDP




;	NoteOffOpl3( voice);
;		0 <= voice <= 19
;
BEGIN _NoteOffOpl3
nfop3 struc
nfop3_local 	dw	(?)		; old bp
		db	CPSIZE DUP (?)	; return addr
nfop3_voice 	dw	(?)
nfop3 ends

	push	bp
	IF nfop3_local NE 0
	sub	sp, nfop3_local
	ENDIF
	mov	bp, sp

	mov	bx, [bp].nfop3_voice
	cmp	bx, LAST_VC
	ja	nfop3_bad
	mov	al, typeVc[ bx]
	test	al, TYPE_DISABLE	; voice valid ?
	jne	nfop3_bad

	test	al, TYPE_PERC
	jne	nfop3_perc		; it's a percussion voice
; melodic voice:
	mov	ah, b0RegVc[ bx]
	and	ah, NOT 20H		; reset KON bit
	mov	b0RegVc[ bx], ah
	mov	dx, opl3_io
	mov	al, vcToChannel[ bx]	; get offset of channel
	test	al, 20h			; test bank indicator
	je	nfop3_first
; second bank
	and	al, NOT 20h		; clear bank indicator
	add	dx, 2			; point to next bank
nfop3_first:
	add	al, 0b0H
	call	OutOPL3			; update B0-B8 register
	jmp	nfop3_ret

; others (BD, SD, TOM, CYMB, HH)
nfop3_perc:
	mov	ah, percBits[ bx - FIRST_PERC]
	not	ah
	and	ah, bdReg		; reset perc bit
	mov	bdReg, ah
	mov	al, 0bdh		; perc bits register offset
	mov	dx, opl3_io
	call	OutOPL3

nfop3_bad:
nfop3_ret:
	IF nfop3_local NE 0
	add	sp, nfop3_local
	ENDIF
	pop	bp
	ret
_NoteOffOpl3 ENDP



;	PitchBendOpl3( voice, pitchBend)
;		0 <= int voice <= BD
;		0 <= unsigned pitchBend <= 0x3fff
;
BEGIN _PitchBendOpl3
pb struc
pb_local 	dw	(?)		; old bp
		db	CPSIZE DUP (?)	; return addr
pb_voice 	dw	(?)
pb_pitchBend	dw	(?)
pb ends

	push	bp
	IF pb_local NE 0
	sub	sp, pb_local
	ENDIF
	mov	bp, sp

	mov	bx, [bp].pb_voice
	cmp	bx, LAST_VC
	ja	pb_bad
	test	typeVc[ bx], TYPE_DISABLE	; voice valid ?
	jne	pb_bad

	cmp	nbOpVc[ bx], 1
	jle	pb_ret			; no pitch bend for SD, TOM, CYMB & HH

; melodic voice or BD:
	mov	ax, [bp].pb_pitchBend
	cmp	ax, 03fffH
	jbe	pb_ok
; overflow
	mov	ax, 03fffH
pb_ok:
	push	bx			; voice
	add	bx, bx			; offste in pitchBendVc array
	mov	pitchBendVc[ bx], ax
	call	near ptr UpdateFreqOpl3
	add	sp, 2
pb_bad:
pb_ret:
	IF pb_local NE 0
	add	sp, pb_local
	ENDIF
	pop	bp
	ret
_PitchBendOpl3 ENDP




;	LeftRightOpl3( voice, lR)
; lR ==> 0: center, 1: left, 2: right
;
BEGIN _LeftRightOpl3
lr_ struc
lr_local 	dw	(?)		; old bp
		db	CPSIZE DUP (?)	; return addr
lr_voice 	dw	(?)
lr_lrbits	dw	(?)
lr_ ends

	push	bp
	IF lr_local NE 0
	sub	sp, lr_local
	ENDIF
	mov	bp, sp
	push	si

	mov	bx, [bp].lr_voice
	cmp	bx, LAST_VC
	ja	lr_bad
	test	typeVc[ bx], TYPE_DISABLE	; voice valid ?
	jne	lr_bad

	mov	si, [bp].lr_lrbits
	cmp	si, 2
	jbe	lr_ok
; overflow
	xor	si, si
lr_ok:
	mov	al, vcToChannel[ bx]		; get channel offset
	or	al, al
	js	lr_bad			; pas de correspondance
	mov	dx, opl3_io
	test	al, 20h			; second bank ?
	je	lr_ok2
; bank 2
	add	dx, 2
	and	al, NOT 20H
lr_ok2:
	add	al, 0c0h
	mov	ah, leftRightBits[ si]
	mov	cl, c0RegVc[ bx]	; get current C0-C8 register val.
	and	cl, NOT 30H		; clear stereo bits
	or	ah, cl
	mov	c0RegVc[ bx], ah	; save new value of reg.
	call	OutOPL3

; do the same thing for second group
	cmp	nbOpVc[ bx], 4			; verify if 4-op voice
	jne	lr_ret

	mov	al, vcToChannel[ bx +1]
	and	al, NOT 20h		; clear bank indicator
	add	al, 0c0h
	mov	ah, leftRightBits[ si]
	mov	cl, byte ptr c0RegVc[ bx +1]	; get current C0-C8 register val.
	and	cl, NOT 30H		; clear stereo bits
	or	ah, cl
	mov	c0RegVc[ bx +1], ah	; save new value of register
	call	OutOPL3

lr_bad:
lr_ret:
	pop	si
	IF lr_local NE 0
	add	sp, lr_local
	ENDIF
	pop	bp
	ret
_LeftRightOpl3 ENDP



;/*
; Set the frequency of voice 'voice' to note number noteVc[ vc] +transpVc[ vc],
; shifting the note by 'pitchBendVc[ vc]/0x2000' of 'pitchRange' (global, 1-12).
;*/
;
;static int UpdateFreqOpl3( voice)
; 	int voice;		/* [0, 19]
; {
; int tblValue, tNote;
; unsigned t1, t2;
; unsigned tableOff;
; unsigned fNLow, fNHigh;
; int block;
; int signP;

UpdateFreqOpl3 PROC NEAR
SF_F struc
sf_note	dw	(?)
sf_local dw	(?)	; old bp
	dw	(?)	; ret. addr.
voice	dw	(?)
SF_F ends

	push	bp
	IF sf_local NE 0
	sub	sp, sf_local
	ENDIF
	mov	bp, sp
	push	si
	push	di

	mov	bx, [bp].voice

	mov	al, DELTA_PITCH		; convert from MID_C = 60 to MID_C = 48
	add	al, transpVc[ bx]	; add transposition of voice timbre
	add	al, noteVc[ bx]		; add note value
	mov	byte ptr [bp].sf_note, al

; signP = (int)pitch - 0x2000;
	add	bx, bx			; offset in pitchBendVc array
	mov	ax, pitchBendVc[ bx]
	sub	ax, 2000H
	je	after_mul		; if 0, by-pass multiplication...

; t2 = signP >> (13 -LOG_PITCH);
	IF log_pitch NE 8
	*** code non prevu pour log_pitch != 8
	ENDIF
	sar	ax, 1
	sar	ax, 1
	sar	ax, 1
	sar	ax, 1
	sar	ax, 1

; t2 *= pBRange;	/* 8 bits apres le point */
	IF 0
	IF log_pitch NE 8
	*** code non prevu pour log_pitch != 8: multiplication 8 bits
	ENDIF
	imul	pitchRange	; t2		... byte ptr ???

	ELSE
	mov	cx, ax
	mov	bx, pitchRange
	shl	bx, 1
	neg	bx
	add	bx, offset CS: add_table+2
	jmp	bx
	EVEN
	add	ax, cx		; x 12
	add	ax, cx		; x 11
	add	ax, cx		; x 10
	add	ax, cx		; x 9
	add	ax, cx		; x 8
	add	ax, cx		; x 7
	add	ax, cx		; x 6
	add	ax, cx		; x 5
	add	ax, cx		; x 4
	add	ax, cx		; x 3
	add	ax, cx		; x 2
add_table:			; x 1
	ENDIF

; t1 = note << LOG_PITCH;
; tNote = (t1 + t2);
after_mul:
	IF log_pitch NE 8
 	*** code non defini pour log_pitch != 8
	ENDIF
	add	ah, byte ptr [bp].sf_note

; /* arrondir a 0.5: */
; tNote += ( 1 << LOG_PITCH - LOG_NB_STEP_PITCH -1);
	add	ax, (1 SHL (log_pitch - log_nb_step_pitch -1))

; tNote >>= LOG_PITCH - LOG_NB_STEP_PITCH;	/* 4 bits apres le point */
	IF (log_pitch - log_nb_step_pitch) NE 4
	*** code non prevu pour (log_pitch - log_nb_step_pitch) != 4
	ENDIF
	sar	ax, 1
	sar	ax, 1
	sar	ax, 1
	sar	ax, 1

; /* attention de ne pas deborder de [0,96[ ... */
; if( tNote < 0)
	jge	l3

; tNote = 0;
	xor	ax, ax
	jmp	l4

; if( tNote >= NB_NOTES << LOG_NB_STEP_PITCH)
l3:	cmp	ax, (nb_notes SHL log_nb_step_pitch)-1
	jl	l4

; tNote = (NB_NOTES << LOG_NB_STEP_PITCH) -1;
	mov	ax, (nb_notes SHL log_nb_step_pitch)-1

; tableOff = divMod12[ (tNote >> LOG_NB_STEP_PITCH)] & 0xf;
l4:
	mov	di, ax
	IF log_nb_step_pitch NE 4
	*** code non prevu pour log_nb_step_pitch != 8
	ENDIF
	shr	di, 1
	shr	di, 1
	shr	di, 1
	shr	di, 1
	mov	dx, di		; tNote >> log_nb_step_pitch
	mov	bl, divMod12[ di]
	and	bl, 0fH		; keep modulo 12 part
	xor	bh, bh
	mov	di, bx

; tableOff <<= LOG_NB_STEP_PITCH +1;
	IF log_nb_step_pitch NE 4
	*** code non prevu pour log_nb_step_pitch != 4
	ENDIF
	shl	di, 1
	shl	di, 1
	shl	di, 1
	shl	di, 1
	shl	di, 1

; tableOff +=	(tNote << 1) & (NB_STEP_PITCH *2 -1);
	shl	ax, 1
	and	ax, (nb_step_pitch *2 -1)
	add	di, ax

; tblValue = * (int *)((char *)fNumTbl +tableOff);
	mov	ax, fNumTbl[ di]

; block = (divMod12[ (tNote >> LOG_NB_STEP_PITCH)] >> 4) -1;
	mov	di, dx
	mov	bl, divMod12[ di]
	shr	bl, 1
	shr	bl, 1
	shr	bl, 1
	shr	bl, 1
	dec	bl

; block += tblValue < 0 ? 1 : 0;
	or	ax, ax
	jge	l5
	inc	bl

; if( block < 0) {
l5:
	or	bl, bl
	jge	l6

; block++;
	inc	bl

; tblValue >>= 1;
; }
	sar	ax, 1
l6:


; OutOPL3( 0xA0 +voice % 9, tblValue & 255);
	push	bx		; save block value

	mov	cx, ax		; save tblValue
	mov	dx, opl3_io	; io addr
	mov	bx, [bp].voice
	mov	si, offset DGROUP: vcToChannel
	lea	si, [si +bx]
	mov	al, [si]	; get channel offset (+20h if bank 2)
	or	al, al		; check if we can change freq. for this voice
	js	sf_ret		; jump if no...
	test	al, 020h	; check if bank 2
	je	sf_10
; bank 2
	add	dx, 2
	and	al, NOT 020h
sf_10:
	add	al, 0a0H	; F-Number(Low) offset
	mov	ah, cl		; f-num low value
	call	OutOPL3

; OutOPL3( 0xB0 +voice % 9, (block << 2) + (tblValue >> 8) & 3);
	mov	ah, b0RegVc[ bx]	; get current register value
	and	ah, NOT 01fH	; mask off block & f-num(h)
	and	ch, 3		; keep only f-num(h) 2 low bits
	or	ah, ch
	pop	cx		; restore block #
	and	cl, 7		; keep block field
	add	cl, cl
	add	cl, cl		; shift block two bits left
	or	ah, cl
	mov	b0RegVc[ bx], ah	; save new register value

	mov	al, [si]	; get channel offset
	and	al, NOT 20h	; clear bank indicator
	add	al, 0b0h
	call	OutOPL3
	
; }
sf_ret:
	pop	di
	pop	si
	IF sf_local NE 0
	add	sp, sf_local
	ENDIF
	pop	bp
	ret

UpdateFreqOpl3 ENDP






; send parameters of base addr 20h, 40h, 60h, 80h & 0e0h
;
sop_ struc
sop_local dw	(?)	; old bp
	dw	(?)	; return addr
sop_slot dw	(?)	; slot offset (+20H if second bank)
sop_carrier dw	(?)	; != 0 if it's a carrier slot
sop_data dd	(?)
sop_vcVol dw	(?)	; voice number, [0 - 28]
sop_ ends

SendOper PROC NEAR
	push	bp
	IF sop_local NE 0
	sub	sp, sop_local
	ENDIF
	mov	bp, sp
	push	di
	push	es
	push	ax
	push	bx
	push	cx
	push	dx

	mov	dx, opl3_io
	mov	al, byte ptr [bp].sop_slot
	or	al, al			; test sign bit for validity
	js	sop_ret

	cmp	al, 020h		; first or second bank ?
	jl	sop_ok1
; bank #2
	sub	al, 020h
	add	dx, 2			; point to second bank
sop_ok1:
	mov	cl, al
	les	di, dword ptr [bp].sop_data	; get pointer to preset data
	add	al, 020H		; AM VIB EG_TYPE KSR MULTI
	mov	ah, es:[ di]
	inc	di
	call	OutOPL3

	mov	ah, es:[ di]
	inc	di
	mov	bx, [bp].sop_slot
	mov	kslLevSlots[ bx], ah
	cmp	byte ptr [bp].sop_carrier, 0
	je	sop_ok2
; carrier:
	mov	al, ah
	and	al, 3fH			; mask off KSL
	add	al, byte ptr [bp].sop_vcVol
	cmp	al, 03fH
	jle	sop_ok3
; overflow:
	mov	al, 03fH
sop_ok3:
	and	ah, 0c0H
	or	ah, al
sop_ok2:
	mov	al, cl
	add	al, 040H
	call	OutOPL3
	
	mov	ah, es:[ di]		; AR  DR
	inc	di
	mov	al, cl
	add	al, 060H
	call	OutOPL3

	mov	ah, es:[ di]		; SL  RR
	inc	di
	mov	al, cl
	add	al, 080H
	call	OutOPL3

	mov	ah, es:[ di]		; WS
	and	ah, 07h
	inc	di
	mov	al, cl
	add	al, 0e0H
	call	OutOPL3

sop_ret:
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	pop	es	
	pop	di
	IF sop_local NE 0
	add	sp, sop_local
	ENDIF
	pop	bp
	ret
SendOper ENDP



; Send data byte to OPL3
; input:
;	DX:	I/O base addr	(bank 1 or 2)
;	AL:	register #
;	AH:	data
; lost:
;	AL, AH
OutOPL3 PROC NEAR

	pushf
	cli

	IF 0
; garder une copie des valeurs transferees au chip...
	push	bx
	xor	bh, bh
	mov	bl, al
	test	dx, 2
	je	o3_1
; second map
	mov	_map1[ bx], ah
	jmp	o3_2
; first map
o3_1:
	mov	_map0[ bx], ah
o3_2:
	pop	bx
	ENDIF


	out	dx, al

 if 0		; pour que ca marche sur l'ancienne carte...
 in al, dx
 in al, dx
 in al, dx
 in al, dx
 in al, dx
 endif

;	in	al, dx	; delay (delay >= 280 ns) between addr and data write
	in	al, 20H		; delay ...
	in	al, 20H		; delay ... for 33 MHz machines
	mov	al, ah		; get data byte
	inc	dx		; point to data register
	out	dx, al
	dec	dx		; restore orginal address

 if 0		; pour que ca marche sur l'ancienne carte...
 in al, dx
 in al, dx
 in al, dx
 in al, dx
 in al, dx
 in al, dx
 in al, dx
 in al, dx
 in al, dx
 in al, dx

 in al, dx
 in al, dx
 in al, dx
 in al, dx
 in al, dx
 in al, dx
 in al, dx
 in al, dx
 in al, dx
 in al, dx

 in al, dx
 in al, dx
 in al, dx
 in al, dx
 in al, dx
 in al, dx
 in al, dx
 in al, dx
 in al, dx
 in al, dx

 in al, dx
 in al, dx
 in al, dx
 in al, dx
 in al, dx
 in al, dx
 in al, dx
 in al, dx
 in al, dx
 in al, dx

 in al, dx
 in al, dx
 in al, dx
 in al, dx
 in al, dx
 endif
	popf

	ret
OutOPL3 ENDP



	ECODES	opl3
	END

