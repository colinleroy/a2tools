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

char * __fastcall__ cgets(char *buf, size_t size) {
#ifdef __CC65__
  char c;
  size_t i = 0, max_i = 0;
  int cur_x;
  int prev_cursor = 0;
  
  prev_cursor = cursor(1);
  
  while (i < size - 1) {
    cur_x = wherex();

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
    gotox(cur_x);
    
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
static char clearbuf[39];
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
