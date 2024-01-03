#ifndef __HGR_ADDRS_H__
#define __HGR_ADDRS_H__

#include "platform.h"
#include "hgr.h"

extern uint8 *hgr_baseaddr[HGR_HEIGHT];
extern uint8 div7_table[HGR_WIDTH];
extern uint8 mod7_table[HGR_WIDTH];

void init_hgr_base_addrs (void);

#endif
