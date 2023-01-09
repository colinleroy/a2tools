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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "surl.h"
#include "simple_serial.h"
#include "extended_conio.h"
#ifndef __CC65__
#include <termios.h>
#include <unistd.h>
#endif
#include "math.h"

#define BUFSIZE 255
static char *buf;
static int translate_ln = 0;

#define CTRL_STEP_START  0
#define CTRL_STEP_X      1
#define CTRL_STEP_XY_SEP 2
#define CTRL_STEP_Y      3
#define CTRL_STEP_CMD    4

static unsigned char scrw, scrh;

static char handle_escape_sequence(void) {
  char o;
  char cur_x = 0, cur_y = 0;
  char x = 0, y = 0;
  char command, has_bracket = 0;
  char step = CTRL_STEP_START;

  /* We're here after receiving an escape. Now do the vt100 stuff */
  while (step != CTRL_STEP_CMD) {
#ifdef __CC65__
    while (ser_get(&o) == SER_ERR_NO_DATA);
#else
    o = simple_serial_getc();
#endif
    if (o == (char)EOF) {
      return EOF;
    }
    if (step == CTRL_STEP_START && o == '[') {
      has_bracket = 1;
    } else if (o >= '0' && o <= '9') {
      if (step == CTRL_STEP_START)
        step = CTRL_STEP_X;
      if (step == CTRL_STEP_XY_SEP)
        step = CTRL_STEP_Y;
      if (step == CTRL_STEP_X)
        x = x * 10 + (o - '0');
      else
        y = y * 10 + (o - '0');
    } else if (o == ';') {
      step = CTRL_STEP_XY_SEP;
    } else {
      step = CTRL_STEP_CMD;
      command = o;
    }
  }

  /* We should have our command by now */
  switch(command) {
    /* cursor home */
    case 'H':
    case 'f':
      gotoxy(x,y);
      break;
    /* cursor up */
    case 'A':
curs_up:
      cur_y = wherey();
      gotoy(min(0, cur_y - (y == 0 ? 1 : y)));
      break;
    /* cursor down */
    case 'B': 
curs_down:
      cur_y = wherey();
      gotoy(max(scrh - 1, cur_y + (y == 0 ? 1 : y)));
      break;
    /* cursor right */
    case 'C': 
      cur_x = wherex();
      gotox(max(scrw - 1, cur_x + (x == 0 ? 1 : x)));
      break;
    /* cursor left, or down, depending on bracket */
    case 'D': 
      if (!has_bracket) goto curs_down;
      cur_x = wherex();
      gotox(min(0, cur_x - (x == 0 ? 1 : x)));
      break;
    /* cursor up if no bracket */
    case 'M':
      goto curs_up;
      break;
    /* next line (CRLF) if no bracket */
    case 'E':
      if (!has_bracket) {
        printf("\n"); /* printf to handle scroll */
      }
      break;
    /* request for status */
    case 'n':
      if (x == 5) {
        /* terminal status */
        simple_serial_printf("%c[0n", CH_ESC);
      } else if (x == 6) {
        /* cursor position report */
        simple_serial_printf("%c[%d;%dR", CH_ESC, wherex(), wherey());
      }
      break;
    /* request to identify terminal type */
    case 'c':
    case 'Z':
      simple_serial_printf("%c[?1;0c", CH_ESC); /* VT100 */
      break;
    /* Erase to beginning/end of line (incl) */
    case 'K':
      if (x == 0) clrzone(wherex(), wherey(), scrw-1, wherey());
      if (x == 1) clrzone(0, wherey(), wherex(), wherey());
      if (x == 2) clrzone(0, wherey(), scrw-1, wherey());
      break;
    /* Erase to beginning/end of screen (incl), only full screen supported */
    case 'J':
      clrscr();
      break;
  }
}

/* 80*25 + 1 */
static char scrbuf[2001] = {0};
static int scridx = 0;
static char flush_cnt = 0;

void flush_scrbuf(void) {
  scrbuf[scridx] = '\0';
  ++scridx;
  fputs(scrbuf, stdout);
  fflush(stdout);
  flush_cnt = 0;
  scridx = 0;
}

int main(int argc, char **argv) {
  surl_response *response = NULL;
  char *buffer = NULL;
  char i, o;

#ifdef __CC65__
  videomode(VIDEOMODE_80COL);
#endif
  screensize(&scrw, &scrh);

#ifndef __CC65__
  static struct termios ttyf;

  tcgetattr( STDOUT_FILENO, &ttyf);
  ttyf.c_lflag &= ~(ICANON);
  tcsetattr( STDOUT_FILENO, TCSANOW, &ttyf);
#endif

again:
  clrscr();
  if (argc > 1) {
    buf = strdup(argv[1]);
    argc = 1; /* Only first time */
  } else {
    char t;

    buf = malloc(BUFSIZE);
    cputs("Enter host:port: ");
    cgets(buf, BUFSIZE);
    cputs("Translate \\n <=> \\r\\n (Y/n)? ");
    t = cgetc();
    translate_ln = (t != 'n' && t != 'N');
  }

  if (strchr(buf, '\n'))
    *strchr(buf, '\n') = '\0';

  response = surl_start_request("RAW", buf, NULL, 0);
  if (response == NULL) {
    printf("No response.\n");
    exit(1);
  }
  i = '\0';
#ifdef __CC65__
  puts("\n");
#endif

  do {
    if (kbhit()) {
      i = cgetc();
      if (i == '\r') {
        if (translate_ln) {
          simple_serial_puts("\r\n");
        } else {
          simple_serial_putc('\n');
        }
      } else {
#ifdef __CC65__
        ser_put(i);
#else
        simple_serial_putc(i);
#endif
      }
#ifdef __CC65__
      /* Echo */
      if (i == '\r') {
        printf("\n");
      } else {
        cputc(i);
      }
#endif
    }

    ++flush_cnt;
#ifdef __CC65__
    while (ser_get(&o) != SER_ERR_NO_DATA) {
#else
    while ((o = simple_serial_getc_immediate()) != EOF && o != '\0') {
#endif
      if (o == '\r' && translate_ln) {
        continue;
      } else if (o == 0x04) {
        flush_scrbuf();
        goto remote_closed;
      } else if (o == CH_ESC) {
        flush_scrbuf();
        if (handle_escape_sequence() == (char)EOF) {
          goto remote_closed;
        }
      } else {
        if (scridx == 2000)
          flush_scrbuf();
        scrbuf[scridx] = o;
        ++scridx;
        if (o == '\n' || flush_cnt == 255)
          flush_scrbuf();
      }
    }
  } while(i != 0x04);

remote_closed:
  printf("\ndone\n");
  surl_response_free(response);
  free(buffer);
  free(buf);

  goto again;
  
  exit(0);
}
