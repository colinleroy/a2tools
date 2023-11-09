/*
 * Copyright (C) 2022 Colin Leroy-Mira <colin@colino.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include "clrzone.h"
#include "extended_conio.h"

static unsigned char scrw = 255, scrh = 255;

#ifndef __CC65__
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

int exec(const char *cmd, const char *params) {
  printf("%s %s\n", cmd, params);
  exit(0);
  return 0;
}

void clrscr(void) {
  fprintf(stdout, "%c[H", CH_ESC);
  fprintf(stdout, "%c[J", CH_ESC);
  fprintf(stdout, "%c[H", CH_ESC);
}
void gotoxy(int x, int y) {
  fprintf(stdout, "%c[%d;%dH", CH_ESC, y + 1, x + 1);
}
void gotox(int x) {
  int y = wherey();
  fprintf(stdout, "%c[%d;%dH", CH_ESC, y + 1, x + 1); /* fixme not 0 y */
}
void gotoy(int y) {
  int x = wherex();
  fprintf(stdout, "%c[%d;%dH", CH_ESC, y + 1, x + 1); /* fixme not 0 x */
}

char cgetc(void) {
  char c;
  static int unset_canon = 0;
  static struct termios ttyf;

  if (!unset_canon) {
    tcgetattr( STDIN_FILENO, &ttyf);
    ttyf.c_lflag &= ~(ICANON);
    tcsetattr( STDIN_FILENO, TCSANOW, &ttyf);
    unset_canon = 1;
  }
  fread(&c, 1, 1, stdin);
  if (c == '\33') {
    fread(&c, 1, 1, stdin);
    if (c == '[') {
      fread(&c, 1, 1, stdin);
      switch (c) {
        case 'A': return CH_CURS_UP;
        case 'B': return CH_CURS_DOWN;
        case 'C': return CH_CURS_RIGHT;
        case 'D': return CH_CURS_LEFT;
      }
    } else {
      return c;
    }
  }
  return c;
}
int kbhit(void) {
  static int n = 0;
  if (n > 1) {
    n--;
    return 1;
  }
  return (ioctl(fileno(stdin), FIONREAD, &n) == 0 && n > 0);
}

void cputs(char *buf) {
  printf("%s", buf);
  fflush(stdout);
}

void cputsxy(unsigned char x, unsigned char y, char *buf) {
  if (scrw == 255 && scrh == 255) {
    screensize(&scrw, &scrh);
  }

  gotoxy(x, y);
  printf("%s", buf);
  fflush(stdout);
}

void cputcxy(unsigned char x, unsigned char y, char buf) {
  if (scrw == 255 && scrh == 255) {
    screensize(&scrw, &scrh);
  }

  gotoxy(x, y);
  printf("%c", buf);
  fflush(stdout);
}

void dputsxy(unsigned char x, unsigned char y, char *buf) {
  if (scrw == 255 && scrh == 255) {
    screensize(&scrw, &scrh);
  }

  gotoxy(x, y);
  printf("%s", buf);
  fflush(stdout);
}

void dputc(char buf) {
  printf("%c", buf);
}

void dputcxy(unsigned char x, unsigned char y, char buf) {
  if (scrw == 255 && scrh == 255) {
    screensize(&scrw, &scrh);
  }

  gotoxy(x, y);
  printf("%c", buf);
  fflush(stdout);
}

void screensize(unsigned char *w, unsigned char *h) {
  *w = 40;
  *h = 24;
}

void chline(int len) {
  int i = 0;
  for (i = 0; i < len; i++) {
    printf("_");
  }
}
#endif

void printxcentered(int y, char *buf) {
  int len = strlen(buf);
  int startx;

  if (scrw == 255 && scrh == 255) {
    screensize(&scrw, &scrh);
  }

  startx = (scrw - len) / 2;
  gotoxy(startx, y);
  cputs(buf);
}

void printxcenteredbox(int width, int height) {
  int startx, starty, i;
  char *line = malloc(width + 1);

  if (scrw == 255 && scrh == 255) {
    screensize(&scrw, &scrh);
  }

  startx = (scrw - width) / 2;
  starty = (scrh - height) / 2;
  
  clrzone(startx, starty, startx + width - 1, starty + height);
  
  for (i = 0; i < width; i++) {
    line[i] = '-';
  }
  line[0] = '+';
  line[i - 1] = '+';
  line[i] = '\0';
  
  gotoxy(startx, starty);
  puts(line);
  for (i = 1; i < height + 1; i++) {
    cputsxy(startx, starty + i,  "!");
    cputsxy(startx + width - 1, starty + i, "!");
  }
  gotoxy(startx, starty + height + 1);
  puts(line);
  
  free(line);
}

#ifndef __CC65__

static int get_pos(int *x, int *y) {

 char buf[30]={0};
 int ret, i, pow;
 char ch;

*y = 0; *x = 0;

 struct termios term, restore;

 tcgetattr(0, &term);
 tcgetattr(0, &restore);
 term.c_lflag &= ~(ICANON|ECHO);
 tcsetattr(0, TCSANOW, &term);

 write(1, "\033[6n", 4);

 for( i = 0, ch = 0; ch != 'R'; i++ )
 {
    ret = read(0, &ch, 1);
    if ( !ret ) {
       tcsetattr(0, TCSANOW, &restore);
       fprintf(stderr, "getpos: error reading response!\n");
       return 1;
    }
    buf[i] = ch;
 }

 if (i < 2) {
    tcsetattr(0, TCSANOW, &restore);
    printf("i < 2\n");
    return(1);
 }

 for( i -= 2, pow = 1; buf[i] != ';'; i--, pow *= 10)
     *y = *y + ( buf[i] - '0' ) * pow;

 for( i-- , pow = 1; buf[i] != '['; i--, pow *= 10)
     *x = *x + ( buf[i] - '0' ) * pow;

 tcsetattr(0, TCSANOW, &restore);
 return 0;
}
int wherex(void) {
  int x = 0, y = 0;
  get_pos(&x, &y);

  return x - 1;
}

int wherey(void) {
  int x = 0, y = 0;
  get_pos(&x, &y);

  return y - 1;
}
#endif
