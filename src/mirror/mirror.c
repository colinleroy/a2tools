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
#include "simple_serial.h"
#include <string.h>
#include <dbg.h>
#include <errno.h>
#include <apple2enh.h>
#include <conio.h>

#define BUF_SIZE 255

int main(void) {
  int exit_code = 0;
  char *buf = malloc(BUF_SIZE);

  printf("Opening serial port\n");
  simple_serial_open(2, SER_BAUD_9600);

  clrscr();
  gotoxy(0,0);
  simple_serial_set_timeout(10);
  while (1) {
    printf("> ");
    if (simple_serial_gets_with_timeout(buf, BUF_SIZE) != NULL) {

      printf("%s", buf);

      simple_serial_puts(buf);
    }
  }

  simple_serial_close();
  exit(exit_code);
}
