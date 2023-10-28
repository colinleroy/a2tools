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

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#ifndef __CC65__
#include <sys/ioctl.h>
#else
#include <peekpoke.h>
#endif
#include "malloc0.h"
#include "simple_serial.h"
#include "extended_conio.h"
#include "extended_string.h"

extern unsigned char baudrate;
extern unsigned char flow_control;
#ifdef __CC65__
  #pragma static-locals(push, on)
  #pragma code-name (push, "LOWCODE")
  extern unsigned char slot;
#else
  extern FILE *ttyfp;
#endif

char * __fastcall__ simple_serial_gets(char *out, size_t size) {
  static char c;
  static char *cur;
  static char *end;

  if (size == 0) {
    return NULL;
  }

  cur = out;
  end = cur + size - 1;
  while (cur < end) {
#ifdef __CC65__
    while (ser_get(&c) == SER_ERR_NO_DATA);
#else
    c = simple_serial_getc();
#endif
    
    if (c == '\r') {
      /* ignore \r */
      continue;
    }

    *cur = c;
    ++cur;

    if (c == '\n') {
      break;
    }
  }
  *cur = '\0';

  return out;
}

char *simple_serial_buf = NULL;
void simple_serial_printf(const char* format, ...) {
  va_list args;

  if (simple_serial_buf == NULL)
    simple_serial_buf = malloc0(SIMPLE_SERIAL_BUF_SIZE);

  va_start(args, format);
  vsnprintf(simple_serial_buf, SIMPLE_SERIAL_BUF_SIZE - 1, format, args);
  va_end(args);

  simple_serial_puts(simple_serial_buf);
}

#ifdef __CC65__
void simple_serial_set_speed(unsigned char b) {
  baudrate = b;
}
void simple_serial_set_flow_control(unsigned char fc) {
  flow_control = fc;
}

void simple_serial_dtr_onoff(unsigned char on) {
#ifdef IIGS
  /* TODO */
#endif
}

void simple_serial_acia_onoff(unsigned char slot_num, unsigned char on) {
#ifndef IIGS
  static unsigned char reg_idx;
  
  reg_idx = slot_num << 4;

  if (on) {
    __asm__("ldx     %v", reg_idx);
    __asm__("lda     #%b", (unsigned char)0b00001011);
    __asm__("sta     $c08a,x");
  } else {
    __asm__("ldx     %v", reg_idx);
    __asm__("lda     #%b", (unsigned char)0b00000000);
    __asm__("sta     $c08a,x");
  }
#endif
}

/* https://www.princeton.edu/~mae412/HANDOUTS/Datasheets/6551_acia.pdf */
void simple_serial_set_parity(unsigned int p) {
#ifndef IIGS
  static unsigned char reg_idx;
  
  reg_idx = slot << 4;

  switch (p) {
    case SER_PAR_NONE:
      __asm__("ldx     %v", reg_idx);
      __asm__("lda     $c08a,x");
      __asm__("and     #%b", (unsigned char)0b00011111);
      __asm__("sta     $c08a,x");
      break;
    case SER_PAR_EVEN:
      __asm__("ldx     %v", reg_idx);
      __asm__("lda     $c08a,x");
      __asm__("and     #%b", (unsigned char)0b00011111);
      __asm__("ora     #%b", (unsigned char)0b01100000);
      __asm__("sta     $c08a,x");
      break;
    case SER_PAR_ODD:
      __asm__("ldx     %v", reg_idx);
      __asm__("lda     $c08a,x");
      __asm__("and     #%b", (unsigned char)0b00011111);
      __asm__("ora     #%b", (unsigned char)0b00100000);
      __asm__("sta     $c08a,x");
      break;
    case SER_PAR_MARK:
      __asm__("ldx     %v", reg_idx);
      __asm__("lda     $c08a,x");
      __asm__("and     #%b", (unsigned char)0b00011111);
      __asm__("ora     #%b", (unsigned char)0b10100000);
      __asm__("sta     $c08a,x");
      break;
    case SER_PAR_SPACE:
      __asm__("ldx     %v", reg_idx);
      __asm__("lda     $c08a,x");
      __asm__("and     #%b", (unsigned char)0b00011111);
      __asm__("ora     #%b", (unsigned char)0b11100000);
      __asm__("sta     $c08a,x");
      break;
    default:
      break;
  }
#endif
}

#else
void simple_serial_set_speed(unsigned char b) {
  struct termios tty;

  setenv("A2_TTY_SPEED", "9600", 1);
  if (ttyfp == NULL) {
    return;
  }
  if(tcgetattr(fileno(ttyfp), &tty) != 0) {
    printf("tcgetattr error\n");
    exit(1);
  }
  cfsetispeed(&tty, b);
  cfsetospeed(&tty, b);
}

void simple_serial_dtr_onoff(unsigned char on) {
  int b;

  if (on) {
    b = TIOCM_DTR;
    ioctl(fileno(ttyfp), TIOCMBIS, &b);
  } else {
    b = TIOCM_DTR;
    ioctl(fileno(ttyfp), TIOCMBIC, &b);
  }
}


void simple_serial_set_parity(unsigned int p) {
  struct termios tty;

  if(tcgetattr(fileno(ttyfp), &tty) != 0) {
    printf("tcgetattr error\n");
    exit(1);
  }
  if (p != 0) {
    tty.c_iflag |= INPCK;
    tty.c_cflag |= p;
  } else {
    tty.c_iflag &= ~INPCK;
    tty.c_cflag &= ~PARENB;
  }

  if (tcsetattr(fileno(ttyfp), TCSANOW, &tty) != 0) {
    printf("tcgetattr error\n");
    exit(1);
  }
  tcgetattr(fileno(ttyfp), &tty);
}
#endif

#ifdef __CC65__
#pragma static-locals(pop)
#endif
