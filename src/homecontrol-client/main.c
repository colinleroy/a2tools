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
#include <unistd.h>
#include "simple_serial.h"
#include "extended_conio.h"
#include "extended_string.h"
#include "math.h"
#include "slist.h"
#include "http.h"
#include "constants.h"
#include "switches.h"
#include "sensors.h"
#include "heating.h"
#ifdef __CC65__
#include <apple2enh.h>
#endif

static unsigned char scrw, scrh;

static int cur_page = SWITCH_PAGE;
static int prev_page = -1;

static slist *switches = NULL;
static slist *sensors = NULL;
static slist *sensor_metrics = NULL;
static slist *heating = NULL;

static char *cur_sensor = NULL;
static int cur_list_offset = -1;
static int cur_list_display_offset = 0;
static int cur_list_length = -1;

#define clear_list(x) do { clrzone(x, PAGE_BEGIN, 39, PAGE_BEGIN + PAGE_HEIGHT + 1); } while(0)
static void update_switch_page(int update_data) {
  slist *w;
  int i;

  if (update_data) {
    switches = update_switches();
  }

  if (switches == NULL) {
    clear_list(0);
    printxcentered(12, "Can't load data.");
    return;
  }

  w = switches;
  
  for (i = 0; i < cur_list_display_offset && w; i++) {
    w = w->next;
  }

  clear_list(3);
  for (i = 0; w && i < PAGE_HEIGHT; w = w->next, i++) {
    hc_switch *sw = w->data;

    gotoxy(3, PAGE_BEGIN + i);
    printf("%s", sw->name);
    
    gotoxy(34, PAGE_BEGIN + i);

    if (!strcmp(sw->state, "on")) {
      revers(1);
      printf("( ON)");
      revers(0);
    } else {
      printf("(OFF)");
    }
  }
  
  cur_list_length = slist_length(switches);
}

static void update_sensor_page(int update_data) {
  slist *w;
  int i;

  if (update_data) {
    sensors = update_sensors();
  }

  if (sensors == NULL) {
    clear_list(0);
    printxcentered(12, "Can't load data.");
    return;
  }

  w = sensors;

  for (i = 0; i < cur_list_display_offset && w; i++) {
    w = w->next;
  }

  clear_list(3);

  for (i = 0; w && i < PAGE_HEIGHT; w = w->next, i++) {
    hc_sensor *sensor = w->data;

    gotoxy(3, PAGE_BEGIN + i);
    printf("%s\n", sensor->name);
  }
  
  cur_list_length = slist_length(sensors);
}

static void update_sensor_metric_page(int update_data, char *sensor_id) {
  slist *w;
  int i;

  if (sensor_id == NULL) {
    return;
  }

  if (update_data) {
    sensor_metrics = update_sensor_metrics(sensor_id);
  }

  if (sensor_metrics == NULL) {
    clear_list(0);
    printxcentered(12, "Can't load data.");
    return;
  }

  w = sensor_metrics;

  for (i = 0; i < cur_list_display_offset && w; i++) {
    w = w->next;
  }

  clear_list(3);

  for (i = 0; w && i < PAGE_HEIGHT; w = w->next, i++) {
    hc_sensor_metric *metric = w->data;

    gotoxy(3, PAGE_BEGIN + i);
    printf("%s (%d day%s, %s)\n", metric->name, metric->scale, 
      metric->scale > 1 ? "s":"", metric->unit);
  }
  
  cur_list_length = slist_length(sensor_metrics);
}

static void update_heating_page(int update_data) {
  slist *w;
  int i;

  if (update_data) {
    heating = update_heating_zones();
  }

  if (heating == NULL) {
    clear_list(0);
    printxcentered(12, "Can't load data.");
    return;
  }

  w = heating;
  
  for (i = 0; i < cur_list_display_offset && w; i++) {
    w = w->next;
  }

  clear_list(3);
  for (i = 0; w && i < PAGE_HEIGHT; w = w->next, i++) {
    hc_heating_zone *heat = w->data;

    gotoxy(3, PAGE_BEGIN + i);
    printf("%s", heat->name);
    
    gotoxy(24, PAGE_BEGIN + i);
    printf("%4s/%2d[C %2s%%H", heat->cur_temp, heat->set_temp, heat->cur_humidity);
  }
  
  cur_list_length = slist_length(heating);
}

static void print_header(void) {
  printxcentered(0, "HOME CONTROL");
  printxcentered(1, "1. Switches ! 2. Sensors ! 3. Heating");
  gotoxy(0, 2);
  chline(scrw);
}

static void print_footer(void) {
  gotoxy(0, 22);
  chline(scrw);
  printxcentered(23, "Up/Down: Choose ! Enter: Select");
}

static void update_offset(int new_offset) {
  int scroll_changed = 0, scroll_way = 0;
  /* Handle list offset */
  if (new_offset < 0) {
    if (cur_list_offset > 0) {
      cur_list_offset--;
      scroll_way = -1;
    } else {
      cur_list_offset = cur_list_length - 1;
      scroll_way = +1;
    }
  } else if (new_offset > 0) {
    if (cur_list_offset < cur_list_length - 1) {
      cur_list_offset++;
      scroll_way = +1;
    } else {
      cur_list_offset = 0;
      scroll_way = -1;
    }
  } else {
    return;
  }

  /* Handle scroll */
  if (scroll_way < 0 && cur_list_offset < cur_list_display_offset) {
    cur_list_display_offset = cur_list_offset;
    scroll_changed = 1;
  }
  if (scroll_way > 0 && cur_list_offset > cur_list_display_offset + PAGE_HEIGHT - 1) {
    cur_list_display_offset = cur_list_offset - PAGE_HEIGHT + 1;
    scroll_changed = 1;
  }

  // gotoxy(30,0);
  // printf("%d/%d/%d ", cur_list_offset, cur_list_display_offset, cur_list_length);

  if (scroll_changed) {
    switch(cur_page) {
      case SWITCH_PAGE:  update_switch_page(0);  break;
      case SENSOR_PAGE:  update_sensor_page(0);  break;
      case HEATING_PAGE: update_heating_page(0); break;
      case METRIC_PAGE:  update_sensor_metric_page(0, cur_sensor); break;
    }
  }

  clrzone(1, PAGE_BEGIN, 1, PAGE_BEGIN + PAGE_HEIGHT + 1);
  gotoxy(1, PAGE_BEGIN + cur_list_offset - cur_list_display_offset);
  cputc('>');
}

static void select_switch(void) {
  int i;
  slist *w = switches;

  for (i = 0; i < cur_list_offset; i++) {
    w = w->next;
  }

  toggle_switch(w->data);
}

static void select_sensor(void) {
  int i;
  slist *w = sensors;
  hc_sensor *sensor;

  for (i = 0; i < cur_list_offset; i++) {
    w = w->next;
  }

  sensor = w->data;
  cur_sensor = strdup(sensor->id);
  cur_page = METRIC_PAGE;
}

static void select_sensor_metric(void) {
  int i;
  slist *w = sensor_metrics;
  hc_sensor_metric *metric;
  char *params = NULL;

  for (i = 0; i < cur_list_offset; i++) {
    w = w->next;
  }

  metric = w->data;

  params = malloc(BUFSIZE);
  snprintf(params, BUFSIZE, "%s %s %d %s", metric->sensor_id, metric->id, metric->scale, metric->unit);
#ifdef __CC65__
  exec("MTRCFTCH", params);
  free(params);
#else
  /* TODO */
  printf("exec MTRCFTCH %s", params);
#endif
}

static int select_heating_zone(void) {
  int i;
  slist *w = heating;
  
  for (i = 0; i < cur_list_offset; i++) {
    w = w->next;
  }
  
  if (!configure_heating_zone(w->data)) {
    /* Redraw on config screen */
    update_heating_page(0);
    return 0;
  }
  return 1;
}

static int select_item(void) {
  switch(cur_page) {
    case SWITCH_PAGE: select_switch(); return 1;
    case SENSOR_PAGE:  select_sensor();  return 1;
    case HEATING_PAGE: return select_heating_zone();
    case METRIC_PAGE: select_sensor_metric(); break;
  }
}

static long refresh_counter = REFRESH_DELAY;
int main(int argc, char **argv) {
  char command;

  http_connect_proxy();

  clrscr();
  screensize(&scrw, &scrh);
  
  print_header();
  print_footer();

update:
  refresh_counter = REFRESH_DELAY;
  if (cur_page != prev_page) {
    clear_list(0);
    printxcentered(12, "Loading...");
    cur_list_offset = -1;
    cur_list_display_offset = 0;
    prev_page = cur_page;
    if (cur_page != METRIC_PAGE) {
      free(cur_sensor);
      cur_sensor = NULL;
    }
  }
  
  if (argc > 1) {
    cur_page = atoi(argv[1]);
    if (cur_page == SENSOR_PAGE && argc > 2) {
      cur_page = METRIC_PAGE;
      cur_sensor = strdup(argv[2]);
    }
    argc = 0; /* Only first time */
  }

  switch(cur_page) {
    case SWITCH_PAGE:  update_switch_page(1);  break;
    case SENSOR_PAGE:  update_sensor_page(1);  break;
    case HEATING_PAGE: update_heating_page(1); break;
    case METRIC_PAGE:  update_sensor_metric_page(1, cur_sensor); break;
  }

// #ifdef __CC65__
//   gotoxy(0,0);
//   printf("%d  ", _heapmaxavail());
// #endif

command:
  while (!kbhit()) {
    refresh_counter--;
    if (refresh_counter == 0) {
      
      goto update;
    }
  }

  command = cgetc();

  switch(command) {
    case '1': cur_page = SWITCH_PAGE; goto update;
    case '2': cur_page = SENSOR_PAGE; goto update;
    case '3': cur_page = HEATING_PAGE; goto update;
    case CH_ENTER:
    case 'e':
      if (select_item())
        goto update;
      else
        goto command;
    case CH_ESC:
    case 'z':
      if (cur_page == METRIC_PAGE) {
        cur_page = SENSOR_PAGE;
        goto update;
      }
      goto command;
    case CH_CURS_UP: 
    case 'i':
      update_offset(-1);
      goto command;
    case CH_CURS_DOWN: 
    case 'k':
      update_offset(+1);
      goto command;
    default: 
      goto command;
  }

  exit(0);
}
