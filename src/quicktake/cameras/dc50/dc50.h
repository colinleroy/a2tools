#ifndef _DC50_H
#define _DC50_H

#include "platform.h"

extern uint8 scrw, scrh;
extern uint8 do_debug;

#define CMD_SET_SPEED   0x41
#define CMD_GET_STATUS  0x7f

#define REP_COMPLETE    0x00
#define REP_ACK         0xd1
#define REP_CORRECT     0xd2
#define REP_NACK        0xe1
#define REP_ILLEGAL     0xe3
#define REP_BUSY        0xf0

#endif
