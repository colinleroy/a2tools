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
#include <errno.h>
#include <limits.h>
#ifdef __CC65__
#include <apple2enh.h>
#endif
#include "http.h"
#include "constants.h"
#include "extended_conio.h"
#include "simple_serial.h"

/* Copied from server_url.c for code size */
static char* server_url = NULL;


static int get_metrics(const char *sensor_id, int scale, char *unit) {
  http_response *resp = NULL;
  char *body = NULL;
  FILE *fp;
  char *buf = malloc(BUFSIZE);
  char *w;
  int err = 0;
  static long min_val = LONG_MAX;
  static long max_val = LONG_MIN;
  static long start_time = -1L;
  static long end_time = -1L;

  snprintf(buf, BUFSIZE, "%s/sensor_metrics.php"
                         "?sensor_id=%s"
                         "&scale=%d"
                         "&unit=%s",
                         server_url,
                         sensor_id, scale, unit);

  gotoxy(1, 12);
  printf("Fetching metrics, please be patient...");
  simple_serial_set_activity_indicator(1, -1, -1);
  resp = http_start_request("GET", buf, NULL, 0);
  simple_serial_set_activity_indicator(0, 0, 0);
  free(buf);

  if (resp == NULL || resp->code != 200) {
    printf("Could not get response\n");
    http_response_free(resp);
    cgetc();
    return -1;
  }

  body = malloc(resp->size + 1);
  http_receive_data(resp, body, resp->size + 1);

  gotoxy(12, 13);
  printf("Writing metrics...");

#ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
#endif
  fp = fopen(METRICS_TMPFILE, "wb");
  if (fp == NULL) {
    printf("Error opening file: %s\n", strerror(errno));
    cgetc();
    return -1;
  }

  w = body;
  while (*w) {
    char *vs;
    long v;
    if (start_time == -1L)
      start_time = atol(w);
    end_time = atol(w);
    vs = strchr(w, ';');
    if (vs) {
      vs = vs + 1;
      v = atol(vs);

      if (v < min_val) {
        min_val = v;
      }
      if (v > max_val) {
        max_val = v;
      }
    }
    while (*w != '\n' && *w != '\0')
      w++;
    if (*w == '\n')
      w++;
  }

  fprintf(fp, "TIME;%ld;%ld\n", start_time, end_time);
  fprintf(fp, "VALS;%ld;%ld\n", min_val, max_val);
  if (fwrite(body, sizeof(char), resp->size, fp) < resp->size) {
    printf("Cannot write to file: %s\n", strerror(errno));
    cgetc();
    err = -1;
  }
  http_response_free(resp);
  free(body);
  
  if (fclose(fp) != 0) {
    printf("Cannot close file: %s\n", strerror(errno));
    cgetc();
    err = -1;
  }

  return err;
}

int main(int argc, char **argv) {
  int scale;
  char *buf = NULL;
  char *sensor_id, *sensor_name, *unit = NULL;
  FILE *fp;

#ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
#endif

  clrscr();

  if (argc > 4) {
    sensor_id = argv[1];
    sensor_name = argv[2];
    scale = atoi(argv[3]);
    unit = argv[4];
  } else {
    gotoxy(12, 13);
    printf("Missing argument(s).");
    cgetc();
    goto err_out;
  }

  if (server_url == NULL) {
    server_url = malloc(BUFSIZE);
    fp = fopen(SRV_URL_FILE,"r");
    if (fp != NULL) {
      fgets(server_url, BUFSIZE, fp);
      *strchr(server_url,'\n') = '\0';
      fclose(fp);
    } else {
      printf("Can't load server URL.");
      cgetc();
      goto err_out;
    }
  }

  buf = malloc(BUFSIZE);
#ifdef __CC65__
  if (get_metrics(sensor_id, scale, unit) == 0) {
    sprintf(buf, "%s \"%s\" %d %s", sensor_id, sensor_name, scale, unit);
    simple_serial_close();
    exec("GRPHVIEW", buf);
    free(buf); /* unreachable code anyway */
    free(server_url);
  } else {
err_out:
    sprintf(buf, "2 %s", sensor_id);
    simple_serial_close();
    exec("HOMECTRL", buf);
    free(buf); /* unreachable code anyway */
    free(server_url);
  }
#else
  get_metrics(sensor_id, scale, unit);
  free(buf);
  free(server_url);
  simple_serial_close();
err_out:
#endif
  exit(0);
}
