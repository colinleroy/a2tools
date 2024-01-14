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
#include "platform.h"
#include "simple_serial.h"
#include "extended_conio.h"

extern unsigned char baudrate;
extern unsigned char flow_control;
#ifdef __CC65__
  #pragma static-locals(push, on)
  #pragma code-name (push, "LOWCODE")
  extern unsigned char open_slot;
#else
  extern FILE *ttyfp;
#endif

#ifdef __CC65__

void simple_serial_set_speed(int b) {
#ifndef IIGS
  static unsigned char reg_idx;
  /* Set speed before port is opened */
  baudrate = (unsigned char)b;
  /* Set speed after port is opened */
  reg_idx = open_slot << 4;

  switch (b) {
    case SER_BAUD_9600:
      __asm__("ldx     %v", reg_idx);
      __asm__("lda     $c08b,x");
      __asm__("and     #%b", (unsigned char)0b11110000);
      __asm__("ora     #%b", (unsigned char)0b00001110);
      __asm__("sta     $c08b,x");
      break;
    case SER_BAUD_19200:
      __asm__("ldx     %v", reg_idx);
      __asm__("lda     $c08b,x");
      __asm__("and     #%b", (unsigned char)0b11110000);
      __asm__("ora     #%b", (unsigned char)0b00001111);
      __asm__("sta     $c08b,x");
      break;
    default:
      break;
  }
#else
  /* Set speed before port is opened */
  baudrate = (unsigned char)b;
  /* Set speed after port is opened */
  switch (b) {
    case SER_BAUD_9600:
    __asm__("ldx     #12"); /* BaudLow */
    __asm__("lda     #$0A");
    __asm__("stx     $c038");
    __asm__("sta     $c038");
      break;
    case SER_BAUD_19200:
    __asm__("ldx     #12"); /* BaudLow */
    __asm__("lda     #$04");
    __asm__("stx     $c038");
    __asm__("sta     $c038");
      break;
    case SER_BAUD_57600:
    __asm__("ldx     #12"); /* BaudLow */
    __asm__("lda     #$00");
    __asm__("stx     $c038");
    __asm__("sta     $c038");
      break;
    default:
      break;
  }
  __asm__("ldx     #13"); /* BaudHigh */
  __asm__("lda     #$00"); /* It's 0 for 9.6, 19.2 and 57.6 kbps. */
  __asm__("stx     $c038");
  __asm__("sta     $c038");

#endif
}

void simple_serial_set_flow_control(unsigned char fc) {
  /* Only done before opening */
  flow_control = fc;
}

void simple_serial_dtr_onoff(unsigned char on) {
#ifndef IIGS
  simple_serial_acia_onoff(open_slot, on);
#else
  /* http://www.applelogic.org/files/Z8530UM.pdf */
  if (on) {
    __asm__("ldx     #5");
    __asm__("lda     #%b", (unsigned char)0b01101010);
    __asm__("stx     $c038");
    __asm__("sta     $c038");
  } else {
    __asm__("ldx     #5");
    __asm__("lda     #%b", (unsigned char)0b11101010);
    __asm__("stx     $c038");
    __asm__("sta     $c038");
  }
#endif
}

#ifndef IIGS
void simple_serial_acia_onoff(unsigned char slot_num, unsigned char on) {
  static unsigned char reg_idx;

  reg_idx = slot_num << 4;

  if (on) {
    __asm__("ldx     %v", reg_idx);
    __asm__("lda     $c08a,x");
    __asm__("and     #%b", (unsigned char)0b11110000);
    __asm__("ora     #%b", (unsigned char)0b00001011);
    __asm__("sta     $c08a,x");
  } else {
    __asm__("ldx     %v", reg_idx);
    __asm__("lda     $c08a,x");
    __asm__("and     #%b", (unsigned char)0b11110000);
    __asm__("sta     $c08a,x");
  }
}
#endif

/* https://www.princeton.edu/~mae412/HANDOUTS/Datasheets/6551_acia.pdf */
void simple_serial_set_parity(unsigned int p) {
#ifndef IIGS
  static unsigned char reg_idx;

  reg_idx = open_slot << 4;

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
#else
  /* Fixme this assumes 8 databits (01000000) and 1 stop bit (00000100) */
  switch (p) {
    case SER_PAR_NONE:
      __asm__("ldx     #4");
      __asm__("lda     #%b", (unsigned char)0b01000100);
      __asm__("stx     $c038");
      __asm__("sta     $c038");
      break;
    case SER_PAR_EVEN:
      __asm__("ldx     #4");
      __asm__("lda     #%b", (unsigned char)0b01000111);
      __asm__("stx     $c038");
      __asm__("sta     $c038");
      break;
    case SER_PAR_ODD:
      __asm__("ldx     #4");
      __asm__("lda     #%b", (unsigned char)0b01000101);
      __asm__("stx     $c038");
      __asm__("sta     $c038");
      break;
    case SER_PAR_MARK:
      break;
    case SER_PAR_SPACE:
      break;
    default:
      break;
  }
#endif
}

#else

void simple_serial_set_speed(int b) {
  struct termios tty;
  char *spd_str = tty_speed_to_str(b);

  /* Set speed before the port is opened */
  setenv("A2_TTY_SPEED", spd_str, 1);
  if (ttyfp == NULL) {
    return;
  }

  #ifdef TCFLSH
  ioctl(fileno(ttyfp), TCFLSH, TCIOFLUSH);
  #endif

  /* Set speed after the port is opened */
  if(tcgetattr(fileno(ttyfp), &tty) != 0) {
    printf("tcgetattr error\n");
    exit(1);
  }

  cfsetispeed(&tty, b);
  cfsetospeed(&tty, b);
  tcsetattr(fileno(ttyfp), TCSANOW, &tty);
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
