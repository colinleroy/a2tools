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

static void heating_add(char *id, char *name, char *set_temp, char *cur_temp, char *cur_humidity, char manual_mode) {
  hc_heating_zone *heat = malloc(sizeof(hc_heating_zone));
  heat->id           = strdup(id);
  heat->name         = strdup(name);
  heat->set_temp     = atoi(set_temp);
  heat->manual_mode  = manual_mode;

  if (!strcmp(heat->id, "-1")) {
    heat->set_temp   = 21;
  }

  if (strcmp(cur_temp, "n/a")) {
    if (strstr(cur_temp, " - ")) {
      char **min_max;
      int num_parts, i;
      num_parts = strsplit(cur_temp, ' ', &min_max);

      if (num_parts == 3) {
        int avg = (atoi(min_max[2]) + atoi(min_max[0])) / 2;
        heat->cur_temp = malloc(4);
        sprintf(heat->cur_temp, "%d", avg);
      } else {
        heat->cur_temp = strdup("NA");
      }

      for (i = 0; i  < num_parts; i++) {
        free(min_max[i]);
      }
      free(min_max);
    } else {
      heat->cur_temp = do_round(cur_temp, 0);
    }
  } else {
    heat->cur_temp     = strdup("NA");
  }
  if (strcmp(cur_humidity, "n/a")) {
    if (strstr(cur_humidity, " - ")) {
      char **min_max;
      int num_parts, i;
      num_parts = strsplit(cur_humidity, ' ', &min_max);

      if (num_parts == 3) {
        int avg = (atoi(min_max[2]) + atoi(min_max[0])) / 2;
        heat->cur_humidity = malloc(4);
        sprintf(heat->cur_humidity, "%d", avg);
      } else {
        heat->cur_humidity = strdup("NA");
      }

      for (i = 0; i  < num_parts; i++) {
        free(min_max[i]);
      }
      free(min_max);
    } else {
      heat->cur_humidity = do_round(cur_humidity, 0);
    }
  } else {
    heat->cur_humidity = strdup("NA");
  }

  heating_zones = slist_append(heating_zones, heat);
}

static void heating_free(hc_heating_zone *heat) {
  free(heat->id);
  free(heat->name);
  free(heat->cur_temp);
  free(heat->cur_humidity);
  free(heat);
}

void heating_zones_free_all(void) {
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
  char **lines = NULL;
  int i, num_lines;
  
  heating_zones_free_all();

  if (resp == NULL || resp->size == 0) {
    http_response_free(resp);
    return NULL;
  }
  num_lines = strsplit(resp->body, '\n', &lines);
  for (i = 0; i < num_lines; i++) {
    char **parts;
    int j, num_parts;
    num_parts = strsplit(lines[i],';', &parts);
    if (num_parts == 6) {
      heating_add(parts[0], parts[1], parts[2], parts[3], parts[4], !strcmp(parts[5], "MANUAL"));
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


int configure_heating_zone(hc_heating_zone *heat) {
  http_response *resp = NULL;
  char *url = NULL;
  int i;
  int new_temp = heat->set_temp;
  char new_mode = heat->manual_mode ? 'M':'S';
  int min_temp = MIN_TEMP, max_temp = MAX_TEMP;
  char c;

  if (!strcmp(heat->id, "0")) {
    /* Domestic water */
    min_temp = WATER_MIN_TEMP;
    max_temp = WATER_MAX_TEMP;
  }

  printxcenteredbox(30, 12);
  printxcentered(7, heat->name);

  gotoxy(6, 16);
  chline(28);
  
  gotoxy(6, 17);
  printf("Up/Down   : mode ! Enter: OK");

update_mode:
  gotoxy(7, 9);
  printf("Mode: ");
  
  gotoxy(12, 9);
  revers(new_mode == 'S');
  printf("SCHEDULE");

  gotoxy(12, 10);
  revers(new_mode == 'M');
  printf("MANUAL");

  if (!strcmp(heat->id, "-1")) {
    gotoxy(12, 11);
    revers(new_mode == 'A');
    printf("AWAY");
  }

  revers(0);
  
  gotoxy(7, 13);
  printf("Temperature:");

set_temp:
  gotoxy(22, 13);
  printf("%d [C  ", new_temp);
  
  gotoxy(7, 14);
  for (i = min_temp; i < new_temp; i++) {
    cputc('-');
  }
  
  cputc('!');
  for (i = new_temp + 1; i <= max_temp; i++) {
    cputc('-');
  }

  gotoxy(6, 18);
  if (new_mode == 'M') {
    printf("Left/Right:  temp! Esc: back");
  } else {
    printf("                 ! Esc: back");
  }

  c = cgetc();
  while (c != CH_ENTER && c != CH_ESC) {
    if (c == CH_CURS_DOWN) {
      if (new_mode == 'S') {
        new_mode = 'M';
      } else if (new_mode == 'M') {
        if (!strcmp(heat->id, "-1")) {
          new_mode = 'A';
        } else {
          new_mode = 'S';
        }
      } else if (new_mode == 'A') {
        new_mode = 'S';
      }
      if (new_mode != 'M') {
        new_temp = heat->set_temp;
      }
      goto update_mode;
    }
    if (c == CH_CURS_UP) {
      if (new_mode == 'A') {
        new_mode = 'M';
      } else if (new_mode == 'M') {
        new_mode = 'S';
      } else if (new_mode == 'S') {
        if (!strcmp(heat->id, "-1")) {
          new_mode = 'A';
        } else {
          new_mode = 'M';
        }
      }
      if (new_mode != 'M') {
        new_temp = heat->set_temp;
      }
      goto update_mode;
    }
    if (new_mode == 'M') {
      if (c == CH_CURS_RIGHT && new_temp < max_temp) {
        new_temp++;
        goto set_temp;
      }
      if (c == CH_CURS_LEFT && new_temp > min_temp) {
        new_temp--;
        goto set_temp;
      }
    }
    c = cgetc();
  }

  if (c == CH_ESC)
    return 0;

  url = malloc(BUFSIZE);
  snprintf(url, BUFSIZE, HOMECONTROL_SRV"/control/toggle_tado.php?id=%s"
                                        "&state=%s&override=%s&temp=%d",
                                        heat->id, 
                                        new_mode == 'A' ? "AWAY":"HOME",
                                        new_mode == 'M' ? "MANUAL":"AUTO",
                                        new_temp);
  resp = get_url(url);

  free(url);
  http_response_free(resp);
  
  return 1;
}
