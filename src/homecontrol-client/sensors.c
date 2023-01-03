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
#include "http.h"
#include "simple_serial.h"
#include "extended_conio.h"
#include "extended_string.h"
#include "slist.h"

#include "constants.h"
#include "network.h"
#include "sensors.h"
#include "server_url.h"

static slist *sensors = NULL;

static void sensor_add(char *id, char *name, char scale, long cur_value, char *unit) {
  hc_sensor *sensor = malloc(sizeof(hc_sensor));
  memset(sensor, 0, sizeof(hc_sensor));
  sensor->id        = strdup(id);
  sensor->scale     = scale;
  sensor->cur_value = cur_value;
  sensor->unit      = strdup(unit);
  if (strlen(name) >= 25) {
    name[22]='.';
    name[23]='.';
    name[24]='.';
    name[25]='\0';
  }
  sensor->name = strdup(name);

  sensors = slist_append(sensors, sensor);
}

static void sensor_free(hc_sensor *sensor) {
  free(sensor->id);
  free(sensor->name);
  free(sensor->unit);
  free(sensor);
}

void sensors_free_all(void) {
  slist *w;
  for (w = sensors; w; w = w->next) {
    sensor_free(w->data);
  }
  slist_free(sensors);
  sensors = NULL;
}

slist *update_sensors(void) {
  http_response *resp;
  char **lines = NULL;
  int i, num_lines;
  char *url = malloc(BUFSIZE);

  snprintf(url, BUFSIZE, "%s/sensors.php", get_server_root_url());
  resp = get_url(url);
  free(url);

  sensors_free_all();

  if (resp == NULL || resp->size == 0) {
    http_response_free(resp);
    return NULL;
  }
  num_lines = strsplit(resp->body, '\n', &lines);

  for (i = 0; i < num_lines; i++) {
    char **parts;
    int j, num_parts;
#ifdef __CC65__
  gotoxy(0,0);
  printf("%d/%d    ", _heapmaxavail(), _heapmemavail());
#endif
    num_parts = strsplit(lines[i],';', &parts);
    if (num_parts == 5) {
      sensor_add(parts[0], parts[1], atoi(parts[2]), atol(parts[3]), parts[4]);
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
