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
#include <errno.h>
#include <unistd.h>
#include "stp_list.h"
#include "stp_save.h"
#include "get_buf_size.h"
#include "simple_serial.h"
#include "clrzone.h"
#include "extended_conio.h"
#include "progress_bar.h"
#include "math.h"
#include "file_select.h"

#define APPLESINGLE_HEADER_LEN 58

extern unsigned char scrw, scrh;

char *stp_confirm_save_all(void) {
  char *out_dir;

  clrzone(0, 2, scrw - 1, 2 + PAGE_HEIGHT);
  gotoxy(0, 4);
  cprintf("Save all files in current directory\r\n\r\n");

  cprintf("Save to: ");
  out_dir = file_select(wherex(), wherey(), scrw - 1, 17, 1, "Select destination directory");
  return out_dir;
}

char *cleanup_filename(char *in) {
  int len = strlen(in), i;
  if (len > 15) {
    in[16] = '\0';
    len = 15;
  }
#ifdef __CC65__
  in = strupper(in);
#endif
  for (i = 0; i < len; i++) {
    if (in[i] >='A' && in[i] <= 'Z')
      continue;
    if (in[i] >= '0' && in[i] <= '9')
      continue;
    if (in[i] == '.')
      continue;

    in[i] = '.';
  }
  return in;
}

int stp_save_dialog(char *url, const surl_response *resp, char *out_dir) {
  char *filename = strdup(strrchr(url, '/') + 1);
  int r;
  char free_out_dir = (out_dir == NULL);

  clrzone(0, 2, scrw - 1, 2 + PAGE_HEIGHT);
  gotoxy(0, 4);
  cprintf("%s\r\n"
          "%s, %lu bytes\r\n"
          "\r\n"
          "Save to: ", filename, resp->content_type ? resp->content_type : "", resp->size);

  if (!out_dir) {
    out_dir = file_select(wherex(), wherey(), scrw - 1, 17, 1, "Select destination directory");

    if (!out_dir) {
      free(filename);
      return 1;
    }
  } else {
    cprintf("%s", out_dir);
  }

  gotoxy(0, 12);
  cprintf("Saving file...              ");
  r = stp_save(filename, out_dir, resp);

  free(filename);
  if (free_out_dir)
    free(out_dir);
  return r;
}

int stp_save(char *full_filename, char *out_dir, const surl_response *resp) {
  FILE *fp = NULL;
  char *data = NULL;
  char *filename;
#ifdef __APPLE2ENH__
  char *filetype;
#endif
  size_t r = 0;
  uint32 total = 0;
  unsigned int buf_size;
  char keep_bin_header = 0;
  char had_error = 0;
  char *full_path = NULL;
  char start_y = 10;

  filename = strdup(full_filename);

  clrzone(0, start_y, scrw - 1, start_y);
  gotoxy(0, start_y);

#ifdef __APPLE2ENH__
  if (strchr(filename, '.') != NULL) {
    filetype = strrchr(filename, '.') + 1;
  } else {
    filetype = "TXT";
  }

  if (!strcasecmp(filetype, "TXT")) {
    _filetype = PRODOS_T_TXT;
    _auxtype  = PRODOS_AUX_T_TXT_SEQ;
  } else if (!strcasecmp(filetype,"HGR")) {
    _filetype = PRODOS_T_BIN;
  } else if (!strcasecmp(filetype,"SYSTEM")) {
    _filetype = PRODOS_T_SYS;
    _auxtype = 0x2000;
  } else if (!strcasecmp(filetype,"BIN")) {
    _filetype = PRODOS_T_BIN;
    *(strrchr(filename, '.')) = '\0';

    /* Look into the header, and skip it by the way */
    data = malloc(APPLESINGLE_HEADER_LEN);
    r = surl_receive_data(data, APPLESINGLE_HEADER_LEN);

    if (r == APPLESINGLE_HEADER_LEN
     && data[0] == 0x00 && data[1] == 0x05
     && data[2] == 0x16 && data[3] == 0x00) {
      cprintf("AppleSingle: $%04x\r\n", (data[56]<<8 | data[57]));
      _auxtype = (data[56]<<8 | data[57]);
      free(data);
    } else {
      keep_bin_header = 1;
    }
    total = r;

  } else if (!strcasecmp(filetype,"SYS") || !strcasecmp(filetype,"SYSTEM")) {
    char *tmp = malloc(255);
    sprintf(tmp, "%s.system", filename);
    free(filename);
    filename = tmp;
    _filetype = PRODOS_T_SYS;
  } else {
    cprintf("Filetype unknown, using BIN.");
    free(filename);
    filename = strdup(full_filename);
    _filetype = PRODOS_T_BIN;
  }

  filename = cleanup_filename(filename);
#endif

  /* Unlink before writing (a change in AppleSingle header
   * would not be reflected, as it's written only at CREATE)
   */
  full_path = malloc(FILENAME_MAX);
  snprintf(full_path, FILENAME_MAX, "%s/%s", out_dir, filename);
  unlink(full_path);
  fp = fopen(full_path, "w");
  if (fp == NULL) {
    gotoxy(0, 15);
    cprintf("%s: %s", full_path, strerror(errno));
    cgetc();
    had_error = 1;
    goto err_out;
  }

  /* coverity[dead_error_condition] */
  if (keep_bin_header) {
    if (fwrite(data, sizeof(char), APPLESINGLE_HEADER_LEN, fp) < APPLESINGLE_HEADER_LEN) {
      gotoxy(0, 15);
      cprintf("%s.", strerror(errno));
      cgetc();
      had_error = 1;
      goto err_out;
    }
    free(data);
  }

  buf_size = get_buf_size();
  data = malloc(buf_size + 1);

  if (data == NULL) {
    gotoxy(0, 15);
    cprintf("Cannot allocate buffer.");
    cgetc();
    had_error = 1;
    goto err_out;
  }

  progress_bar(0, 15, scrw - 1, 0, resp->size);
  do {
    size_t bytes_to_read = buf_size;
    if (resp->size - total < (uint32)buf_size)
      bytes_to_read = (uint16)(resp->size - total);

    clrzone(0,14, scrw - 1, 14);
    gotoxy(0, 14);
    cprintf("Reading %zu bytes...", bytes_to_read);

    r = surl_receive_data(data, bytes_to_read);

    progress_bar(0, 15, scrw - 1, total + (bytes_to_read / 2), resp->size);
    clrzone(0,14, scrw - 1, 14);
    gotoxy(0, 14);
    total += r;
    cprintf("Saving %lu/%lu bytes...", total, resp->size);

    if (fwrite(data, sizeof(char), r, fp) < r) {
      gotoxy(0, 15);
      cprintf("%s.", strerror(errno));
      cgetc();
      had_error = 1;
      goto err_out;
    }

    progress_bar(0, 15, scrw - 1, total, resp->size);
  } while (r > 0);

err_out:
  if (fp) {
    fclose(fp);
  }
  if (had_error) {
    unlink(filename);
  }
  free(full_path);
  free(filename);
  free(data);
  return had_error;
}
