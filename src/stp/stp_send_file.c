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
#include "path_helper.h"
#include "file_select.h"
#include "stp.h"
#include "stp_cli.h"
#include "stp_send_file.h"
#include "simple_serial.h"
#include "dgets.h"
#include "clrzone.h"
#include "extended_conio.h"
#include "progress_bar.h"
#include "get_filedetails.h"
#include "get_buf_size.h"
#include "math.h"

#define BUFSIZE 255

static unsigned char scrw = 255, scrh = 255;

static char *stp_send_dialog() {
  char *filename = file_select(0, 2, scrw - 1, 2 + PAGE_HEIGHT, 0, "Select file to send");
  clrzone(0, 2, scrw - 1, 2 + PAGE_HEIGHT);
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
  static char *path;
  static char *remote_filename;
  static surl_response *resp;
  static int r = 0;
  
  fp = NULL;
  filename = NULL;
  path = NULL;
  remote_filename = NULL;
  resp = NULL;
  r = 0;
  
  if (scrw == 255)
    screensize(&scrw, &scrh);

  path  = stp_send_dialog();
  if (path == NULL) {
    return;
  }

#ifdef PRODOS_T_TXT
  /* We want to send raw files */
  _filetype = PRODOS_T_BIN;
  _auxtype  = PRODOS_AUX_T_TXT_SEQ;
#endif

  clrzone(0, 2, scrw - 1, 2 + PAGE_HEIGHT);
  gotoxy(0, 2);
  printf("Opening %s...\n", path);

  fp = fopen(path, "r");
  if (fp == NULL) {
    printf("%s: %s", path, strerror(errno));
    cgetc();
    goto err_out;
  }
  
  if (get_filedetails(path, &filename, &filesize, &type, &auxtype) < 0) {
    printf("Can't get file details.");
    cgetc();
    goto err_out;
  }

#ifdef PRODOS_T_TXT
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
    printf("Cannot allocate buffer.");
    cgetc();
    goto err_out;
  }

  progress_bar(0, 5, scrw, 0, filesize);

  resp = surl_start_request(SURL_METHOD_PUT, remote_filename, NULL, 0);
  if (resp == NULL || resp->code != 100) {
    printf("Bad response.");
    cgetc();
    goto err_out;
  }

  if (surl_send_data_params(filesize, SURL_DATA_X_WWW_FORM_URLENCODED_RAW) != 0) {
    goto finished;
  }

  total = 0;

  do {
    size_t rem = (size_t)((long)filesize - (long)total);
    size_t chunksize = min(buf_size, rem);
    clrzone(0, 2, scrw - 1, 2);
    gotoxy(0, 2);
    printf("Reading %zu bytes...", chunksize);

    r = fread(data, sizeof(char), chunksize, fp);
    total = total + r;
    
    clrzone(0, 2, scrw - 1, 2);
    gotoxy(0, 2);
    printf("Sending %zu/%lu...", total, filesize);
    surl_send_data(data, r);

    progress_bar(0, 5, scrw, total, filesize);
  } while (total < filesize);
  clrzone(0, 2, scrw - 1, 2 + PAGE_HEIGHT);
  gotoxy(0, 2);
  printf("Sent %zu/%lu.\n", total, filesize);

finished:
  surl_read_response_header(resp);
  printf("File sent, response code: %d\n", resp->code);
  printf("Hit a key to continue.");
  cgetc();

err_out:
  if (fp)
    fclose(fp);
  // 
  // while (reopen_start_device() != 0) {
  //   clrzone(0, 21, scrw - 1, 22);
  //   gotoxy(0, 21);
  //   printf("Please reinsert the program disk.");
  //   cgetc();
  // }

  free(path);
  free(remote_filename);
  free(data);
  surl_response_free(resp);
  clrzone(0, 19, scrw - 1, 20);
  stp_print_footer();
}
