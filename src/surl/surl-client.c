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
#include "cgets.h"

#define BUFSIZE 255
static char *buf;

int main(int argc, char **argv) {
  const surl_response *response = NULL;
  char *headers[1] = {"Accept: text/*"};
  char *buffer;
  size_t r;

again:
  if (argc > 1) {
    buf = strdup(argv[1]);
  } else {
    buf = malloc(BUFSIZE);
    printf("Enter URL: ");
    cgets(buf, BUFSIZE);
  }

  if (strchr(buf, '\n'))
    *strchr(buf, '\n') = '\0';

  response = surl_start_request(SURL_METHOD_GET, buf, headers, 1);

  printf("Got response %d (%zu bytes), %s\n", response->code, response->size, response->content_type);

  surl_strip_html(SURL_HTMLSTRIP_FULL);
  printf("Stripped: %d (%zu bytes), %s\n", response->code, response->size, response->content_type);
  
  surl_translit("ISO646-FR1");
  printf("Stripped: %d (%zu bytes), %s\n", response->code, response->size, response->content_type);
  
  buffer = malloc(BUFSIZE);
  while ((r = surl_receive_data(buffer, BUFSIZE - 1)) > 0) {
    printf("%s", buffer);
  }
  
  printf("done\n");
  
  free(buffer);
  free(buf);

  if (argc == 1) {
    goto again;
  }
  
  exit(0);
}
