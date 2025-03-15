#ifndef __apple2_mem_h
#define __apple2_mem_h

#include "platform.h"

#ifdef __APPLE2__

extern uint8 *WNDLFT;
//#define WNDLFT  0x20
#define WNDWDTH 0x21
#define WNDTOP  0x22
#define WNDBTM  0x23
#define CH      0x24
#define CV      0x25
#define OURCH   0x057B
#define OURCV   0X05FB
#endif

#endif
