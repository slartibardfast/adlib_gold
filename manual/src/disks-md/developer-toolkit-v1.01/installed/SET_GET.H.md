# `developer-toolkit-v1.01/installed/SET_GET.H`

> UTF-8 rendering of a DOS-encoded (CP437 / CRLF) file. Byte-for-byte original: [`SET_GET.H`](../../../disks/developer-toolkit-v1.01/installed/SET_GET.H).

```c
#ifdef __cplusplus
extern "C" {
#endif
#ifndef TURBO
	void far cdecl _dos_setvect( unsigned int intnum, void (interrupt far* handler)());
   void (interrupt far * _dos_getvect(unsigned int intnum))();
	int cdecl printf(const char *format,...)  ;
#endif
#ifdef __cplusplus
	};
#endif

#if 0

#undef	outportb
#undef	inportb
#undef	outp
#undef	inp

int  outp( unsigned port, int byte);
int  inp(unsigned port );

#endif

```
