#include <stdlib.h>
#include <string.h>
#include "slist.h"

#ifndef __switches_h
#define __switches_h

typedef struct _hc_switch hc_switch;

struct _hc_switch {
  char *id;
  char *name;
  char *state;
};

slist *switches_get(void);
slist *update_switches(void);

#endif
