#ifndef __HGR_ADDRS_H__
#define __HGR_ADDRS_H__

#include "platform.h"
#include "hgr.h"

extern uint8 hgr_baseaddr_l[HGR_HEIGHT];
extern uint8 hgr_baseaddr_h[HGR_HEIGHT];
extern uint8 div7_table[HGR_WIDTH];
extern uint8 mod7_table[HGR_WIDTH];
extern uint8 centered_div7_table[256];
extern uint8 centered_mod7_table[256];

void init_hgr_base_addrs (void);

#endif
