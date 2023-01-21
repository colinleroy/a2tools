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

#define CH_CURS_UP     0x01
#define CH_CURS_DOWN   0x02
#define CH_CURS_LEFT   0x03
#define CH_CURS_RIGHT  0x05
#define CH_ENTER       '\n'
#define CH_ESC         0x1B

#define __fastcall__
#define cputs(s) puts(s)
#define cputc(c) putc(c, stdout)
#define cursor(a) do {} while(0)
#define cputcxy(a,b,c) do {} while(0)
#define dputcxy cputcxy
#define dputsxy cputsxy
#define dputs cputs
#define chlinexy(a, b, c) do {} while(0)
#define cvlinexy(a, b, c) do {} while(0)
#define cprintf printf
#define revers(x) do {} while(0)
#define videomode(x) do {} while(0)
#define VIDEOMODE_40COL 1
#define VIDEOMODE_80COL 1

void clrscr(void);
void gotoxy(int x, int y);
void gotox(int x);
void gotoy(int y);
char cgetc(void);
void screensize(unsigned char *w, unsigned char *h);
void chline(int len);
void cputsxy(int x, int y, char *buf);
int wherex(void);
int wherey(void);
int kbhit(void);
#endif

void get_scrollwindow(unsigned char *top, unsigned char *bottom);
void set_scrollwindow(unsigned char top, unsigned char bottom);
void echo(int on);
char * __fastcall__ cgets(char *buf, size_t size);
void __fastcall__ clrzone(char xs, char ys, char xe, char ye);

void printxcentered(int y, char *buf);
void printxcenteredbox(int width, int height);
void progress_bar(int x, int y, int width, size_t cur, size_t end);
#endif

#endif
