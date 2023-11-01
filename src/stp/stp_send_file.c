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
#include "platform.h"
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

extern unsigned char scrw, scrh;

static char *stp_send_dialog() {
  char *filename;
  clrzone(0, 2, scrw - 1, 2 + PAGE_HEIGHT);
  gotoxy(0, 3);
  cprintf("File name: ");
  filename = file_select(wherex(), wherey(), scrw - 1, PAGE_HEIGHT, 0, "Select file to send");
  return filename;
}

static unsigned long filesize = 0;
static unsigned long total = 0;
static unsigned char type;
static unsigned auxtype;
static int buf_size;
static char *data = NULL;

void stp_send_file(char *remote_dir) {
  static FILE *fp;
  static char *filename;
  static char *path;
  static char *remote_filename;
  static const surl_response *resp;
  static int r = 0;
  static char start_y = 3;

  fp = NULL;
  filename = NULL;
  path = NULL;
  remote_filename = NULL;
  r = 0;

  path = stp_send_dialog();
  if (path == NULL) {
    return;
  }

#ifdef __APPLE2ENH__
  /* We want to send raw files */
  _filetype = PRODOS_T_BIN;
  _auxtype  = PRODOS_AUX_T_TXT_SEQ;
#endif

  clrzone(0, 2, scrw - 1, 2 + PAGE_HEIGHT);
  gotoxy(0, 3);
  cprintf("Opening %s...", path);

  fp = fopen(path, "r");
  if (fp == NULL) {
    cprintf("%s: %s", path, strerror(errno));
    cgetc();
    goto err_out;
  }

  if (get_filedetails(path, &filename, &filesize, &type, &auxtype) < 0) {
    cprintf("Can't get file details.");
    cgetc();
    goto err_out;
  }
  cprintf("(%lu bytes)\r\n", filesize);

#ifdef __APPLE2ENH__
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
    cprintf("Cannot allocate buffer.");
    cgetc();
    goto err_out;
  }

  progress_bar(0, start_y + 3, scrw, 0, filesize);

  resp = surl_start_request(SURL_METHOD_PUT, remote_filename, NULL, 0);
  if (resp->code != 100) {
    cprintf("Bad response.");
    cgetc();
    goto err_out;
  }

  if (surl_send_data_params(filesize, SURL_DATA_X_WWW_FORM_URLENCODED_RAW) != 0) {
    goto finished;
  }

  total = 0;

  do {
    unsigned long rem = filesize - total;
    size_t chunksize = buf_size;

    if (rem < (unsigned long)chunksize)
      chunksize = (size_t)rem;

    clrzone(0, start_y, scrw - 1, start_y);
    gotoxy(0, start_y);
    cprintf("Sending %s: %lu/%lu bytes...", filename, total, filesize);

    r = fread(data, sizeof(char), chunksize, fp);
    progress_bar(0, start_y + 3, scrw, total + (chunksize / 2), filesize);

    total = total + r;

    surl_send_data(data, r);

    progress_bar(-1, -1, scrw, total, filesize);
  } while (total < filesize);

finished:
  surl_read_response_header();
  clrzone(0, 2, scrw - 1, 2 + PAGE_HEIGHT);
  gotoxy(0, start_y);
  cprintf("Sent %lu bytes.\r\n", total);
  cprintf("File %s sent, response code: %d\r\n", filename, resp->code);
  cprintf("Hit a key to continue.");
  cgetc();

err_out:
  if (fp)
    fclose(fp);

  free(path);
  free(remote_filename);
  free(data);
  clrzone(0, 19, scrw - 1, 20);
  stp_print_footer();
}
