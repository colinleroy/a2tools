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

static void switches_free_all(void) {
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
  char **lines;
  int i, num_lines;
  
  switches_free_all();

  if (resp->size == 0) {
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
