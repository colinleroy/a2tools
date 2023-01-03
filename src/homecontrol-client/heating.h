#include <stdlib.h>
#include <string.h>
#include "slist.h"

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

slist *heating_zones_get(void);
slist *update_heating_zones(void);
void heating_zones_free_all(void);
int configure_heating_zone(hc_heating_zone *heat);

#endif
