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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "stp_cli.h"
#include "stp_send_file.h"
#include "simple_serial.h"
#include "cgets.h"
#include "clrzone.h"
#include "extended_conio.h"
#include "get_filedetails.h"
#include "get_buf_size.h"
#include "math.h"

#define BUFSIZE 255

static unsigned char scrw = 255, scrh = 255;

static char *stp_send_dialog() {
  char *filename = malloc(BUFSIZE);

  if (scrw == 255)
    screensize(&scrw, &scrh);

  clrzone(0, 19, scrw - 1, 23);
  gotoxy(0, 19);

  printf("Enter file to send, or empty to abort: ");
  gotoxy(0, 20);
  cgets(filename, BUFSIZE);
  
  if (strchr(filename, '\n'))
    *strchr(filename, '\n') = '\0';
  
  if (*filename == '\0') {
    free(filename);
    stp_print_footer();
    return NULL;
  }

  return filename;
}

static unsigned long filesize = 0;
static size_t total = 0;
static unsigned char type;
static unsigned auxtype;
static int buf_size;
static char *data = NULL;

void stp_send_file(char *remote_dir) {
  static FILE *fp;
  static char *filename;
  static char *remote_filename;
  static surl_response *resp;
  static int r = 0;
  
  fp = NULL;
  filename = NULL;
  remote_filename = NULL;
  resp = NULL;
  r = 0;
  
  if (scrw == 255)
    screensize(&scrw, &scrh);

  filename  = stp_send_dialog();
  if (filename == NULL) {
    return;
  }

  fp = fopen(filename, "r");
  if (fp == NULL) {
    gotoxy(0, 21);
    printf("%s: %s", filename, strerror(errno));
    cgetc();
    goto err_out;
  }
  
  if (get_filedetails(filename, &filesize, &type, &auxtype) < 0) {
    gotoxy(0, 21);
    printf("Can't get file details.");
    cgetc();
    goto err_out;
  }

#ifdef PRODOS_T_TXT
  /* We want to send raw files */
  _filetype = PRODOS_T_TXT;
  _auxtype  = PRODOS_AUX_T_TXT_SEQ;
  remote_filename = malloc(BUFSIZE);
  if (type == PRODOS_T_SYS) {
    snprintf(remote_filename, BUFSIZE, "%s/%s.SYS", remote_dir, filename);
  } else if (type == PRODOS_T_BIN) {
    snprintf(remote_filename, BUFSIZE, "%s/%s.BIN", remote_dir, filename);
  } else {
    snprintf(remote_filename, BUFSIZE, "%s/%s.TXT", remote_dir, filename);
  }
#else
    remote_filename = malloc(BUFSIZE);
    snprintf(remote_filename, BUFSIZE, "%s/%s", remote_dir, filename);
#endif

  buf_size = get_buf_size();
  data = malloc(buf_size + 1);

  if (data == NULL) {
    gotoxy(0, 21);
    printf("Cannot allocate buffer.");
    cgetc();
    goto err_out;
  }

  progress_bar(0, 22, scrw, 0, filesize);

  resp = surl_start_request("PUT", remote_filename, NULL, 0);
  if (resp == NULL || resp->code != 100) {
    gotoxy(0, 21);
    printf("Bad response.");
    goto err_out;
  }

  if (surl_send_data_params(resp, filesize, 1) != 0) {
    goto finished;
  }

  total = 0;

  do {
    size_t rem = (size_t)((long)filesize - (long)total);
    size_t chunksize = min(buf_size, rem);
    clrzone(0, 21, scrw - 1, 21);
    gotoxy(0, 21);
    printf("Reading %zu bytes...", chunksize);

    r = fread(data, sizeof(char), chunksize, fp);
    total = total + r;
    
    simple_serial_set_activity_indicator(1, 39, 0);
    clrzone(0, 21, scrw - 1, 21);
    gotoxy(0, 21);
    printf("Sending %zu/%lu...", total, filesize);
    r = surl_send_data(resp, data, r);
    simple_serial_set_activity_indicator(0, 0, 0);

    progress_bar(0, 22, scrw, total, filesize);
  } while (total < filesize);
  gotoxy(0, 21);
  clrzone(0, 21, scrw - 1, 21);
  printf("Sent %zu/%lu.", total, filesize);

finished:
  surl_read_response_header(resp);
  clrzone(0, 21, scrw - 1, 22);
  gotoxy(0, 21);
  printf("File sent, response code: %d\n", resp->code);
  printf("Hit a key to continue.");
  cgetc();

err_out:
  if (fp)
    fclose(fp);
  free(filename);
  free(remote_filename);
  free(data);
  surl_response_free(resp);
  clrzone(0, 19, scrw - 1, 20);
  stp_print_footer();
}
