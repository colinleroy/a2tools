#ifndef __extended_conio_h
#define __extended_conio_h

#ifdef __CC65__
#include <conio.h>
#else
#include <stdio.h>
#endif
#include <string.h>

#ifndef __extended_conio
#define __extended_conio

#ifndef __CC65__
#define __fastcall__
#define cgetc() fgetc(stdin)
#define cputs(s) puts(s)
#define cputc(c) putc(c, stdout)
#define clrscr()
#define wherex() 1
#define wherey() 1
#define chlinexy(a, b, c) do {} while(0)
#define cvlinexy(a, b, c) do {} while(0)
#define gotoxy(a, b) do {} while(0)
#define cprintf printf
#endif


char * __fastcall__ cgets(char *buf, size_t size);
void printfat(char x, char y, char clear, const char* format, ...);
void __fastcall__ clrzone(char xs, char ys, char xe, char ye);

#endif

#endif
