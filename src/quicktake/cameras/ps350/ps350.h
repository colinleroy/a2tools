#ifndef _SIERRA_H
#define _SIERRA_H

#include "platform.h"

extern uint8 scrw, scrh;
extern uint8 do_debug;

#define PS350_PKT_LEN 300

#define PS350_CNT_IDX 1
#define PS350_CMD_IDX 2
#define PS350_CHK_IDX 297

#define INIT_REPLY_NAME_IDX 29


#define CMD_EOT  0x03
#define CMD_ACK  0x04
#define CMD_PING 0x10
#define CMD_SPD  0x42

#define PS350_SPEED_9600   0x00
#define PS350_SPEED_19200  0x01
#define PS350_SPEED_57600  0x03
#define PS350_SPEED_115200 0x04

#endif
