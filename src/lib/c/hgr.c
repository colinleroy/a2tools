#include "hgr.h"

char hgr_init_done = 0;
char hgr_mix_is_on = 0;

#ifndef __CC65__
#include "tgi_compat.h"

void init_hgr(uint8 mono) {
  tgi_init();
}
void init_text(void) {
  tgi_done();
}

#endif
