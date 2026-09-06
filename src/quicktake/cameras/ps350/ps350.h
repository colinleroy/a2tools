#ifndef _SIERRA_H
#define _SIERRA_H

#include "platform.h"

extern uint8 scrw, scrh;
extern uint8 do_debug;

#define PS350_PKT_LEN 300

#define PS350_CNT_IDX       1
#define PS350_TYPE_IDX      2
#define PS350_CMD_IDX       9
#define PS350_CHK_IDX       297
#define PS350_DATA_IDX      21

#define INIT_REPLY_NAME_IDX 29


#define CMD_EOT       0x03
#define CMD_ACK       0x04
#define CMD_PACKET    0x10
#define CMD_SPD       0x42

#define CMD_CODE_PING        0x31
#define CMD_CODE_PING_REPLY  0x09
#define CMD_CODE_USE_DISK    0x14
#define CMD_CODE_GET_DISKS   0x18

#define PS350_SPEED_9600   0x00
#define PS350_SPEED_19200  0x01
#define PS350_SPEED_57600  0x03
#define PS350_SPEED_115200 0x04

#endif
