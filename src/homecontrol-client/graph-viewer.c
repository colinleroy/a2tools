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
#include <time.h>
#ifdef __CC65__
#include <apple2enh.h>
#endif
#include "math.h"
#include "tgi_compat.h"
#include "constants.h"
#include "extended_conio.h"
#include "surl.h"


#ifdef __CC65__
#pragma code-name (push, "LOWCODE")
#endif

#define MIN_VAL_SCR_Y 145L
#define MAX_VAL_SCR_Y 5L
#define VAL_SCR_INTERVAL (MIN_VAL_SCR_Y - MAX_VAL_SCR_Y)

#define VAL_ZOOM_FACTOR ((max_val - min_val) / VAL_SCR_INTERVAL)

static long val_div = -1L;
static long val_mul = -1L;
static int val_y(int val, long min, long max) {
  if (max - min >= VAL_SCR_INTERVAL) {
    /* zoom out */
    if (val_div == -1L)
      val_div = ((max - min) / VAL_SCR_INTERVAL);

    return max(0L,(MIN_VAL_SCR_Y - ((val - min) / val_div)));
  } else {
    /* zoom in */
    if (val_mul == -1L)
      val_mul = VAL_SCR_INTERVAL / (max - min);

    return max(0L,(MIN_VAL_SCR_Y - ((val - min) * val_mul )));
  }
}

#define MIN_TIME_SCR_X 5L
#define MAX_TIME_SCR_X 275L
#define TIME_SCR_INTERVAL (MIN_TIME_SCR_X - MAX_TIME_SCR_X)

#define TIME_ZOOM_FACTOR (((end_time - start_time)) / TIME_SCR_INTERVAL)
#define TIME_OFFSET_INTERVAL(t) ((t - start_time) / TIME_ZOOM_FACTOR)
#define TIME_X(t) max(0L,(MIN_TIME_SCR_X - TIME_OFFSET_INTERVAL(t)))

int main(int argc, char **argv) {
  char *server_url = NULL;
  FILE *fp;
  char *buf = NULL;
  char *text;
  long prev_t = -1L;
  long prev_v = -1L;
  char *cur_line;
  char header_done = 0;
  long timestamp;
  long value;
  long min_val = LONG_MAX;
  long max_val = LONG_MIN;
  long start_time = -1L;
  long end_time = -1L;

#ifdef __CC65__
  _heapadd ((void *) 0x0803, 0x17FD);
#endif

  tgi_install(a2e_hi_tgi);

  clrscr();

#ifdef __CC65__
  gotoxy(5, 10);
  printf("Free memory: %zu bytes.", _heapmemavail());
#endif

  if (argc < 5) {
    gotoxy(12, 13);
    printf("Missing argument(s).");
    cgetc();
    goto err_out;
  }

  if (server_url == NULL) {
    server_url = malloc(BUFSIZE);
#ifdef __CC65__
    if (server_url == NULL) {
      printf("Cant alloc server_url buffer. (%zu avail)", _heapmaxavail());
    }
#endif
    fp = fopen(SRV_URL_FILE, "r");
    if (fp != NULL) {
      fgets(server_url, BUFSIZE, fp);
      *strchr(server_url,'\n') = '\0';
      fclose(fp);
    } else {
#ifdef __CC65__
      printf("Can't load server URL (%zu avail).", _heapmaxavail());
#endif
      cgetc();
      goto err_out;
    }
  }

#ifdef PRODOS_T_TXT
  gotoxy(10, 12);
  printf("Loading metrics...");
#endif

  buf = malloc(BIG_BUFSIZE);
  if (buf == NULL) {
#ifdef __CC65__
    printf("Cannot allocate buffer. (%zu avail)", _heapmaxavail());
#endif
    goto err_out;
  }

  snprintf(buf, BUFSIZE, "%s/sensor_metrics.php"
                         "?sensor_id=%s"
                         "&scale=%s"
                         "&unit=%s",
                         server_url,
                         argv[1], argv[3], argv[4]);

  surl_start_request(SURL_METHOD_GET, buf, NULL, 0);
  if (!surl_response_ok()) {
    gotoxy(12, 10);
    printf("Error loading metrics.");
    cgetc();
    goto err_out;
  }

  while (surl_receive_lines(buf, BIG_BUFSIZE - 1) > 0) {
    cur_line = buf;

    while (cur_line != NULL && *cur_line != '\0') {
      char *next_line = NULL;
      char *line_sep = strchr(cur_line, '\n');
      if (line_sep) {
        next_line = line_sep + 1;
        *line_sep = '\0';
      }

      /* get mins and maxes */
      if (header_done < 2) {
        if(!strncmp(cur_line, "TIME;", 5) && strchr(cur_line + 6, ';')) {
          char *w = strchr(cur_line, ';') + 1;
          start_time = atol(w);
          w = strchr(w,';') + 1;
          end_time = atol(w);
          header_done++;
          cur_line = next_line;
          continue;
        }
        if(!strncmp(cur_line, "VALS;", 5) && strchr(cur_line + 5, ';')) {
          char *w = strchr(cur_line, ';') + 1;
          min_val = atol(w);
          w = strchr(w,';') + 1;
          max_val = atol(w);
          header_done++;
          cur_line = next_line;
          continue;
        }
      } 

      if (header_done == 2) {
        gotoxy(0, 20);
        printf("%s", argv[2]);

        gotoxy(0, 21);
        printf("X scale = Time: ");
        text = ctime((time_t *)&start_time);
        if (strchr(text, '\n'))
          *strchr(text, '\n') = '\0';
        printf("%s", text);
        text = ctime((time_t *)&end_time);
        if (strchr(text, '\n'))
          *strchr(text, '\n') = '\0';

        gotoxy(12, 22);
        printf("to: %s", text);

        gotoxy(0, 23);
        printf("Y scale = Value: %ld to %ld %s", (long)min_val, (long)max_val, argv[4]);

        tgi_init();
        tgi_clear();
        tgi_apple2_mix(1);

        /* Draw scales */
        tgi_setcolor(TGI_COLOR_ORANGE);
        tgi_line(MIN_TIME_SCR_X, MIN_VAL_SCR_Y, MAX_TIME_SCR_X, MIN_VAL_SCR_Y);
        
        tgi_setcolor(TGI_COLOR_ORANGE);
        tgi_line(MIN_TIME_SCR_X, MIN_VAL_SCR_Y, MIN_TIME_SCR_X, MAX_VAL_SCR_Y);

        header_done = 3;
      }

      tgi_setcolor(TGI_COLOR_WHITE);

      if (strchr(cur_line, ';')) {
        value = atol(strchr(cur_line, ';') + 1);
        *strchr(cur_line, ';') = '\0';
        timestamp = atol(cur_line);
      } else {
        continue;
      }
      if (prev_t == -1L && value == 0L) {
        /* Skip first val */
      } else {
        if (prev_v == -1L){
          prev_v = value;
          prev_t = start_time;
        }
        if ((int)TIME_X(timestamp) - (int)TIME_X(prev_t) < 6) {
          tgi_line((int)TIME_X(prev_t), val_y(prev_v, min_val, max_val), (int)TIME_X(timestamp), val_y(value, min_val, max_val));
        } else {
          tgi_line((int)TIME_X(prev_t), val_y(prev_v, min_val, max_val), (int)TIME_X(prev_t), val_y(value, min_val, max_val));
          tgi_line((int)TIME_X(prev_t), val_y(value, min_val, max_val), (int)TIME_X(timestamp), val_y(value, min_val, max_val));
        }
        prev_t = timestamp;
        prev_v = value;

      }
      cur_line = next_line;
    }
  }

  free(buf);

  cgetc();
  tgi_done();

err_out:
  tgi_uninstall();
  clrscr();
  gotoxy(12, 12);
  printf("Please wait...");
#ifdef __CC65__
  exec("HOMECTRL", "2");
  free(buf); /* unreachable code anyway */
#else
  /* TODO */
#endif
  exit(0);
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
