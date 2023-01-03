#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
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
  char *url = malloc(BUFSIZE);
  int err = 0;

  snprintf(url, BUFSIZE, HOMECONTROL_SRV"/csv/sensor_metrics.php"
                         "?sensor_number=%s"
                         "&metric=%s"
                         "&scale=%d",
                         sensor_number, metric, scale);

  printxcentered(12, "Fetching metrics, please be patient...");
  simple_serial_set_activity_indicator(1, -1, -1);
  resp = http_request("GET", url, NULL, 0);
  simple_serial_set_activity_indicator(0, 0, 0);
  free(url);

  if (resp == NULL) {
    printf("Could not get response\n");
    cgetc();
    return -1;
  }
  printxcentered(13,"Writing metrics...");

#ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
#endif
  fp = fopen(METRICS_TMPFILE, "wb");
  if (fp == NULL) {
    printf("Error opening file: %s\n", strerror(errno));
    cgetc();
    return -1;
  }
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
    printxcentered(13,"Missing argument(s).");
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
