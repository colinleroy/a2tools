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

static long min_val = LONG_MAX;
static long max_val = LONG_MIN;
static long start_time = -1L;
static long end_time = -1L;

static char *sensor_name, *unit;

#define MIN_VAL_SCR_Y 145L
#define MAX_VAL_SCR_Y 5L
#define VAL_SCR_INTERVAL (MIN_VAL_SCR_Y - MAX_VAL_SCR_Y)

#define VAL_ZOOM_FACTOR ((max_val - min_val) / VAL_SCR_INTERVAL)
#define VAL_OFFSET_INTERVAL(val) ((val - min_val) / VAL_ZOOM_FACTOR)
#define VAL_Y(val) max(0L,(MIN_VAL_SCR_Y - VAL_OFFSET_INTERVAL(val)))

#define MIN_TIME_SCR_X 5L
#define MAX_TIME_SCR_X 275L
#define TIME_SCR_INTERVAL (MIN_TIME_SCR_X - MAX_TIME_SCR_X)

#define TIME_ZOOM_FACTOR (((end_time - start_time)) / TIME_SCR_INTERVAL)
#define TIME_OFFSET_INTERVAL(t) ((t - start_time) / TIME_ZOOM_FACTOR)
#define TIME_X(t) max(0L,(MIN_TIME_SCR_X - TIME_OFFSET_INTERVAL(t)))

static void display_graph(void) {
  FILE *fp;
  char *buf, *text;
  long prev_t = -1L;
  long prev_v = -1L;
  int line;

#ifdef PRODOS_T_TXT
  gotoxy(12, 12);
  printf("Loading metrics...");
  _filetype = PRODOS_T_TXT;
#endif
  fp = fopen(METRICS_TMPFILE, "rb"); 
  if (fp == NULL) {
    tgi_done();
    printf("Error opening file: %s\n", strerror(errno));
    cgetc();
    return;
  }

  buf = malloc(BUFSIZE);
  /* get mins and maxes */
  if (fgets(buf, BUFSIZE, fp) != NULL) {
    if(!strncmp(buf, "TIME;", 5) && strchr(buf + 6, ';')) {
      char *w = strchr(buf, ';') + 1;
      start_time = atol(w);
      w = strchr(w,';') + 1;
      end_time = atol(w);
    }
  }
  if (fgets(buf, BUFSIZE, fp) != NULL) {
    if(!strncmp(buf, "VALS;", 5) && strchr(buf + 5, ';')) {
      char *w = strchr(buf, ';') + 1;
      min_val = atol(w);
      w = strchr(w,';') + 1;
      max_val = atol(w);
    }
  }

  line = 20;
  gotoxy(0, line);
  printf("%s", sensor_name);

  line++;
  gotoxy(0, line);
  printf("X scale = Time: ");
  text = ctime((time_t *)&start_time);
  if (strchr(text, '\n'))
    *strchr(text, '\n') = '\0';
  printf("%s", text);
  text = ctime((time_t *)&end_time);
  if (strchr(text, '\n'))
    *strchr(text, '\n') = '\0';

  line++;
  gotoxy(12, line);
  printf("to: %s", text);

  line++;
  gotoxy(0, line);
  printf("Y scale = Value: %ld to %ld %s", (long)min_val, (long)max_val, unit);

  tgi_init();
  tgi_apple2_mix(1);

  text = NULL;
  /* Draw scales */
  tgi_setcolor(TGI_COLOR_ORANGE);
  tgi_line(MIN_TIME_SCR_X, MIN_VAL_SCR_Y, MAX_TIME_SCR_X, MIN_VAL_SCR_Y);
  
  tgi_setcolor(TGI_COLOR_ORANGE);
  tgi_line(MIN_TIME_SCR_X, MIN_VAL_SCR_Y, MIN_TIME_SCR_X, MAX_VAL_SCR_Y);

  // text = malloc(BUFSIZE);
  // 
  // snprintf(text, BUFSIZE, "%ld", max_val);
  // tgi_outtextxy(0, MAX_VAL_SCR_Y, text);
  // snprintf(text, BUFSIZE, "%ld", min_val);
  // tgi_outtextxy(0, MIN_VAL_SCR_Y, text);
  // 
  // snprintf(text, BUFSIZE, "%s", ctime((time_t *)&start_time));
  // tgi_outtextxy(MIN_TIME_SCR_X, MIN_VAL_SCR_Y + 10, text);
  // 
  // snprintf(text, BUFSIZE, "%s", ctime((time_t *)&end_time));
  // tgi_outtextxy(MAX_TIME_SCR_X - 60, MIN_VAL_SCR_Y + 10, text);
  // 
  // free(text);

  tgi_setcolor(TGI_COLOR_WHITE);
  while (fgets(buf, BUFSIZE, fp) != NULL) {
    long timestamp;
    long value;
    if (strchr(buf, ';')) {
      value = atol(strchr(buf, ';') + 1);
      *strchr(buf, ';') = '\0';
      timestamp = atol(buf);
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
        tgi_line((int)TIME_X(prev_t), (int)VAL_Y(prev_v), (int)TIME_X(timestamp), (int)VAL_Y(value));
      } else {
        tgi_line((int)TIME_X(prev_t), (int)VAL_Y(prev_v), (int)TIME_X(prev_t), (int)VAL_Y(value));
        tgi_line((int)TIME_X(prev_t), (int)VAL_Y(value), (int)TIME_X(timestamp), (int)VAL_Y(value));
      }
      // printf("line from %d,%d-%d,%d (%ld,%ld-%ld,%ld)\n",
      //         (int)TIME_X(prev_t), (int)VAL_Y(prev_v), (int)TIME_X(timestamp), (int)VAL_Y(value),
      //         TIME_X(prev_t), VAL_Y(prev_v), TIME_X(timestamp), VAL_Y(value));
      prev_t = timestamp;
      prev_v = value;
    }
  }

  free(buf);

  if (fclose(fp) != 0) {
    printf("Cannot close file: %s\n", strerror(errno));
    cgetc();
    return;
  }
  unlink(METRICS_TMPFILE);

  cgetc();
  tgi_done();

}

int main(int argc, char **argv) {
  char *sensor_id;
  char *buf = NULL;
  int scale;

  tgi_install(a2e_hi_tgi);

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

  display_graph();
err_out:
  tgi_uninstall();
  clrscr();
  gotoxy(12, 12);
  printf("Please wait...");
#ifdef __CC65__
  buf = malloc(BUFSIZE);
  sprintf(buf, "2 %s", sensor_id);
  exec("HOMECTRL", buf);
  free(buf); /* unreachable code anyway */
#else
  /* TODO */
#endif
  exit(0);
}
