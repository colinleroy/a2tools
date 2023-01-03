#include <stdlib.h>
#include <string.h>

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

int update_sensors(hc_sensor ***sensors_list);
void sensors_free_all(void);
#endif
