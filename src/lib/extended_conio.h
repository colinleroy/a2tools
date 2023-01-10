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

#define CH_CURS_UP     'i'
#define CH_CURS_DOWN   'k'
#define CH_CURS_LEFT   'j'
#define CH_CURS_RIGHT  'l'
#define CH_ENTER       '\n'
#define CH_ESC         0x1B

#define __fastcall__
#define cputs(s) puts(s)
#define cputc(c) putc(c, stdout)
#define cursor(a) do {} while(0)
#define wherex() 1
#define cpeekc() 1
#define wherey() 1
#define chlinexy(a, b, c) do {} while(0)
#define cvlinexy(a, b, c) do {} while(0)
#define cprintf printf
#define revers(x) 1
#define videomode(x) 1
#define VIDEOMODE_40COL 1
#define VIDEOMODE_80COL 1

void clrscr(void);
void gotoxy(int x, int y);
void gotox(int x);
void gotoy(int y);
char cgetc(void);
void screensize(unsigned char *w, unsigned char *h);
void chline(int len);
int kbhit(void);
#endif


char * __fastcall__ cgets(char *buf, size_t size);
void __fastcall__ clrzone(char xs, char ys, char xe, char ye);

void printxcentered(int y, char *buf);
void printxcenteredbox(int width, int height);
#endif

#endif
