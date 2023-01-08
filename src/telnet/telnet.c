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

#define BUFSIZE 255
static char *buf;
static int translate_ln = 0;

int main(int argc, char **argv) {
  surl_response *response = NULL;
  char *buffer = NULL;
  size_t r, i, o;

#ifdef __CC65__
  videomode(VIDEOMODE_80COL);
  clrscr();
#endif

#ifndef __CC65__
  static struct termios ttyf;

  tcgetattr( STDOUT_FILENO, &ttyf);
  ttyf.c_lflag &= ~(ICANON);
  tcsetattr( STDOUT_FILENO, TCSANOW, &ttyf);
#endif

again:
  if (argc > 1) {
    buf = strdup(argv[1]);
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
        simple_serial_putc(i);
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
    while ((o = simple_serial_getc_immediate()) != EOF && o != '\0') {
      if (o == '\r' && translate_ln)
        continue;
      if (o == 0x04) {
        goto remote_closed;
      }
      fputc(o, stdout);
      fflush(stdout);
    }
  } while(i != 0x04);

remote_closed:
  printf("\ndone\n");
  surl_response_free(response);
  free(buffer);
  free(buf);

  if (argc == 1) {
    goto again;
  }
  
  exit(0);
}
