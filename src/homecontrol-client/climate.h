#include <stdlib.h>
#include <string.h>

#ifndef __climate_h
#define __climate_h

typedef struct _hc_climate_zone hc_climate_zone;

struct _hc_climate_zone {
  char *id;
  char *name;
  int set_temp;
  char *cur_temp;
  char *cur_humidity;
  char manual_mode;
};

int update_climate_zones(hc_climate_zone ***zones_list);
void climate_zones_free_all(void);
int configure_climate_zone(hc_climate_zone *heat);

int climate_can_schedule(void);
int climate_can_set_away(void);
#endif
