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
#ifdef __CC65__
#include <apple2enh.h>
#endif
#include "stp_save.h"
#include "simple_serial.h"
#include "extended_conio.h"
#include "math.h"

#define APPLESINGLE_HEADER_LEN 58

static unsigned char scrw = 255, scrh = 255;

static unsigned int get_buf_size(void) {
#ifdef __CC65__
  unsigned int avail = _heapmaxavail();
  if (avail > 18000)
    return 16384;
  if (avail > 9000)
    return 8192;
  if (avail > 5000)
    return 4096;
  if (avail > 3000)
    return 2048;
  
  return 1024;
#else
  return 32768;
#endif
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

void stp_save(char *full_filename, surl_response *resp) {
  FILE *fp = NULL;
  char *data = NULL;
  char *filename;
  char *filetype;
  size_t r = 0, total = 0;
  unsigned int buf_size, i;
  unsigned long percent;
  char keep_bin_header = 0;
  
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

  filename = cleanup_filename(filename);
#endif

  fp = fopen(filename, "w");
  if (fp == NULL) {
    gotoxy(6, 15);
    printf("%s", strerror(errno));
    cgetc();
    goto err_out;
  }

  if (keep_bin_header) {
    fwrite(data, sizeof(char), APPLESINGLE_HEADER_LEN, fp);
    free(data);
  }

  buf_size = get_buf_size();
  data = malloc(buf_size + 1);

  if (data == NULL) {
    gotoxy(6, 15);
    printf("Cannot allocate buffer.");
    goto err_out;
  }

  gotoxy(6, 15);
  for (i = 0; i < 28; i++)
    printf("-");

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

    fwrite(data, sizeof(char), r, fp);

    gotoxy(6, 15);
    percent = (long)total * 28L;
    percent = percent/((long)resp->size);
    for (i = 0; i <= ((int)percent) && i < 28; i++)
      printf("*");
    for (i = (int)(percent + 1L); i < 28; i++)
      printf("-");

  } while (r > 0);

err_out:
  fclose(fp);
  free(filename);
  free(data);
}
