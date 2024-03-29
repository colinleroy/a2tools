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
#include "extended_conio.h"
#include "strellipsis.h"
#include "strsplit.h"

#include "constants.h"
#include "network.h"
#include "sensors.h"
#include "server_url.h"

static hc_sensor **sensors = NULL;
static char *sensors_data = NULL;
static int num_sensors = 0;

static void sensor_add(char *id, char *name, char scale, long cur_value, char *unit) {
  hc_sensor *sensor = malloc(sizeof(hc_sensor));
  memset(sensor, 0, sizeof(hc_sensor));
  sensor->id        = id;
  sensor->scale     = scale;
  sensor->cur_value = cur_value;
  sensor->unit      = unit;
  sensor->name      = ellipsis(name, 25);

  sensors[num_sensors++] = sensor;
}

static void sensor_free(hc_sensor *sensor) {
  free(sensor);
}

void sensors_free_all(void) {
  int i;
  for (i = 0; i < num_sensors; i++) {
    sensor_free(sensors[i]);
  }
  free(sensors);
  free(sensors_data);
  sensors = NULL;
  sensors_data = NULL;
  num_sensors = 0;
}

int update_sensors(hc_sensor ***sensors_list) {
  char **lines = NULL;
  int i, num_lines;
  char *url;

  sensors_free_all();

  url = malloc(BUFSIZE);
  snprintf(url, BUFSIZE, "%s/sensors.php", get_server_root_url());
  sensors_data = get_url(url);
  free(url);

  if (sensors_data == NULL) {
    *sensors_list = NULL;
    return 0;
  }

  num_lines = strsplit_in_place(sensors_data, '\n', &lines);
  sensors = malloc(num_lines * sizeof(hc_sensor *));

  for (i = 0; i < num_lines; i++) {
    char **parts;
    int num_parts;

    num_parts = strsplit_in_place(lines[i],';', &parts);
    if (num_parts == 5) {
      sensor_add(parts[0], parts[1], atoi(parts[2]), atol(parts[3]), parts[4]);
    }
    free(parts);
  }

  free(lines);

  *sensors_list = sensors;
  return num_sensors;
}
