#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "extended_conio.h"

#ifndef __CC65__
static void cputsxy(int x, int y, char *buf) {
  char *prefix = malloc(39);
  int i;
  
  for (i = 0; i < x && i < 38; i++) {
    prefix[i] = ' ';
  }
  prefix[i] = '\0';
  if (strlen(buf) > 39 - x) {
    buf[39 - x] = '\0';
  }
  printf("%s%s", prefix, buf);
  free(prefix);
}
#endif

static unsigned char scrw = 255, scrh = 255;

#ifndef __CC65__
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

char * __fastcall__ cgets(char *buf, size_t size) {
#ifdef __CC65__
  char c;
  size_t i = 0, max_i = 0;
  int cur_x, cur_y;
  int prev_cursor = 0;
  
  if (scrw == 255 && scrh == 255) {
    screensize(&scrw, &scrh);
  }

  prev_cursor = cursor(1);
  
  while (i < size - 1) {
    cur_x = wherex();
    cur_y = wherey();

    c = cgetc();

    if (c == CH_ENTER) {
      break;
    } else if (c == CH_CURS_LEFT) {
      if (i > 0) {
        i--;
        cur_x--;
      }
    } else if (c == CH_CURS_RIGHT) {
      if (i < max_i) {
        i++;
        cur_x++;
      }
    } else {
      cputc(c);
      buf[i] = c;
      i++;
      cur_x++;
    }
    if (i > max_i) {
      max_i = i;
    }

    if (cur_x > scrw - 1) {
      cur_x = 0;
      cur_y++;
      if (cur_y > scrh - 1) {
        gotoxy(scrw - 1, scrh - 1);
        printf("\n");
        cputcxy(scrw - 1, scrh - 2, c);
        cur_y--;
      }
    } else if (cur_x < 0) {
      cur_x = scrw - 1;
      cur_y--;
    }
    gotoxy(cur_x, cur_y);
    
  }
  cursor(prev_cursor);
  printf("\n");
  buf[i] = '\0';

  return buf;
#else
  return fgets(buf, size, stdin);
#endif
}

static char printfat_buf[255];
static char clearbuf[42];
void printfat(char x, char y, char clear, const char* format, ...) {
  int i;
  va_list args;

  va_start(args, format);
  vsnprintf(printfat_buf, 255, format, args);
  va_end(args);

  printfat_buf[39 - x] = '\0';
  cputsxy(x, y, printfat_buf);
  if (clear) {
    x = wherex();
    i = 0;
    while (++x < 40) {
      clearbuf[i++] = ' ';
    }
    clearbuf[i++] = '\0';
    cputs(clearbuf);
  }
}

void __fastcall__ clrzone(char xs, char ys, char xe, char ye) {
  int l = xe - xs + 1;
  memset(clearbuf, ' ', l);
  clearbuf[l + 1] = '\0';

  while (ys < ye) {
    cputsxy(xs, ys, clearbuf);
    ys++;
  }
}

void printxcentered(int y, char *buf) {
  int len = strlen(buf);
  int startx;

  if (scrw == 255 && scrh == 255) {
    screensize(&scrw, &scrh);
  }

  startx = (scrw - len) / 2;
  printfat(startx, y, 0, buf);
}

void printxcenteredbox(int width, int height) {
  int startx, starty, i;
  char *line = malloc(width);

  if (scrw == 255 && scrh == 255) {
    screensize(&scrw, &scrh);
  }

  startx = (scrw - width) / 2;
  starty = (scrh - height) / 2;
  
  clrzone(startx, starty, startx + width, starty + height + 1);
  
  for (i = 0; i < width; i++) {
    line[i] = '-';
  }
  line[0] = '+';
  line[i - 1] = '+';
  line[i] = '\0';
  
  printfat(startx, starty, 0, "%s", line);
  for (i = 1; i < height + 1; i++) {
    cputsxy(startx, starty + i,  "!");
    cputsxy(startx + width - 1, starty + i, "!");
  }
  printfat(startx, starty + height + 1, 0, "%s", line);
  
  free(line);
}
