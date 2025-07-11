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
#include "platform.h"
#include "simple_serial.h"
#include "clrzone.h"
#include "extended_conio.h"
#include "strsplit.h"
#include "math.h"
#include "surl.h"
#include "constants.h"
#include "switches.h"
#include "sensors.h"
#include "climate.h"
#include "server_url.h"
#include "scroll.h"
#include "scrollwindow.h"
#include "dputc.h"
#include "dputs.h"
#include "a2_features.h"
#include <time.h>

static unsigned char scrw, scrh;

static int cur_page = SWITCH_PAGE;
static int prev_page = -1;

static hc_switch **switches = NULL;
static hc_sensor **sensors = NULL;
static hc_climate_zone **climate_zones = NULL;

static int cur_list_offset = 0;
static int cur_list_display_offset = 0;
static int cur_list_length = -1;

#define clear_list(x) do { clrzone(x, PAGE_BEGIN, scrw - 1, PAGE_BEGIN + PAGE_HEIGHT); } while(0)
static void update_switch_page(char update_data, char clear) {
  int i;

  if (update_data) {
    cur_list_length = update_switches(&switches);
  }

  if (switches == NULL) {
    clear_list(0);
    printxcentered(12, "Can't load data.");
    return;
  }

  if (clear)
    clear_list(3);

  for (i = 0; i + cur_list_display_offset < cur_list_length && i < PAGE_HEIGHT; i++) {
    hc_switch *sw = switches[i + cur_list_display_offset];

    gotoxy(3, PAGE_BEGIN + i);
    dputs(sw->name);
    
    gotoxy(30, PAGE_BEGIN + i);

    if (!strcmp(sw->state, "on")) {
      revers(1);
      cputs("( ON)");
      revers(0);
    } else {
      cputs("(OFF)");
    }
  }
}

static void update_sensor_page(char update_data, char clear) {
  int i;

  if (update_data) {
    cur_list_length = update_sensors(&sensors);
  }

  if (sensors == NULL) {
    clear_list(0);
    printxcentered(12, "Can't load data.");
    return;
  }

  if (clear)
    clear_list(3);

  for (i = 0; i + cur_list_display_offset < cur_list_length && i < PAGE_HEIGHT; i++) {
    hc_sensor *sensor = sensors[i + cur_list_display_offset];

    gotoxy(3, PAGE_BEGIN + i);
    dputs(sensor->name);

    gotoxy(30, PAGE_BEGIN + i);
    cprintf("%ld ", sensor->cur_value);
    dputs(sensor->unit);
  }
}

static void update_climate_page(char update_data, char clear) {
  int i;

  if (update_data) {
    cur_list_length = update_climate_zones(&climate_zones);
  }

  if (climate_zones == NULL) {
    clear_list(0);
    printxcentered(12, "Can't load data.");
    return;
  }

  if (clear)
    clear_list(3);

  for (i = 0; i + cur_list_display_offset < cur_list_length && i < PAGE_HEIGHT; i++) {
    hc_climate_zone *zone = climate_zones[i + cur_list_display_offset];

    gotoxy(3, PAGE_BEGIN + i);
    dputs(zone->name);
    
    gotoxy(24, PAGE_BEGIN + i);
    cprintf("%4s/%2d[C %2s%%H", zone->cur_temp, zone->set_temp, zone->cur_humidity);
  }
}

static void print_header(void) {
  printxcentered(0, "HOME CONTROL");
  printxcentered(1, "1. Switches ! 2. Sensors ! 3. Climate");
  gotoxy(0, 2);
  chline(scrw);
}

static void print_footer(void) {
  gotoxy(0, 22);
  chline(scrw);
  printxcentered(23, "Up/Down: Choose ! Enter: Select");
}

static void update_offset(signed char new_offset) {
  signed char scroll_changed = 0, scroll_way = 0;
  char rollover = 0;

  if (new_offset < 0) {
    if (cur_list_offset > 0) {
      cur_list_offset--;
      scroll_way = -1;
    } else {
      rollover = 1;
      cur_list_offset = cur_list_length - 1;
      scroll_way = +1;
    }
  } else if (new_offset > 0) {
    if (cur_list_offset < cur_list_length - 1) {
      cur_list_offset++;
      scroll_way = +1;
    } else {
      rollover = 1;
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
    if (!rollover) {
      set_scrollwindow(PAGE_BEGIN, PAGE_BEGIN + PAGE_HEIGHT);
      scrolldown_one();
      set_scrollwindow(0, 24);
    }
  }
  if (scroll_way > 0 && cur_list_offset > cur_list_display_offset + PAGE_HEIGHT - 1) {
    cur_list_display_offset = cur_list_offset - PAGE_HEIGHT + 1;
    scroll_changed = 1;
    if (!rollover) {
      set_scrollwindow(PAGE_BEGIN, PAGE_BEGIN + PAGE_HEIGHT);
      scrollup_one();
      set_scrollwindow(0, 24);
    }
  }

  if (scroll_changed) {
    switch(cur_page) {
      case SWITCH_PAGE:  update_switch_page(0, rollover);  break;
      case SENSOR_PAGE:  update_sensor_page(0, rollover);  break;
      case CLIMATE_PAGE: update_climate_page(0, rollover); break;
    }
  }

  clrzone(1, PAGE_BEGIN, 1, PAGE_BEGIN + PAGE_HEIGHT);
  gotoxy(1, PAGE_BEGIN + cur_list_offset - cur_list_display_offset);
  dputc('>');
}

static void select_switch(void) {
  hc_switch *sw;

  if (cur_list_offset < 0 || cur_list_offset >= cur_list_length)
    return;

  sw = switches[cur_list_offset];
  toggle_switch(sw);
}

static void select_sensor(void) {
  hc_sensor *sensor;
  char *params = NULL;

  if (cur_list_offset < 0 || cur_list_offset >= cur_list_length)
    return;

  sensor = sensors[cur_list_offset];

  params = malloc(BUFSIZE);
  snprintf(params, BUFSIZE, "\"%s\" \"%s\" %d \"%s\"",
                         sensor->id, sensor->name, sensor->scale, sensor->unit);
#ifdef __CC65__
  exec("GRPHVIEW", params);
  free(params);
#else
  /* TODO */
  printf("exec GRPHVIEW %s", params);
  free(params);
  exit(0);
#endif
}

static int select_climate_zone(void) {
  hc_climate_zone *zone;

  if (cur_list_offset < 0 || cur_list_offset >= cur_list_length)
    return 0;

  zone = climate_zones[cur_list_offset];

  if (!configure_climate_zone(zone)) {
    /* Redraw on config screen */
    update_climate_page(0, 1);
    return 0;
  }
  return 1;
}

static int select_item(void) {
  switch(cur_page) {
    case SWITCH_PAGE: select_switch(); return 1;
    case SENSOR_PAGE:  select_sensor();  return 1;
    case CLIMATE_PAGE: return select_climate_zone();
  }
  return 1;
}

static void cleanup(void) {
  surl_disconnect_proxy();
  switches_free_all();
  sensors_free_all();
  climate_zones_free_all();
  printf("Cleaned up.\n");
}

static long refresh_counter = REFRESH_DELAY;
#ifndef __CC65__
static time_t last_update = 0;
#endif

int main(int argc, char **argv) {
  char command;

  clrscr();
  screensize(&scrw, &scrh);
  surl_ping();
  surl_user_agent = "HomeControl "VERSION"/Apple II";
  print_header();
  
  /* init if needed */
  get_server_root_url();
  
  print_footer();

update:
  refresh_counter = REFRESH_DELAY;
#ifndef __CC65__
  last_update = time(NULL);
#endif
  if (cur_page != prev_page) {
    clear_list(0);
    printxcentered(12, "Loading...");
    cur_list_offset = -1;
    cur_list_display_offset = 0;
    prev_page = cur_page;
  }
  
  if (argc > 1) {
    cur_page = atoi(argv[1]);
    argc = 0; /* Only first time */
  }

  switches_free_all();
  sensors_free_all();
  climate_zones_free_all();
  cur_list_length = 0;
  switch(cur_page) {
    case SWITCH_PAGE:  update_switch_page(1, 1);  break;
    case SENSOR_PAGE:  update_sensor_page(1, 1);  break;
    case CLIMATE_PAGE: update_climate_page(1, 1); break;
  }
  if (cur_list_length > 0 && cur_list_offset == -1) {
    update_offset(+1);
  }

command:
  while (!kbhit()) {
#ifndef __CC65__
    if (time(NULL) - last_update > REFRESH_DELAY/1000)
      goto update;
#else
    refresh_counter--;
    if (refresh_counter == 0) {
      
      goto update;
    }
#endif
  }

  command = cgetc();

  switch(command) {
    case '1': cur_page = SWITCH_PAGE; goto update;
    case '2': cur_page = SENSOR_PAGE; goto update;
    case '3': cur_page = CLIMATE_PAGE; goto update;
    case CH_ENTER:
      if (select_item())
        goto update;
      else
        goto command;
    case CH_CURS_UP: 
      update_offset(-1);
      goto command;
    case CH_CURS_DOWN: 
      update_offset(+1);
      goto command;
    default: 
      goto command;
  }
  /* coverity[unreachable] */
  cleanup();
  exit(0);
}
