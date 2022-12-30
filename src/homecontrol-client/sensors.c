#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "http.h"
#include "simple_serial.h"
#include "extended_conio.h"
#include "extended_string.h"
#include "slist.h"

#include "constants.h"
#include "network.h"
#include "sensors.h"

static slist *sensors = NULL;

static void sensor_add(char *id, char *name) {
  hc_sensor *sensor = malloc(sizeof(hc_sensor));
  sensor->id    = strdup(id);
  sensor->name  = strdup(name);

  sensors = slist_append(sensors, sensor);
}

static void sensor_free(hc_sensor *sensor) {
  free(sensor->id);
  free(sensor->name);
  free(sensor);
}

static void sensors_free_all(void) {
  slist *w;
  for (w = sensors; w; w = w->next) {
    sensor_free(w->data);
  }
  slist_free(sensors);
  sensors = NULL;
}

slist *sensors_get(void) {
  return sensors;
}

slist *update_sensors(void) {
  http_response *resp = get_url(HOMECONTROL_SRV"/csv/sensors.php");
  char **lines;
  int i, num_lines;
  
  sensors_free_all();

  if (resp->size == 0) {
    http_response_free(resp);
    return NULL;
  }
  num_lines = strsplit(resp->body, '\n', &lines);
  for (i = 0; i < num_lines; i++) {
    char **parts;
    int j, num_parts;
    num_parts = strsplit(lines[i],';', &parts);
    if (num_parts == 2) {
      sensor_add(parts[0], parts[1]);
    }
    for (j = 0; j < num_parts; j++) {
      free(parts[j]);
    }
    free(parts);
    free(lines[i]);
  }

  http_response_free(resp);
  free(lines);
  return sensors;
}
