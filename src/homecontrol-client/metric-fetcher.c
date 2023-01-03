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

static int get_metrics(int sensor_number, const char *metric, int scale) {
  http_response *resp = NULL;
  FILE *fp;
  char *url = malloc(BUFSIZE);
  int err = 0;

  snprintf(url, BUFSIZE, HOMECONTROL_SRV"/csv/sensor_metrics.php"
                         "?sensor_number=%d"
                         "&metric=%s"
                         "&scale=%d",
                         sensor_number, metric, scale);

  printxcentered(12, "Fetching metrics, please be patient...");
  resp = http_request("GET", url, NULL, 0);
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
  int sensor_number, scale, i;
  char *buf = NULL;
  const char *metric = NULL;

  clrscr();

  printf("argc %d\n", argc);
  for (i = 0; i < argc; i++) {
    printf("argv[%d]: %s\n", i, argv[i]);
  }

  if (argc > 3) {
    sensor_number = atoi(argv[1]);
    metric = argv[2];
    scale = atoi(argv[3]);
  } else if (argc > 1) {
    char *tmp_metric = strchr(argv[1], ' ');
    char *tmp_scale;
    if (tmp_metric != NULL) {
      tmp_scale = strchr(tmp_metric + 1, ' ');
      if (tmp_scale != NULL) {
        scale = atoi(tmp_scale + 1);
      }
      *tmp_scale = '\0';
      metric = tmp_metric + 1;
      tmp_metric = '\0';
    }
    sensor_number = atoi(argv[1]);
  }

  buf = malloc(BUFSIZE);
#ifdef __CC65__
  if (get_metrics(sensor_number, metric, scale) == 0) {
    sprintf(buf, "%d %s %d", sensor_number, metric, scale);
    exec("GRPHVIEW", buf);
    free(buf); /* unreachable code anyway */
  } else {
    sprintf(buf, "2 %d", sensor_number);
    exec("HOMECTRL", buf);
    free(buf); /* unreachable code anyway */
  }
#else
  get_metrics(sensor_number, metric, scale);
#endif
  exit(0);
}
