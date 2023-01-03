#include <stdlib.h>
#include <string.h>

#ifndef __switches_h
#define __switches_h

typedef struct _hc_switch hc_switch;

struct _hc_switch {
  char *id;
  char *name;
  char *state;
};

int update_switches(hc_switch ***switches_list);
void switches_free_all(void);
void toggle_switch(hc_switch *sw);
#endif
