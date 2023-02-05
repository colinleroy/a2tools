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
#include <sys/stat.h>
#include <dirent.h>
#ifndef __CC65__
#include <libgen.h>
#endif
#include "cgets.h"
#include "surl.h"
#include "get_filedetails.h"
#include "simple_serial.h"
#include "extended_conio.h"

#define BUFSIZE 255
static char *buf;
static char *filename;

int main(int argc, char **argv) {
  surl_response *response = NULL;
  char *headers[1] = {"Accept: text/*"};
  char *buffer;
  size_t r;
  unsigned long filesize;
  unsigned char type;
  unsigned auxtype;
  FILE *fp;

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

  if (argc > 2) {
    filename = strdup(argv[2]);
  } else {
    filename = malloc(BUFSIZE);
    printf("Enter file to send: ");
    cgets(filename, BUFSIZE);
  }

  if (strchr(filename, '\n'))
    *strchr(filename, '\n') = '\0';
    
  if (get_filedetails(filename, &filesize, &type, &auxtype) < 0) {
    printf("Cannot get file details.\n");
    exit(1);
  }

  fp = fopen(filename,"r");
  if (fp == NULL) {
    printf("Can't open %s\n", filename);
    exit(1);
  }

  response = surl_start_request(SURL_METHOD_PUT, buf, headers, 1);
  if (response == NULL) {
    printf("No response.\n");
    exit(1);
  }

  if (response->code != 100) {
    printf("Unexpected response code %d\n", response->code);
    exit(1);
  }

  buffer = malloc(BUFSIZE);
  surl_send_data_params(filesize, 1);
  while ((r = fread(buffer, 1, BUFSIZE, fp)) > 0) {
    surl_send_data(buffer, r);
  }

  fclose(fp);

  surl_read_response_header(response);

  printf("Got response %d (%zu bytes), %s\n", response->code, response->size, response->content_type);

  while ((r = surl_receive_data(response, buffer, BUFSIZE - 1)) > 0) {
    printf("%s", buffer);
  }
  
  printf("done\n");
  
  surl_response_free(response);
  free(buffer);
  free(buf);

  if (argc == 1) {
    goto again;
  }
  
  exit(0);
}
