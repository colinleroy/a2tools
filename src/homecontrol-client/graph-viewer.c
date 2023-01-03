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
#include "tgi_compat.h"
#include "constants.h"
#include "extended_conio.h"

static long min_val = LONG_MAX;
static long max_val = LONG_MIN;
static long start_time = -1L;
static long end_time = -1L;

#define MIN_VAL_SCR_Y 145L
#define MAX_VAL_SCR_Y 5L
#define VAL_SCR_INTERVAL (MIN_VAL_SCR_Y - MAX_VAL_SCR_Y)

#define VAL_ZOOM_FACTOR ((max_val - min_val) / VAL_SCR_INTERVAL)
#define VAL_OFFSET_INTERVAL(val) ((val - min_val) / VAL_ZOOM_FACTOR)
#define VAL_Y(val) (MIN_VAL_SCR_Y - VAL_OFFSET_INTERVAL(val))

#define MIN_TIME_SCR_X 5L
#define MAX_TIME_SCR_X 285L
#define TIME_SCR_INTERVAL (MIN_TIME_SCR_X - MAX_TIME_SCR_X)

#define TIME_ZOOM_FACTOR (((end_time - start_time)) / TIME_SCR_INTERVAL)
#define TIME_OFFSET_INTERVAL(t) ((t - start_time) / TIME_ZOOM_FACTOR)
#define TIME_X(t) (MIN_TIME_SCR_X - TIME_OFFSET_INTERVAL(t))

static void display_graph(void) {
  FILE *fp;
  char *buf, *text;
  long prev_t = -1L;
  long prev_v = -1L;

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

    if (start_time == -1) {
      start_time = timestamp;
    }
    end_time = timestamp;

    if (value < min_val) {
      min_val = value;
    }
    if (value > max_val) {
      max_val = value;
    }
  }
  rewind(fp);

  gotoxy(0, 20);
  printf("X scale = Time: ");
  text = ctime((time_t *)&start_time);
  if (strchr(text, '\n'))
    *strchr(text, '\n') = '\0';
  printf("%s", text);
  text = ctime((time_t *)&end_time);
  if (strchr(text, '\n'))
    *strchr(text, '\n') = '\0';
  gotoxy(12, 21);
  printf("to: %s", text);
  gotoxy(0, 22);
  printf("Y scale = Value: %ld to %ld", (long)min_val, (long)max_val);

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
    if (prev_t == -1L) {
      tgi_setpixel((int)TIME_X(timestamp), (int)VAL_Y(value));
      // printf("pix at %d,%d (%ld,%ld)\n", (int)TIME_X(timestamp), (int)VAL_Y(value), TIME_X(timestamp), VAL_Y(value));
    } else {
      tgi_line((int)TIME_X(prev_t), (int)VAL_Y(prev_v), (int)TIME_X(timestamp), (int)VAL_Y(value));
      // printf("line from %d,%d-%d,%d (%ld,%ld-%ld,%ld)\n",
      //         (int)TIME_X(prev_t), (int)VAL_Y(prev_v), (int)TIME_X(timestamp), (int)VAL_Y(value),
      //         TIME_X(prev_t), VAL_Y(prev_v), TIME_X(timestamp), VAL_Y(value));
    }
    prev_t = timestamp;
    prev_v = value;
  }

  free(buf);

  if (fclose(fp) != 0) {
    printf("Cannot close file: %s\n", strerror(errno));
    cgetc();
    return;
  }

  cgetc();
}

int main(int argc, char **argv) {
  int sensor_number;
  char *buf = NULL;

  tgi_install(a2e_hi_tgi);

  clrscr();

  if (argc > 3) {
    sensor_number = atoi(argv[1]);
  } else {
    sensor_number=4;
  }

  display_graph();

#ifdef __CC65__
  buf = malloc(BUFSIZE);
  sprintf(buf, "2 %s", sensor_number);
  exec("HOMECTRL", buf);
  free(buf); /* unreachable code anyway */
#else
  /* TODO */
#endif
  exit(0);
}
