#include <stdlib.h>
#include <string.h>
#include "slist.h"

#ifndef __sensors_h
#define __sensors_h

typedef struct _hc_sensor hc_sensor;

struct _hc_sensor {
  char *id;
  char *name;
};

typedef struct _hc_sensor_metric hc_sensor_metric;

struct _hc_sensor_metric {
  char *sensor_id;
  char *id;
  char *name;
  int scale;
  char *unit;
};

slist *sensors_get(void);
slist *update_sensors(void);
slist *update_sensor_metrics(const char *id);
void sensors_free_all(void);
#endif
