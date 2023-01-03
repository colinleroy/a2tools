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
#include "http.h"
#include "simple_serial.h"
#include "extended_conio.h"

#define BUFSIZE 255
static char buf[BUFSIZE];

int main(int argc, char **argv) {
  http_response *response = NULL;
  const char *headers[1] = {"Accept: text/*"};
  char *buffer;
  size_t r;
  http_connect_proxy();

again:
  printf("Enter URL: ");
  cgets(buf, BUFSIZE);
  if (*strchr(buf, '\n'))
    *strchr(buf, '\n') = '\0';

  response = http_start_request("GET", buf, headers, 1);

  printf("Got response %d (%zu bytes)\n", response->code, response->size);
  buffer = malloc(BUFSIZE);
  while ((r = http_receive_lines(response, buffer, BUFSIZE - 1)) > 0) {
    printf("%s", buffer);
  }
  
  printf("done\n");
  
  http_response_free(response);

  goto again;
  
  exit(0);
}
