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
#include "stp_save.h"
#include "get_buf_size.h"
#include "simple_serial.h"
#include "clrzone.h"
#include "extended_conio.h"
#include "math.h"

#define APPLESINGLE_HEADER_LEN 58

static unsigned char scrw = 255, scrh = 255;

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

void stp_save_dialog(char *url, surl_response *resp) {
  char *filename = strdup(strrchr(url, '/') + 1);
  char c;

  printxcenteredbox(30, 11);
  printxcentered(7, filename);

  gotoxy(6, 10);
  printf("%s", resp->content_type ? resp->content_type : "");
  gotoxy(6, 11);
  printf("%zu bytes", resp->size);

  gotoxy(6, 16);
  chline(28);
  gotoxy(6, 17);
  printf("Esc: cancel  !   Enter: Save");
  do {
    c = cgetc();
  } while (c != CH_ENTER && c != CH_ESC);
  
  if (c == CH_ENTER) {
    gotoxy(6, 17);
    printf("Saving file...              ");
    stp_save(filename, resp);
  }
  free(filename);
}

void stp_save(char *full_filename, surl_response *resp) {
  FILE *fp = NULL;
  char *data = NULL;
  char *filename;
  char *filetype;
  size_t r = 0, total = 0;
  unsigned int buf_size;
  char keep_bin_header = 0;
  char had_error = 0;

  if (scrw == 255)
    screensize(&scrw, &scrh);

  filename = strdup(full_filename);

  gotoxy(6, 12);

#ifdef __CC65__
  if (strchr(filename, '.') != NULL) {
    filetype = strrchr(filename, '.') + 1;
    *(strchr(filename, '.')) = '\0';
  } else {
    filetype = "TXT";
  }

#ifdef PRODOS_T_TXT
  if (!strcasecmp(filetype, "TXT")) {
    _filetype = PRODOS_T_TXT;
    _auxtype  = PRODOS_AUX_T_TXT_SEQ;
  } else if (!strcasecmp(filetype,"BIN")) {
    _filetype = PRODOS_T_BIN;

    /* Look into the header, and skip it by the way */
    data = malloc(APPLESINGLE_HEADER_LEN);
    r = surl_receive_data(resp, data, APPLESINGLE_HEADER_LEN);

    if (r == 58
     && data[0] == 0x00 && data[1] == 0x05
     && data[2] == 0x16 && data[3] == 0x00) {
      printf("AppleSingle: $%04x\n", (data[56]<<8 | data[57]));
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
    printf("Filetype unknown, using TXT.");
    free(filename);
    filename = strdup(full_filename);
    _filetype = PRODOS_T_TXT;
  }
#endif

  filename = cleanup_filename(filename);
#endif

  fp = fopen(filename, "w");
  if (fp == NULL) {
    gotoxy(6, 15);
    printf("%s", strerror(errno));
    cgetc();
    had_error = 1;
    goto err_out;
  }

  if (keep_bin_header) {
    if (fwrite(data, sizeof(char), APPLESINGLE_HEADER_LEN, fp) < APPLESINGLE_HEADER_LEN) {
      gotoxy(6, 15);
      printf("%s.", strerror(errno));
      cgetc();
      had_error = 1;
      goto err_out;
    }
    free(data);
  }

  buf_size = get_buf_size();
  data = malloc(buf_size + 1);

  if (data == NULL) {
    gotoxy(6, 15);
    printf("Cannot allocate buffer.");
    cgetc();
    had_error = 1;
    goto err_out;
  }

  progress_bar(6, 15, 28, 0, resp->size);
  do {
    simple_serial_set_activity_indicator(1, 39, 0);

    clrzone(6,14, 6+27, 14);
    gotoxy(6, 14);
    printf("Reading %zu bytes...", min(buf_size, resp->size - total));
    
    r = surl_receive_data(resp, data, buf_size);
    simple_serial_set_activity_indicator(0, 0, 0);

    gotoxy(6, 14);
    total += r;
    printf("Saving %zu/%zu...", total, resp->size);

    if (fwrite(data, sizeof(char), r, fp) < r) {
      gotoxy(6, 15);
      printf("%s.", strerror(errno));
      cgetc();
      had_error = 1;
      goto err_out;
    }

    progress_bar(6, 15, 28, total, resp->size);
  } while (r > 0);

err_out:
  fclose(fp);
  if (had_error) {
    unlink(filename);
  }
  free(filename);
  free(data);
}
