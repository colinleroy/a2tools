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
#include "extended_string.h"
#include "strsplit.h"

#include "constants.h"
#include "network.h"
#include "switches.h"
#include "server_url.h"

static hc_switch **switches = NULL;
static char *switches_data = NULL;
static int num_switches = 0;

static void switch_add(char *id, char *name, char *state) {
  hc_switch *sw = malloc(sizeof(hc_switch));
  memset(sw, 0, sizeof(hc_switch));
  sw->id    = id;
  sw->state = state;
  sw->name  = ellipsis(name, 25);

  switches[num_switches++] = sw;
}

static void switch_free(hc_switch *sw) {
  free(sw);
}

void switches_free_all(void) {
  int i;
  for (i = 0; i < num_switches; i++) {
    switch_free(switches[i]);
  }
  free(switches);
  free(switches_data);
  switches = NULL;
  switches_data = NULL;
  num_switches = 0;
}

int update_switches(hc_switch ***switches_list) {
  char **lines = NULL;
  int i, num_lines;
  char *url;

  switches_free_all();

  url = malloc(BUFSIZE);
  snprintf(url, BUFSIZE, "%s/switches.php", get_server_root_url());
  switches_data = get_url(url);
  free(url);

  if (switches_data == NULL) {
    *switches_list = NULL;
    return 0;
  }

  num_lines = strsplit_in_place(switches_data, '\n', &lines);
  switches = malloc(num_lines * sizeof(hc_switch *));

  for (i = 0; i < num_lines; i++) {
    char **parts;
    int num_parts;
    num_parts = strsplit_in_place(lines[i],';', &parts);
    if (num_parts == 3) {
      switch_add(parts[0], parts[1], parts[2]);
    }
    free(parts);
  }

  free(lines);

  *switches_list = switches;
  return num_switches;
}

void toggle_switch(hc_switch *sw) {
  char *response;
  char *url = malloc(BUFSIZE);

  printxcenteredbox(18, 5);
  printxcentered(12, "Toggling...");

  snprintf(url, BUFSIZE, "%s/switch_ctrl.php?id=%s", get_server_root_url(), sw->id);
  response = get_url(url);

  free(url);
  free(response);
}
