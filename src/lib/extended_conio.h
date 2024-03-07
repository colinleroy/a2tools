#ifndef __extended_conio_h
#define __extended_conio_h

#ifdef __CC65__
  #include <conio.h>

  #ifdef __APPLE2__
    #define CH "$24"
    #define gotox(x)                \
        (                           \
            __A__=(x),              \
            asm("sta "CH)           \
        )
    #define wherex()                \
        (                           \
            asm("lda "CH),          \
            asm("ldx #$00"),        \
            __AX__                  \
        )

  #endif

#else
  #include <stdio.h>
  #include <string.h>

  #define CH_DEL         0x7F
  #define CH_CURS_UP     0x01
  #define CH_CURS_DOWN   0x02
  #define CH_CURS_LEFT   0x03
  #define CH_CURS_RIGHT  0x05
  #define CH_ENTER       '\n'
  #define CH_ESC         0x1B
  #define CH_LLCORNER    'L'

/* Apple II values */
  #define APPLE_CH_CURS_LEFT  0x08
  #define APPLE_CH_CURS_RIGHT 0x15
  #define APPLE_CH_CURS_DOWN  0x0A
  #define APPLE_CH_CURS_UP    0x0B
  
  #define __fastcall__
  #define cputc(c) putc(c, stdout)
  #define cursor(a) do {} while(0)
  #define dputs(a) cputs(a)
  #define chlinexy(a, b, c) do {} while(0)
  #define cvlinexy(a, b, c) do {} while(0)
  #define cvline(a) do {} while(0)
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
  void dputsxy(unsigned char x, unsigned char y, char *buf);
  void dputcxy(unsigned char x, unsigned char y, char buf);
  void cputs(char *buf);
  void cputsxy(unsigned char x, unsigned char y, char *buf);
  void cputcxy(unsigned char x, unsigned char y, char buf);
  int wherex(void);
  int wherey(void);
  int kbhit(void);
  int exec(const char *cmd, const char *params);
#endif

void printxcentered(int y, char *buf);
void printxcenteredbox(int width, int height);
#endif
