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
static char translate_ln = 0;

#define CURS_MODE_CURSOR '['
#define CURS_MODE_APPLICATION 'O'

static char cursor_mode = CURS_MODE_CURSOR;

#define AUTOWRAP_NO 'l'
#define AUTOWRAP_YES 'h'
static char autowrap_mode = AUTOWRAP_NO;

#define CTRL_STEP_START  0
#define CTRL_STEP_X      1
#define CTRL_STEP_XY_SEP 2
#define CTRL_STEP_Y      3
#define CTRL_STEP_CMD    5

static unsigned char scrw, scrh;

static unsigned char top_line = 255, btm_line = 255;

typedef struct _vt100_ctrl {
  char way;
  char abs;
  char x;
  char y;
} vt100_ctrl;

static vt100_ctrl vt100_ctrl_queue[100];
static char n_vt100_ctrls = 0;

#define GOTOXY 0
#define CURSOR 1
#define CLRSCR 2
#define SCROLL_WINDOW 3
#define NEXTLINE 4
#define STATUSREQ 5
#define TERMTYPE 6
#define AUTOWRAP 7
// Dont go up to 8 it's CH_CURS_LEFT
// Back after CH_ESC (1B)
#define SAVE_CURSOR 0x30
#define RESTORE_CURSOR 0x31

static char saved_vt100_curs_x = 255, saved_vt100_curs_y = 255;

#pragma optimize(push, on)
static char scrollbuf[80];
static void manual_scroll_up(char num) {
  static signed char l, x;
  static char h;
  
  h = btm_line - top_line;

  for (l = h - 1 - num; l >= 0; l--) {
    gotoy(l);
    for (x = 0; x < scrw; x++) {
      gotox(x);
      scrollbuf[x] = cpeekc();
    }
    scrollbuf[x] = '\0';
    cputsxy(0, l + num, scrollbuf);
  }
  clrzone(0, 0, scrw - 1, num - 1);
}
#pragma optimize(pop)

static void do_vt100_ctrl(int vt100_ctrl_index) {
  unsigned char cur_x = 0, cur_y = 0;
  char way = vt100_ctrl_queue[vt100_ctrl_index].way;
  char abs = vt100_ctrl_queue[vt100_ctrl_index].abs;
  char x = vt100_ctrl_queue[vt100_ctrl_index].x;
  char y = vt100_ctrl_queue[vt100_ctrl_index].y;

  if (way == CLRSCR) {
    if (abs == 1) {
      clrscr();
    } else {
      cur_x = wherex(); cur_y = wherey();
      if (x == 0) clrzone(cur_x, cur_y, scrw-1, cur_y);
      if (x == 1) clrzone(0, cur_y, cur_x, cur_y);
      if (x == 2) clrzone(0, cur_y, scrw-1, cur_y);
      gotoxy(cur_x, cur_y);
    }
  } else if (way == CURSOR) {
    cursor_mode = x;
  } else if (way == SAVE_CURSOR) {
    saved_vt100_curs_x = wherex();
    saved_vt100_curs_y = wherey();
  } else if (way == RESTORE_CURSOR) {
    if (saved_vt100_curs_x != 255) {
      gotoxy(saved_vt100_curs_x, saved_vt100_curs_y);
    }
  } else if (way == AUTOWRAP) {
    autowrap_mode = x;
  } else if (way == NEXTLINE) {
    printf("\n"); /* printf to handle scroll */
  } else if (way == STATUSREQ) {
    if (x == 5) {
      /* terminal status */
      simple_serial_puts("\33[0n");
    } else if (x == 6) {
      /* cursor position report */
      simple_serial_printf("\33[%d;%dR", wherex(), wherey());
    }
  } else if (way == TERMTYPE) {
    simple_serial_printf("\33[?1;0c"); /* VT100 */
  } else if (way == CH_CURS_LEFT) {
    cur_x = wherex();
    gotox(max(0, cur_x - x));
  } else if (way == CH_CURS_RIGHT) {
    cur_x = wherex();
    gotox(min(scrw - 1, cur_x + x));
  } else if (way == CH_CURS_UP) {
    cur_y = wherey();
    if (cur_y - x >= top_line) {
      gotoy(max(top_line, cur_y - x));
    } else
      manual_scroll_up(x);
  } else if (way == CH_CURS_DOWN) {
    cur_y = wherey();
    gotoy(min(btm_line - 1, cur_y + x));
  } else if (way == SCROLL_WINDOW) {
    top_line = x - 1;
    btm_line = y;
    set_scrollwindow(top_line, btm_line);
  } else if (way == GOTOXY) {
    if (x != 255 && y != 255)
      gotoxy(x, y);
    else if (x != 255)
      gotox(x);
    else if (y != 255)
      gotoy(y);
  }
}

static void enqueue_vt100_ctrl(char way, char abs, char x, char y) {
  if (n_vt100_ctrls == 99)
    return; /* FIXME keep enqueing somehow */
  
  if (n_vt100_ctrls > 0 &&
    vt100_ctrl_queue[n_vt100_ctrls - 1].way == way &&
    vt100_ctrl_queue[n_vt100_ctrls - 1].y == 0 && 
    abs == 0 && 
    (way == CH_CURS_UP || way == CH_CURS_DOWN || way == CH_CURS_LEFT || way == CH_CURS_RIGHT)) {
    /* merge with previous */
    vt100_ctrl_queue[n_vt100_ctrls - 1].x += x;
  } else {
    vt100_ctrl_queue[n_vt100_ctrls].way = way;
    vt100_ctrl_queue[n_vt100_ctrls].abs = abs;
    vt100_ctrl_queue[n_vt100_ctrls].x = x;
    vt100_ctrl_queue[n_vt100_ctrls].y = y;
    n_vt100_ctrls++;
  }
}

static void flush_vt100_ctrls(void) {
  char i;
  for (i = 0; i < n_vt100_ctrls; i++) {
    do_vt100_ctrl(i);
  }
  n_vt100_ctrls = 0;
}

char mid_vt100 = 0;

char buffer[1024];
unsigned int buf_idx = 0;
unsigned int cur_read_idx = 0;

static int handle_vt100_escape_sequence(char pretend) {
  char o;
  char x = 0, y = 0;
  char command, has_bracket = 0;
  char step = CTRL_STEP_START;
  char cmd1, cmd2;

  /* We're here after receiving an escape. Now do the vt100 stuff */
  while (step != CTRL_STEP_CMD) {
    if (pretend) {
      o = simple_serial_getc();
      if (o == (char)EOF) {
        return EOF;
      }
      buffer[buf_idx++] = o;
    } else {
      o = buffer[cur_read_idx++];
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
      else if (step = CTRL_STEP_Y)
        y = y * 10 + (o - '0');

      if (!has_bracket && step == CTRL_STEP_X) {
        /* we can have ^[<n>
         * and 7 and 8 don't have a following letter */
         if (x == 7) {
           if (!pretend)
            enqueue_vt100_ctrl(SAVE_CURSOR, 1, 0, 0);
           return 0;
         } else if (x == 8) {
           if (!pretend)
            enqueue_vt100_ctrl(RESTORE_CURSOR, 1, 0, 0);
           return 0;
         }
       }
    } else if (o == ';') {
      step = CTRL_STEP_XY_SEP;
    } else {
      step = CTRL_STEP_CMD;
      command = o;
    }
  }

  if (command == '?') {
    cmd1 = pretend ? simple_serial_getc() : buffer[cur_read_idx++];
    cmd2 = pretend ? simple_serial_getc() : buffer[cur_read_idx++];

    if (pretend) {
      buffer[buf_idx++] = cmd1;
      buffer[buf_idx++] = cmd2;
      return 0;
    }

    if (cmd1 == '1' && cmd2 == 'h') {
      enqueue_vt100_ctrl(CURSOR, 1, CURS_MODE_APPLICATION, 0);
      return 0;
    }
    if (cmd1 == '1' && cmd2 == 'l') {
      enqueue_vt100_ctrl(CURSOR, 1, CURS_MODE_CURSOR, 0);
      return 0;
    }
    if (cmd1 == '8' && (cmd2 == 'h' || cmd2 == 'l')) {
      enqueue_vt100_ctrl(AUTOWRAP, 1, cmd2, 0);
      return 0;
    }
    if (cmd1 == '2') {
      x = 0;
      while (cmd2 >= '0' && cmd2 <= '9') {
        x = x * 10 + (cmd2 - '0');
        if (pretend) {
          cmd2 = simple_serial_getc();
          buffer[buf_idx++] = cmd2;
        } else {
          cmd2 = buffer[cur_read_idx++];
        }
      }
      /* Now we should have 'h' or 'l' */
      if (cmd2 == 'h' || cmd2 == 'l') {
        /* Ignore it. It's probably \33[?2004h or 2004l,
         *, bracketed paste mode, and I don't know what do
         * do with it. */
      }
      return 0;
    }
  }

  if (pretend) {
    return 0;
  }
  /* VT100 counts from 1 */

  /* We should have our command by now */
  switch(command) {
    /* cursor home */
    case '=':
    case '>':
      break;
    case 'H':
      if(!has_bracket) break;
    case 'f':
      enqueue_vt100_ctrl(GOTOXY, 1, (y > 0 ? y - 1 : 0), (x > 0 ? x - 1 : 0));
      break;
    /* cursor up */
    case 'A':
curs_up:
      enqueue_vt100_ctrl(CH_CURS_UP, 0, (x == 0 ? 1 : x), 0);
      break;
    /* cursor down */
    case 'B': 
curs_down:
      enqueue_vt100_ctrl(CH_CURS_DOWN, 0, (x == 0 ? 1 : x), 0);
      break;
    /* cursor right */
    case 'C': 
      enqueue_vt100_ctrl(CH_CURS_RIGHT, 0, (x == 0 ? 1 : x), 0);
      break;
    /* cursor left, or down, depending on bracket */
    case 'D': 
      if (!has_bracket)
        goto curs_down;
      enqueue_vt100_ctrl(CH_CURS_LEFT, 0, (x == 0 ? 1 : x), 0);
      break;
    /* cursor up if no bracket */
    case 'M':
      goto curs_up;
      break;
    /* next line (CRLF) if no bracket */
    case 'E':
      if (!has_bracket) {
        enqueue_vt100_ctrl(NEXTLINE, 1, 0, 0);
      }
      break;
    /* request for status */
    case 'n':
      enqueue_vt100_ctrl(STATUSREQ, 1, x, 0);
      break;
    /* request to identify terminal type */
    case 'c':
    case 'Z':
      enqueue_vt100_ctrl(TERMTYPE, 1, 0, 0);
      break;
    /* Erase to beginning/end of line (incl) */
    case 'K':
      enqueue_vt100_ctrl(CLRSCR, 0, x, 0);
      break;
    /* Erase to beginning/end of screen (incl), only full screen supported */
    case 'J':
      enqueue_vt100_ctrl(CLRSCR, 1, 0, 0);
      break;
    /* Setwin (top-bottom lines )*/
    case 'r':
      if (!has_bracket)
        break;
      enqueue_vt100_ctrl(SCROLL_WINDOW, 1, x, y);
      break;
  }
  return 0;
}

#define TELNET_SBE  '\360' /* 240 end of subnegociation parameters */
#define TELNET_NOP  '\361' /* 241 No operation */
#define TELNET_DM   '\362' /* 242 Data mark */
#define TELNET_BRK  '\363' /* 243 Break */
#define TELNET_INT  '\364' /* 244 Suspend, interrupt or abort */
#define TELNET_AO   '\365' /* 245 Abort output */
#define TELNET_AYT  '\366' /* 246 Are you there */
#define TELNET_EC   '\367' /* 247 Erase char */
#define TELNET_EL   '\370' /* 248 Erase line */
#define TELNET_GA   '\371' /* 249 Go ahead */
#define TELNET_SB   '\372' /* 250 Subnegociation */
#define TELNET_WILL '\373' /* 251 Desire to begin performing */
#define TELNET_WONT '\374' /* 252 Refusal to perform */
#define TELNET_DO   '\375' /* 253 Request other party to perform */
#define TELNET_DONT '\376' /* 254 Request other party to stop performing */
#define TELNET_IAC  '\377' /* 255 IAC */

#define TELNET_OPT_ECHO   '\1'  /* 1 */
#define TELNET_OPT_SUPGA  '\3'  /* 3 */
#define TELNET_OPT_STATUS '\5'  /* 5 */
#define TELNET_OPT_TIMING '\6'  /* 6 */
#define TELNET_OPT_TERMT  '\30' /* 24 */
#define TELNET_OPT_WSIZE  '\37' /* 31 */
#define TELNET_OPT_TSPEED '\40' /* 32 */
#define TELNET_OPT_RFLOW  '\41' /* 33 */
#define TELNET_OPT_LMODE  '\42' /* 34 */
#define TELNET_OPT_ENVVAR '\44' /* 36 */

#define TELNET_SEND '\1'
#define TELNET_IS '\0'

static void telnet_reply(char resp_code, char opt) {
  simple_serial_putc(TELNET_IAC);
  simple_serial_putc(resp_code);
  simple_serial_putc(opt);
}

static void telnet_subneg(char opt) {
  unsigned char one = simple_serial_getc();
  unsigned char iac = simple_serial_getc();
  unsigned char sne = simple_serial_getc();
  
  switch(opt) {
    case TELNET_OPT_TERMT:
      simple_serial_putc(TELNET_IAC);
      simple_serial_putc(TELNET_SB);
      simple_serial_putc(TELNET_OPT_TERMT);
      simple_serial_putc(TELNET_IS);
      simple_serial_puts("VT100");
      simple_serial_putc(TELNET_IAC);
      simple_serial_putc(TELNET_SBE);
      break;
    case TELNET_OPT_WSIZE:
      simple_serial_putc(TELNET_IAC);
      simple_serial_putc(TELNET_SB);
      simple_serial_putc(TELNET_OPT_WSIZE);
      simple_serial_putc(0);
      simple_serial_putc(scrw);
      simple_serial_putc(0);
      simple_serial_putc(btm_line - top_line);
      simple_serial_putc(TELNET_IAC);
      simple_serial_putc(TELNET_SBE);
    case TELNET_OPT_TSPEED:
      simple_serial_putc(TELNET_IAC);
      simple_serial_putc(TELNET_SB);
      simple_serial_putc(TELNET_OPT_TSPEED);
      simple_serial_putc(TELNET_IS);
      simple_serial_puts("9600,9600");
      simple_serial_putc(TELNET_IAC);
      simple_serial_putc(TELNET_SBE);
      break;
    default:
      /* We shouldn't be there */
      break;
  }
}
static int echo = 1;

static int handle_telnet_command(void) {
  char type;
  char opt;

  /* We're here after receiving a \377 (0xFF). Now do the telnet stuff */
  type = simple_serial_getc();
  opt = simple_serial_getc();

  if (type == TELNET_DO) {
    if (opt == TELNET_OPT_TERMT 
     || opt == TELNET_OPT_WSIZE
     || opt == TELNET_OPT_TSPEED) {
      telnet_reply(TELNET_WILL, opt);
    } else {
      telnet_reply(TELNET_WONT, opt);
    }
  } else if (type == TELNET_SB) {
    telnet_subneg(opt);
    
    /* we can turn echo off and ask remote
     * to turn echo on */
    echo = 0;
#ifndef __CC65__
    static struct termios ttyf;
    tcgetattr( STDIN_FILENO, &ttyf);
    ttyf.c_lflag &= ~(ECHO);
    tcsetattr( STDIN_FILENO, TCSANOW, &ttyf);
#endif

  }

  return 0;
}

static int handle_special_char(char i) {
#ifdef __CC65__
  switch(i) {
    case CH_CURS_LEFT:
      simple_serial_puts(cursor_mode == '[' ? "\33[D":"\33OD");
      return 1;
    case CH_CURS_RIGHT:
      simple_serial_puts(cursor_mode == '[' ? "\33[C":"\33OC");
      return 1;
    case CH_CURS_UP:
      simple_serial_puts(cursor_mode == '[' ? "\33[A":"\33OA");
      return 1;
    case CH_CURS_DOWN:
      simple_serial_puts(cursor_mode == '[' ? "\33[B":"\33OB");
      return 1;
    case CH_ESC:
      simple_serial_puts("\33");
      return 1;
  }
#endif
  return 0;
}

static char curs_x = 255, curs_y = 255;
static char ch_at_curs = 0;
static int cursor_blinker = 0;

static void set_cursor(void) {
  if (curs_x == 255) {
    curs_x = wherex();
    curs_y = wherey();
    ch_at_curs = cpeekc();
    cursor_blinker = 0;
  }
  cursor_blinker++;
  if (cursor_blinker == 1) {
    gotoxy(curs_x, curs_y);
    cputc(0x7F);
  } else if (cursor_blinker == 1501){
    gotoxy(curs_x, curs_y);
    cputc(ch_at_curs);
  } else if (cursor_blinker == 3000) {
    cursor_blinker = 0;
  }
}

static void rm_cursor(void) {
  if (curs_x != 255) {
    gotoxy(curs_x, curs_y);
    cputc(ch_at_curs);
    gotoxy(curs_x, curs_y);
    curs_x = 255;
  }
}

static char screen_scroll_at_next_char = 0;
static void print_char(char o) {
#ifdef __CC65__
  unsigned char cur_x, cur_y;
  cur_x = wherex();
  cur_y = wherey();

  if (o == '\n') {
    printf("\n");
  } else if (o == '\7') {
    printf("%c", '\7'); /* bell */
  } else if (o == '\10') {
    /* handle incoming backspace */
    /* it is not the backspace that should remove
     * a char. It is the control code [33K.
     * \10 just means scrollback one pos left. */
    if (cur_x == 0) {
      cur_x = scrw - 1;
      cur_y--;
    } else {
      cur_x--;
    }
    gotoxy(cur_x, cur_y);
    /* and this is all. Char will be saved by set_cursor. */
  } else {
    if (screen_scroll_at_next_char) {
      char prev;
      /* handle scrolling */
      gotoxy(scrw - 1, btm_line - 1);
      prev = cpeekc();
      printf("\n");
      cputcxy(scrw - 1, btm_line - 2, prev);
      cur_x = 0;
      cur_y = btm_line - 1;
      screen_scroll_at_next_char = 0;
    }
    if (o != '\r' && cur_y == btm_line - 1 && cur_x == scrw - 1) {
      /* about to wrap at bottom of screen */
      screen_scroll_at_next_char = 1;
    }

    if (o != '\r')
      cputc(o);
    else {
      gotox(0);
    }
  }
#else
  fputc(o, stdout);
  fflush(stdout);
#endif
}

static int buffer_pop() {
  char o;

  if (buf_idx == 0) {
    set_cursor();
    return 0;
  }

  rm_cursor();
  cur_read_idx = 0;
  while (cur_read_idx < buf_idx) {
    o = buffer[cur_read_idx++];
    if (o == 0x04) {
      return -1;
    } else if (o == CH_ESC) {
      if (handle_vt100_escape_sequence(0) == EOF) {
        return -1;
      }
    } else {
      flush_vt100_ctrls();
      print_char(o);
    }
  }
  flush_vt100_ctrls();
  set_cursor();
  buf_idx = 0;

  return 0;
}

int main(int argc, char **argv) {
  surl_response *response = NULL;
  char i, o;
  //DEBUG int loop_wait = 0;
  
#ifdef __CC65__
  videomode(VIDEOMODE_80COL);
#endif
  screensize(&scrw, &scrh);
  get_scrollwindow(&top_line, &btm_line);

again:
  clrscr();

  /* turn echo on */
  echo = 1;
#ifndef __CC65__
  static struct termios ttyf;
  tcgetattr( STDIN_FILENO, &ttyf);
  ttyf.c_lflag |= ECHO;
  tcsetattr( STDIN_FILENO, TCSANOW, &ttyf);
#endif

  if (argc > 1) {
    buf = strdup(argv[1]);
    argc = 1; /* Only first time */
  } else {
    char t;

    buf = malloc(BUFSIZE);
    cputs("Enter host:port: ");
    cgets(buf, BUFSIZE);
    cputs("Translate \\n <=> \\r\\n (N/y)? ");
    t = cgetc();
    translate_ln = (t == 'y' || t == 'Y');
  }

#ifndef __CC65__
  tcgetattr( STDOUT_FILENO, &ttyf);
  ttyf.c_lflag &= ~(ICANON);
  tcsetattr( STDOUT_FILENO, TCSANOW, &ttyf);
#endif

  if (strchr(buf, '\n'))
    *strchr(buf, '\n') = '\0';

  cursor(1);
  response = surl_start_request("RAW", buf, NULL, 0);
  if (response == NULL || response->code != 100) {
    printf("No response (%d).\n", response ? response->code : -1);
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
          simple_serial_putc('\r');
        }
      } else if (i != '\0') {
        if (!handle_special_char(i)) {
#ifdef __CC65__
          ser_put(i);
#else
          simple_serial_putc(i);
#endif
        }
      }
#ifdef __CC65__
      /* Echo */
      if (echo) {
        rm_cursor();
        if (i == '\r') {
          printf("\n");
        } else {
          cputc(i);
        }
        set_cursor();
      }
#else
      if (echo) {
        fputc(i, stdout);
      }
#endif
    }

/* Make sure to stop reading far enough 
 * from the buffer end, because vt100 controls
 * can take a few bytes. */
#ifdef __CC65__
    while (ser_get(&o) != SER_ERR_NO_DATA && buf_idx < 1000) {
#else
    int r;
    while ((r = simple_serial_getc_immediate()) != EOF && buf_idx < 1000) {
      o = (char)r;
#endif
      if (o == TELNET_IAC) {
        if (handle_telnet_command() == EOF) {
          printf("Telnet error\n");
          goto remote_closed;
        }
      } else if (o != '\0' && o != '\16' && o != '\17') {
        buffer[buf_idx++] = o;
        if (o == CH_ESC) {
          if (handle_vt100_escape_sequence(1) == EOF) {
            printf("vt100 error\n");
            goto remote_closed;
          }
          //DEBUG loop_wait = 800;
        }
        if (o == '\n') {
          /* flush on \n */
          break;
        }
      }
    }

    /* Fixme should be a ring buffer */
    //DEBUG if (loop_wait == 0 || buf_idx >= 1000) {
      if (buffer_pop() < 0) {
        goto remote_closed;
      }
    // }
    // if (loop_wait > 0)
    //   loop_wait--;

  } while(i != 0x04);

remote_closed:
  top_line = 0;
  btm_line = 24;
  set_scrollwindow(0, scrh);

  printf("\nConnection closed (Enter to continue).\n");
  cgetc();
  surl_response_free(response);
  free(buf);
  cursor_mode = CURS_MODE_CURSOR;
  curs_x = 255;
  curs_y = 255;
  buf_idx = 0;

#ifndef __CC65__
  tcgetattr( STDOUT_FILENO, &ttyf);
  ttyf.c_lflag |= ICANON;
  tcsetattr( STDOUT_FILENO, TCSANOW, &ttyf);
#endif
  goto again;
  
  exit(0);
}
