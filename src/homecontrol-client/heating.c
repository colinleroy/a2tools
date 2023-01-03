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

#include "constants.h"
#include "network.h"
#include "heating.h"
#include "server_url.h"

static hc_heating_zone **heating_zones = NULL;
static int num_heating_zones = 0;

static char *do_round(char *floatval, int num) {
  char *result = strdup(floatval);
  char *decdot = strchr(result, '.');
  int offset = (num > 0) ? (num + 1) : 0;

  if (decdot && strlen(decdot) > offset) {
    *(decdot + offset) = '\0';
  }
  
  return result;
}

static char *avg_vals(char *in) {
  char **min_max, *out = NULL;
  int num_parts;
  num_parts = strsplit_in_place(in, ' ', &min_max);

  if (num_parts == 3) {
    int avg = (atoi(min_max[2]) + atoi(min_max[0])) / 2;
    out = malloc(4);
    sprintf(out, "%d", avg);
  } else {
    out = strdup("NA");
  }
  free(min_max);

  return out;
}
static void heating_add(char *id, char *name, char *set_temp, char *cur_temp, char *cur_humidity, char manual_mode) {
  hc_heating_zone *heat = malloc(sizeof(hc_heating_zone));
  memset(heat, 0, sizeof(hc_heating_zone));

  heat->id           = strdup(id);
  heat->set_temp     = atoi(set_temp);
  heat->manual_mode  = manual_mode;
  heat->name         = strndup_ellipsis(name, 22);

  if (!strcmp(heat->id, "-1")) {
    heat->set_temp   = 21;
  }

  if (strcmp(cur_temp, "n/a")) {
    if (strstr(cur_temp, " - ")) {
      heat->cur_temp = avg_vals(cur_temp);
    } else {
      heat->cur_temp = do_round(cur_temp, 0);
    }
  } else {
    heat->cur_temp     = strdup("NA");
  }
  if (strcmp(cur_humidity, "n/a")) {
    if (strstr(cur_humidity, " - ")) {
      heat->cur_humidity = avg_vals(cur_humidity);
    } else {
      heat->cur_humidity = do_round(cur_humidity, 0);
    }
  } else {
    heat->cur_humidity = strdup("NA");
  }

  heating_zones[num_heating_zones++] = heat;
}

static void heating_free(hc_heating_zone *heat) {
  free(heat->id);
  free(heat->name);
  free(heat->cur_temp);
  free(heat->cur_humidity);
  free(heat);
}

void heating_zones_free_all(void) {
  int i;
  for (i = 0; i < num_heating_zones; i++) {
    heating_free(heating_zones[i]);
  }
  free(heating_zones);
  heating_zones = NULL;
  num_heating_zones = 0;
}

static int can_set_away = 0;
static int can_schedule = 0;
static char *full_home_zone = NULL;
static char *hot_water_zone = NULL;

int climate_can_schedule(void) {
  return can_schedule;
}

int climate_can_set_away(void) {
  return can_set_away;
}

int update_heating_zones(hc_heating_zone ***heating_zones_list) {
  http_response *resp;
  char **lines = NULL;
  int i, num_lines;
  char *url;

  heating_zones_free_all();

  url = malloc(BUFSIZE);
  snprintf(url, BUFSIZE, "%s/climate.php", get_server_root_url());
  resp = get_url(url);
  free(url);

  if (resp == NULL || resp->size == 0 || resp->code != 200) {
    http_response_free(resp);
    *heating_zones_list = NULL;
    return 0;
  }

  num_lines = strsplit_in_place(resp->body, '\n', &lines);

  if (num_lines >= 3) {
    for (i = 0; i < 3; i++) {
      if(!strncmp(lines[i], "CAPS;", 5)) {
        can_set_away = strstr(lines[i],"CAN_AWAY") != NULL;
        can_schedule = strstr(lines[i],"CAN_SCHEDULE") != NULL;
      } else if(!strncmp(lines[i], "FULL_HOME_ZONE;", 15)) {
        free(full_home_zone);
        full_home_zone = strdup(strchr(lines[i], ';') + 1);
      } else if(!strncmp(lines[i], "HOT_WATER_ZONE;", 15)) {
        free(hot_water_zone);
        hot_water_zone = strdup(strchr(lines[i], ';') + 1);
      }
    }
  }

  heating_zones = malloc((num_lines - 3) * sizeof(hc_heating_zone *));
  for (i = 3; i < num_lines; i++) {
    char **parts;
    int num_parts;
    num_parts = strsplit_in_place(lines[i],';', &parts);
    if (num_parts == 5 + can_schedule) {
      heating_add(parts[0], parts[1], parts[2], parts[3], parts[4], can_schedule ? !strcmp(parts[5], "MANUAL") : 1);
    }
    free(parts);
  }

  http_response_free(resp);
  free(lines);
  *heating_zones_list = heating_zones;
  return num_heating_zones;
}


int configure_heating_zone(hc_heating_zone *heat) {
  http_response *resp = NULL;
  char *url = NULL;
  int i;
  int new_temp = heat->set_temp;
  char new_mode = heat->manual_mode ? 'M':'S';
  int min_temp = MIN_TEMP, max_temp = MAX_TEMP;
  char c;

  if (!strcmp(heat->id, hot_water_zone)) {
    /* Domestic water */
    min_temp = WATER_MIN_TEMP;
    max_temp = WATER_MAX_TEMP;
  }

  printxcenteredbox(30, 12);
  printxcentered(7, heat->name);

  gotoxy(6, 16);
  chline(28);
  
update_mode:
  gotoxy(6, 17);
  if (can_schedule) {
    puts("Up/Down   : mode ! Enter: OK");

    gotoxy(7, 9);
    puts("Mode: ");
    
    gotoxy(12, 9);
    revers(new_mode == 'S');
    puts("SCHEDULE");

    gotoxy(12, 10);
    revers(new_mode == 'M');
    puts("MANUAL");

    if (can_set_away && !strcmp(heat->id, full_home_zone)) {
      gotoxy(12, 11);
      revers(new_mode == 'A');
      puts("AWAY");
    }

    revers(0);
    
  } else {
    puts("                 ! Enter: OK");
  }

  gotoxy(7, 13);
  puts("Temperature:");

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
    puts("Left/Right:  temp! Esc: back");
  } else {
    puts("                 ! Esc: back");
  }

  c = cgetc();
  while (c != CH_ENTER && c != CH_ESC) {
    if (c == CH_CURS_DOWN) {
      if (new_mode == 'S') {
        new_mode = 'M';
      } else if (new_mode == 'M') {
        if (can_set_away && !strcmp(heat->id, full_home_zone)) {
          new_mode = 'A';
        } else if (can_schedule) {
          new_mode = 'S';
        }
      } else if (new_mode == 'A') {
        new_mode = can_schedule ? 'S':'M';
      }
      if (new_mode != 'M') {
        new_temp = heat->set_temp;
      }
      goto update_mode;
    }
    if (c == CH_CURS_UP) {
      if (new_mode == 'A') {
        new_mode = 'M';
      } else if (can_schedule && new_mode == 'M') {
        new_mode = 'S';
      } else if (new_mode == 'S') {
        if (can_set_away && !strcmp(heat->id, full_home_zone)) {
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
  snprintf(url, BUFSIZE, "%s/climate_ctrl.php?id=%s"
                            "&presence=%s&mode=%s&set_temp=%d",
                            get_server_root_url(),
                            heat->id, 
                            new_mode == 'A' ? "AWAY":"HOME",
                            new_mode == 'M' ? "MANUAL":"AUTO",
                            new_temp);
  resp = get_url(url);

  free(url);
  http_response_free(resp);
  
  return 1;
}
