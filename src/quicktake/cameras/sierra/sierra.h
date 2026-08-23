#ifndef _SIERRA_H
#define _SIERRA_H

#include "platform.h"

extern uint8 scrw, scrh;
extern uint8 do_debug;

/* Packet types */
#define SIERRA_PACKET_NUL             0x00
#define SIERRA_PACKET_DATA            0x02
#define SIERRA_PACKET_DATA_END        0x03
#define SIERRA_PACKET_ENQ             0x05
#define SIERRA_PACKET_ACK             0x06
#define SIERRA_PACKET_INVALID         0x11
#define SIERRA_PACKET_NAK             0x15
#define SIERRA_PACKET_CANCEL          0x18
#define SIERRA_PACKET_COMMAND         0x1b
#define SIERRA_PACKET_WRONG_SPEED     0x8c
#define SIERRA_PACKET_SESSION_ERROR   0xfc
#define SIERRA_PACKET_SESSION_END     0xff

#define SIERRA_PACKET_SIZE            32774

/* Sub-types */
#define SIERRA_SUBPACKET_CMD_FIRST    0x53
#define SIERRA_SUBPACKET_CMD          0x43

#endif
