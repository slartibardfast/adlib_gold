
#ifndef    _GLOBAL_H
#define    _GLOBAL_H

/*
 * Some general definition for pretty printing considerations.
 */

#ifndef	__WINDOWS_H

#define	LPSTR           char *     // all strings are near in small model

typedef 	char *          FarPtr;
typedef    unsigned int    WORD;
typedef    unsigned char   BOOL;

typedef    unsigned char   BYTE;
typedef    unsigned char   Byte;
typedef    unsigned long   DWORD;

#define    false			0
#define    true			1
#define    FAR				far
#endif	  /* WINDOWS_H */

#define    FALSE           0
#define    TRUE            1

#define    PRIVATE			static
#define    PUBLIC
#define    AND				&&
#define    OR				||

#if	!defined(MK_FP)
#define	MK_FP(x,y)	((void far *)(((unsigned long)(x)<<16)|(unsigned)(y)))
#endif	/* MK_FP */

#ifdef MICROSOFT
#define	enable()		_enable()
#define	disable()		_disable()
#define	outport(a,d)	outpw((a),(d))
#define	outportb(a,d)	outp((a),(d))
#define	inportb(p)		inp(p)
#define	setvect(n,p)	_dos_setvect(n,p)
#define	getvect(n)		_dos_getvect(n)
#define	asm				_asm
#endif /* MICROSOFT */

#ifdef TURBO
#define	_enable()				enable()		
#define	_disable()				disable()		
// #define	outp(a,d)				outportb(a,d)	
//#define	inp(p)					inportb(p)		
#define	_dos_setvect(n,p)		setvect(n,p)	
#define	_dos_getvect(n)			getvect(n)		
#endif	 /* TURBO */

#endif
