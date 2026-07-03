;.DOSSEG

;================================= MACROS ==================================
.proc   macro   name
    name    proc near
endm

.endp   macro   name
    name    endp
endm

.push           macro   r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10
  IFNB <r10>
.error  <.PUSH has more than 10 arguments>
  ENDIF
  irp $reg,<r0,r1,r2,r3,r4,r5,r6,r7,r8,r9>
    IFB <$reg>
      exitm
    ELSEIFIDNI <$reg>,<FLAGS>
		pushf
    ELSE
		push    $reg
    ENDIF
  endm
endm

.pop            macro   r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10
  IFNB <r10>
.error  <.POP has more than 10 arguments>
  ENDIF
  irp $reg,<r9,r8,r7,r6,r5,r4,r3,r2,r1,r0>
    IFNB <$reg>
      IFIDNI <$reg>,<FLAGS>
		popf
      ELSE
		pop     $reg
      ENDIF
    ENDIF
  endm
endm

FlagOn  MACRO  bit_flag
	or   BYTE PTR cs: act_flags, bit_flag
ENDM

FlagOff MACRO  bit_flag
	and   BYTE PTR cs: act_flags, NOT bit_flag
ENDM

SS_SIZE          equ     1000h       ;stack size

;These are bit flags used in the variable 'act_flags'.  A bit set will keep
;the TSR from activating.
BIT_05           equ     01h
BIT_08           equ     02h
BIT_09           equ     04h
BIT_10           equ     08h
BIT_13           equ     10h
BIT_28           equ     20h
BIT_APP_UP       equ     40h
BIT_HOTKEY       equ      80h

;Defines for main window
WIN1_WIDE   equ    38		    ;width of main window
WIN1_HIGH   equ    14		    ;height of main window
COL11       equ    14               ;screen col where window begins
ROW11       equ    4                ;screen row where window begins
COL1_R      equ    (WIN1_WIDE-3)    ;rightmost col in window for data

MOUS_SIZE   equ    400h
STRN_SIZE   equ    256

;interrupt vector numbers
BIOS_KEYBOARD_INT               equ     09H
BIOS_VIDEO_INT                  equ     10H
DOS_FUNCTION_CALL               equ     21H

;function call  numbers
DOS_PRINT_STRING_FUNCTION       equ      9H
BIOS_KEYBOARD_IO_CALL           equ     16H
DOS_TSR_TERMINATE               equ     31H
GET_DOS_VERSION                 equ     30H
GET_INDOS_ADDRESS               equ     34H
DOS_GET_VECTOR                  equ     35H
DOS_OPEN_HANDLE                 equ     3DH
DOS_CLOSE_HANDLE                equ     3EH
DOS_READ_HANDLE          	equ     3FH
DOS_TERMINATE                   equ     4cH
FREE_MEM                        equ     49H
SET_CURRENT_ID                  equ     50H
GET_CURRENT_ID                  equ     51H

; These are bit flags used in the variable 'act_flags'.  A bit set will keep
; the TSR from activating.

BIT_08           		equ     02h
BIT_10           		equ     08h
BIT_13           		equ     10h
BIT_28           		equ     20h
BIT_APP_UP       		equ     40h
BIT_HOTKEY       		equ     80h


; @codesize is predefined by MASM: 0 = small, compact; 1 = med, large, huge
;@codesize  equ  0
IF  @codesize
   CPSIZE  EQU 	4
ELSE
   CPSIZE  EQU  2
ENDIF

;============================ DATA STRUCTURES ==============================

; This structure is used for installing and removing ISR's.  "num" is the
; int number, "new" is the ptr to the new ISR and old is the (4 byte) ptr
; to the previously installed ISR.  "num" is declared as a word so that
; "new" ptr is on a 2 byte boundary and "old" is on a 4 byte boundary.

ISR   STRUC
   num          dw      0
   new          dw      0
   old          dd      0
ISR   ENDS

; This structure is used for passing arguments to subroutines.

PASS_ARGS struc
	dw  (?) ; old bp
	db  CPSIZE DUP (?)  ; return addr
arg1    dw  (?)
arg2    dw  (?)
arg3    dw  (?)
PASS_ARGS ends


;================================= MACROS ==================================

;Saves all registers except (cs, ds, ss and sp) in a given memory location.

Reg_Save    MACRO   mem
	mov	mem, si
	mov	mem+2, di
	mov	mem+4, bp
	mov	mem+6, es
	mov	mem+8, ax
	mov	mem+10, bx
	mov	mem+12, cx
	mov	mem+14, dx
ENDM

;Restore the registers stored by the above macro.
Reg_Restore MACRO   mem
	mov	si, mem
	mov	di, mem+2
	mov	bp, mem+4
	mov	es, mem+6
	mov	ax, mem+8
	mov	bx, mem+10
	mov	cx, mem+12
	mov	dx, mem+14
ENDM

