#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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
static slist *heating = NULL;

static int cur_list_offset = -1;
static int cur_list_display_offset = 0;
static int cur_list_length = -1;

#define clear_list() do { clrzone(3, PAGE_BEGIN, 39, PAGE_BEGIN + PAGE_HEIGHT + 1); } while(0)
static void update_switch_page(int update_data) {
  slist *w;
  int i;

  if (update_data) {
    switches = update_switches();
  }

  w = switches;
  
  for (i = 0; i < cur_list_display_offset && w; i++) {
    w = w->next;
  }

  if (switches == NULL) {
    clear_list();
    printxcentered(12, "Can't load data.");
    return;
  }

  clear_list();
  for (i = 0; w && i < PAGE_HEIGHT; w = w->next, i++) {
    hc_switch *sw = w->data;

    gotoxy(3, PAGE_BEGIN + i);
    printf("%s", sw->name);
    
    gotoxy(34, PAGE_BEGIN + i);
    printf("(%3s)", sw->state);
  }
  
  cur_list_length = slist_length(switches);
}

static void update_sensor_page(int update_data) {
  slist *w;
  int i;

  if (update_data) {
    sensors = update_sensors();
  }

  w = sensors;

  if (sensors == NULL) {
    clear_list();
    printxcentered(12, "Can't load data.");
    return;
  }

  for (i = 0; i < cur_list_display_offset && w; i++) {
    w = w->next;
  }

  clear_list();

  for (i = 0; w && i < PAGE_HEIGHT; w = w->next, i++) {
    hc_sensor *sensor = w->data;

    gotoxy(3, PAGE_BEGIN + i);
    printf("%s\n", sensor->name);
  }
  
  cur_list_length = slist_length(sensors);
}

static void update_heating_page(int update_data) {
  slist *w;
  int i;

  if (update_data) {
    heating = update_heating_zones();
  }

  w = heating;
  
  if (heating == NULL) {
    clear_list();
    printxcentered(12, "Can't load data.");
    return;
  }
  for (i = 0; i < cur_list_display_offset && w; i++) {
    w = w->next;
  }

  clear_list();

  for (i = 0; w && i < PAGE_HEIGHT; w = w->next, i++) {
    hc_heating *heat = w->data;

    gotoxy(3, PAGE_BEGIN + i);
    printf("%s", heat->name);
    
    gotoxy(24, PAGE_BEGIN + i);
    printf("%4s/%2s[C %2s%%H", heat->cur_temp, heat->set_temp, heat->cur_humidity);
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

  gotoxy(30,0);
  printf("%d/%d/%d ", cur_list_offset, cur_list_display_offset, cur_list_length);

  if (scroll_changed) {
    switch(cur_page) {
      case SWITCH_PAGE:  update_switch_page(0);  break;
      case SENSOR_PAGE:  update_sensor_page(0);  break;
      case HEATING_PAGE: update_heating_page(0); break;
    }
  }

  clrzone(1, PAGE_BEGIN, 1, PAGE_BEGIN + PAGE_HEIGHT + 1);
  gotoxy(1, PAGE_BEGIN + cur_list_offset - cur_list_display_offset);
  cputc('>');
}

static void select_switch(void) {
  int i;
  slist *w = switches;
  printxcenteredbox(12, "Toggling...");

  for (i = 0; i < cur_list_offset; i++) {
    w = w->next;
  }

  toggle_switch(w->data);

  for (i = 0; i < 8000; i++); /* wait long enough */
}

static void select_item(void) {
  switch(cur_page) {
    case SWITCH_PAGE: select_switch();  break;
    case SENSOR_PAGE:  /* TODO */;  break;
    case HEATING_PAGE: /* TODO */; break;
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
    clear_list();
    printxcentered(12, "Loading...");
    cur_list_offset = -1;
    cur_list_display_offset = 0;
    prev_page = cur_page;
  }
  switch(cur_page) {
    case SWITCH_PAGE:  update_switch_page(1);  break;
    case SENSOR_PAGE:  update_sensor_page(1);  break;
    case HEATING_PAGE: update_heating_page(1); break;
  }

#ifdef __CC65__
  gotoxy(0,0);
  printf("%d  ", _heapmaxavail());
#endif

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
    case CH_ENTER: select_item(); goto update;
    case CH_CURS_UP: update_offset(-1); goto command;
    case CH_CURS_DOWN: update_offset(+1); goto command;
    default: goto command;
  }

  exit(0);
}