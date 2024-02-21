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
#include <sys/ioctl.h>
#include "malloc0.h"
#include "platform.h"
#include "simple_serial.h"
#include "extended_conio.h"

extern unsigned char baudrate;
extern unsigned char flow_control;
extern FILE *ttyfp;

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
  if (ttyfp == NULL) {
    return;
  }
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
