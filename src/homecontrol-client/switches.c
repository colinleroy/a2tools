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
#include "switches.h"

static slist *switches = NULL;

static void switch_add(char *id, char *name, char *state) {
  hc_switch *sw = malloc(sizeof(hc_switch));
  sw->id    = strdup(id);
  sw->name  = strdup(name);
  sw->state = strdup(state);

  switches = slist_append(switches, sw);
}

static void switch_free(hc_switch *sw) {
  free(sw->id);
  free(sw->name);
  free(sw->state);
  free(sw);
}

void switches_free_all(void) {
  slist *w;
  for (w = switches; w; w = w->next) {
    switch_free(w->data);
  }
  slist_free(switches);
  switches = NULL;
}

slist *switches_get(void) {
  return switches;
}

slist *update_switches(void) {
  http_response *resp = get_url(HOMECONTROL_SRV"/csv/switches.php");
  char **lines = NULL;
  int i, num_lines;
  
  switches_free_all();

  if (resp == NULL || resp->size == 0) {
    http_response_free(resp);
    return NULL;
  }

  num_lines = strsplit(resp->body, '\n', &lines);
  for (i = 0; i < num_lines; i++) {
    char **parts;
    int j, num_parts;
    num_parts = strsplit(lines[i],';', &parts);
    if (num_parts == 3) {
      switch_add(parts[0], parts[1], parts[2]);
    }
    for (j = 0; j < num_parts; j++) {
      free(parts[j]);
    }
    free(parts);
    free(lines[i]);
  }

  http_response_free(resp);
  free(lines);
  return switches;
}

void toggle_switch(hc_switch *sw) {
  http_response *resp;
  char *url = malloc(BUFSIZE);
  int i;

  printxcenteredbox(18, 5);
  printxcentered(12, "Toggling...");

  snprintf(url, BUFSIZE, HOMECONTROL_SRV"/control/toggle_switch.php?switch_number=%s", sw->id);
  resp = get_url(url);

  free(url);
  http_response_free(resp);
  for (i = 0; i < 10000; i++); /* wait long enough */
}
