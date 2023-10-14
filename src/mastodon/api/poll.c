#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "surl.h"
#include "simple_serial.h"
#include "strsplit.h"
#include "api.h"

poll *poll_new(void) {
  poll *p = malloc(sizeof(poll));
  if (p == NULL) {
    return NULL;
  }
  memset(p, 0, sizeof(poll));
  return p;
}

void poll_free(poll *p) {
  char i;
  if (p == NULL)
    return;

  free(p->id);
  for (i = 0; i < p->options_count; p++) {
    free(p->options[i]->title);
    free(p->options[i]);
  }
  free(p->options);
  free(p);
}
