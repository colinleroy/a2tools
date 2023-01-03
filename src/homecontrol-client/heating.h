#include <stdlib.h>
#include <string.h>

#ifndef __heating_h
#define __heating_h

typedef struct _hc_heating_zone hc_heating_zone;

struct _hc_heating_zone {
  char *id;
  char *name;
  int set_temp;
  char *cur_temp;
  char *cur_humidity;
  char manual_mode;
};

int update_heating_zones(hc_heating_zone ***zones_list);
void heating_zones_free_all(void);
int configure_heating_zone(hc_heating_zone *heat);

int climate_can_schedule(void);
int climate_can_set_away(void);
#endif
