	/*
	patch entre microsoft et borlandc 
	*/

#include <stdio.h>
#include <dos.h>

#include "set_get.h"

#ifndef TURBO

void far cdecl
_dos_setvect( unsigned int intnum, void (interrupt far* handler)())
   {
   setvect((int) intnum, handler);
   }

void (interrupt far *far _dos_getvect(unsigned int intnum))()
   {
   return( getvect((int )intnum));
   }

int printf(const char *format,...)
  {
  return(0);
  }


#endif

#if 0

int outp( unsigned port, int byte)
   {
   outportb((int)port, (unsigned char)byte );
   return(0);
   }

int inp(unsigned port )
   {
   return( (unsigned char )  inportb((int) port ));
   }

#endif
