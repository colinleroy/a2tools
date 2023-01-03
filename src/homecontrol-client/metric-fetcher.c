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

static int get_metrics(const char *sensor_number, const char *metric, int scale) {
  http_response *resp = NULL;
  FILE *fp;
  char *buf = malloc(BUFSIZE);
  char *w;
  int err = 0;
  static long min_val = LONG_MAX;
  static long max_val = LONG_MIN;
  static long start_time = -1L;
  static long end_time = -1L;

  snprintf(buf, BUFSIZE, HOMECONTROL_SRV"/csv/sensor_metrics.php"
                         "?sensor_number=%s"
                         "&metric=%s"
                         "&scale=%d",
                         sensor_number, metric, scale);

  gotoxy(1, 12);
  printf("Fetching metrics, please be patient...");
  simple_serial_set_activity_indicator(1, -1, -1);
  resp = http_request("GET", buf, NULL, 0);
  simple_serial_set_activity_indicator(0, 0, 0);
  free(buf);

  if (resp == NULL) {
    printf("Could not get response\n");
    cgetc();
    return -1;
  }

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

  w = resp->body;
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
  if (fwrite(resp->body, sizeof(char), resp->size, fp) < resp->size) {
    printf("Cannot write to file: %s\n", strerror(errno));
    cgetc();
    err = -1;
  }
  http_response_free(resp);
  
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
  const char *sensor_number, *metric = NULL, *unit = NULL;

  clrscr();

  if (argc > 4) {
    sensor_number = argv[1];
    metric = argv[2];
    scale = atoi(argv[3]);
    unit = argv[4];
  } else {
    gotoxy(12, 13);
    printf("Missing argument(s).");
    cgetc();
    goto err_out;
  }

  buf = malloc(BUFSIZE);
#ifdef __CC65__
  if (get_metrics(sensor_number, metric, scale) == 0) {
    sprintf(buf, "%s %s %d %s", sensor_number, metric, scale, unit);
    exec("GRPHVIEW", buf);
    free(buf); /* unreachable code anyway */
  } else {
err_out:
    sprintf(buf, "2 %s", sensor_number);
    exec("HOMECTRL", buf);
    free(buf); /* unreachable code anyway */
  }
#else
  get_metrics(sensor_number, metric, scale);
err_out:
#endif
  exit(0);
}
