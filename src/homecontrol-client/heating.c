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
#include "heating.h"

static slist *heating_zones = NULL;

static char *do_round(char *floatval, int num) {
  char *result = strdup(floatval);
  char *decdot = strchr(result, '.');
  int offset = (num > 0) ? (num + 1) : 0;

  if (decdot && strlen(decdot) > offset) {
    *(decdot + offset) = '\0';
  }
  
  return result;
}

static void heating_add(char *id, char *name, char *set_temp, char *cur_temp, char *cur_humidity) {
  hc_heating *heat = malloc(sizeof(hc_heating));
  heat->id           = strdup(id);
  heat->name         = strdup(name);
  heat->set_temp     = do_round(set_temp, 0);
  if (strcmp(cur_temp, "n/a")) {
    heat->cur_temp     = do_round(cur_temp, 1);
  } else {
    heat->cur_temp     = strdup("NA");
  }
  if (strcmp(cur_humidity, "n/a")) {
    heat->cur_humidity = do_round(cur_humidity, 0);
  } else {
    heat->cur_humidity = strdup("NA");
  }

  heating_zones = slist_append(heating_zones, heat);
}

static void heating_free(hc_heating *heat) {
  free(heat->id);
  free(heat->name);
  free(heat->set_temp);
  free(heat->cur_temp);
  free(heat->cur_humidity);
  free(heat);
}

static void heating_zones_free_all(void) {
  slist *w;
  for (w = heating_zones; w; w = w->next) {
    heating_free(w->data);
  }
  slist_free(heating_zones);
  heating_zones = NULL;
}

slist *heating_zones_get(void) {
  return heating_zones;
}

slist *update_heating_zones(void) {
  http_response *resp = get_url(HOMECONTROL_SRV"/csv/tado_zones.php");
  char **lines;
  int i, num_lines;
  
  heating_zones_free_all();

  if (resp->size == 0) {
    http_response_free(resp);
    return NULL;
  }
  num_lines = strsplit(resp->body, '\n', &lines);
  for (i = 0; i < num_lines; i++) {
    char **parts;
    int j, num_parts;
    num_parts = strsplit(lines[i],';', &parts);
    if (num_parts == 5) {
      heating_add(parts[0], parts[1], parts[2], parts[3], parts[4]);
    }
    for (j = 0; j < num_parts; j++) {
      free(parts[j]);
    }
    free(parts);
    free(lines[i]);
  }

  http_response_free(resp);
  free(lines);
  return heating_zones;
}
