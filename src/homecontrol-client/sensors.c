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

static slist *sensors = NULL;
static slist *sensor_metrics = NULL;

static void sensor_add(const char *id, const char *name) {
  hc_sensor *sensor = malloc(sizeof(hc_sensor));
  sensor->id    = strdup(id);
  sensor->name  = strdup(name);

  sensors = slist_append(sensors, sensor);
}

static void sensor_metric_add(const char *sensor_id, const char *id, const char *name, int scale, const char *unit) {
  hc_sensor_metric *metric = malloc(sizeof(hc_sensor_metric));
  metric->sensor_id = strdup(sensor_id);
  metric->id        = strdup(id);
  metric->name      = strdup(name);
  metric->scale     = scale;
  metric->unit      = strdup(unit);
  
  sensor_metrics = slist_append(sensor_metrics, metric);
}

static void sensor_free(hc_sensor *sensor) {
  free(sensor->id);
  free(sensor->name);
  free(sensor);
}

static void sensor_metric_free(hc_sensor_metric *metric) {
  free(metric->sensor_id);
  free(metric->id);
  free(metric->name);
  free(metric->unit);
  free(metric);
}

void sensors_free_all(void) {
  slist *w;
  for (w = sensors; w; w = w->next) {
    sensor_free(w->data);
  }
  slist_free(sensors);
  sensors = NULL;
}

void sensor_metrics_free_all(void) {
  slist *w;
  for (w = sensor_metrics; w; w = w->next) {
    sensor_metric_free(w->data);
  }
  slist_free(sensor_metrics);
  sensor_metrics = NULL;
}

slist *sensors_get(void) {
  return sensors;
}

slist *update_sensors(void) {
  http_response *resp = get_url(HOMECONTROL_SRV"/csv/sensors.php");
  char **lines = NULL;
  int i, num_lines;
  
  sensors_free_all();

  if (resp == NULL || resp->size == 0) {
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

slist *update_sensor_metrics(const char *id) {
  char *url = malloc(BUFSIZE);
  http_response *resp;
  char **lines = NULL;
  int i, num_lines;
  
  snprintf(url, BUFSIZE, HOMECONTROL_SRV"/csv/sensor_metric_list.php?sensor_number=%s", id);
  resp = get_url(url);
  free(url);

  sensor_metrics_free_all();

  if (resp == NULL || resp->size == 0) {
    http_response_free(resp);
    return NULL;
  }
  num_lines = strsplit(resp->body, '\n', &lines);
  for (i = 0; i < num_lines; i++) {
    char **parts;
    int j, num_parts;
    num_parts = strsplit(lines[i],';', &parts);
    if (num_parts == 4) {
      sensor_metric_add(id, parts[0], parts[1], atoi(parts[2]), parts[3]);
    }
    for (j = 0; j < num_parts; j++) {
      free(parts[j]);
    }
    free(parts);
    free(lines[i]);
  }

  http_response_free(resp);
  free(lines);
  return sensor_metrics;
}
