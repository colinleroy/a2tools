#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "simple_serial.h"
#include "extended_conio.h"
#include "extended_string.h"
#include "slist.h"
#include "http.h"
#include "constants.h"
#include "switches.h"
#include "sensors.h"
#include "heating.h"

static unsigned char scrw, scrh;

static int cur_page = SWITCH_PAGE;
static int prev_page = -1;

static slist *switches = NULL;
static slist *sensors = NULL;
static slist *heating = NULL;

static int cur_switch_offset = 0;
static int cur_sensor_offset = 0;
static int cur_heating_offset = 0;

static void update_switch_page(void) {
  slist *w;
  int i;
  switches = update_switches();
  w = switches;
  
  for (i = 0; i < cur_switch_offset && w; i++) {
    w = w->next;
  }

  if (switches == NULL) {
    clrzone(0, 5, 39, 21);
    printxcentered(12, "Can't load data.");
    return;
  }

  clrzone(0, 5, 39, 21);
  for (i = 0; w && i < PAGE_HEIGHT; w = w->next, i++) {
    hc_switch *sw = w->data;

    gotoxy(3, 5 + i);
    printf("%s", sw->name);
    
    gotoxy(34, 5 + i);
    printf("(%3s)", sw->state);
  }
}

static void update_sensor_page(void) {
  slist *w;
  int i;
  sensors = update_sensors();
  w = sensors;

  if (sensors == NULL) {
    clrzone(0, 5, 39, 21);
    printxcentered(12, "Can't load data.");
    return;
  }

  for (i = 0; i < cur_sensor_offset && w; i++) {
    w = w->next;
  }

  clrzone(0, 5, 39, 21);

  for (i = 0; w && i < PAGE_HEIGHT; w = w->next, i++) {
    hc_sensor *sensor = w->data;

    gotoxy(3, 5 + i);
    printf("%s\n", sensor->name);
  }
}

static void update_heating_page(void) {
  slist *w;
  int i;
  heating = update_heating_zones();
  w = heating;
  
  if (heating == NULL) {
    clrzone(0, 5, 39, 21);
    printxcentered(12, "Can't load data.");
    return;
  }
  for (i = 0; i < cur_heating_offset && w; i++) {
    w = w->next;
  }

  clrzone(0, 5, 39, 21);

  for (i = 0; w && i < PAGE_HEIGHT; w = w->next, i++) {
    hc_heating *heat = w->data;

    gotoxy(3, 5 + i);
    printf("%s", heat->name);
    
    gotoxy(24, 5 + i);
    printf("%4s/%2s[C %2s%%H", heat->cur_temp, heat->set_temp, heat->cur_humidity);

  }
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
    clrzone(0, 5, 39, 21);
    printxcentered(12, "Loading...");
    prev_page = cur_page;
  }
  switch(cur_page) {
    case SWITCH_PAGE:  update_switch_page();  break;
    case SENSOR_PAGE:  update_sensor_page();  break;
    case HEATING_PAGE: update_heating_page(); break;
  }

  while (!kbhit()) {
    refresh_counter--;
    if (refresh_counter == 0) {
      
      goto update;
    }
  }
command:
  command = cgetc();
  switch(command) {
    case '1': cur_page = SWITCH_PAGE; goto update;
    case '2': cur_page = SENSOR_PAGE; goto update;
    case '3': cur_page = HEATING_PAGE; goto update;
    case '4': clrzone(0, 5, 39, 21); goto command;
  }

  exit(0);
}
