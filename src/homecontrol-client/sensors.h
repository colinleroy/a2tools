#include <stdlib.h>
#include <string.h>
#include "slist.h"

#ifndef __sensors_h
#define __sensors_h

typedef struct _hc_sensor hc_sensor;

struct _hc_sensor {
  char *id;
  char *name;
  char scale;
  long cur_value;
  char *unit;
};

slist *sensors_get(void);
slist *update_sensors(void);
void sensors_free_all(void);
#endif
