#ifndef _SIERRA_H
#define _SIERRA_H

#include "platform.h"

extern uint8 scrw, scrh;
extern uint8 do_debug;

/* Indexes */
#define PACKET_TYPE                      0
#define PACKET_SUBTYPE                   1
#define PACKET_LENGTH                    2
#define PACKET_OPERATION                 4
#define PACKET_REGISTER                  5
#define PACKET_VALUE                     6

#define PACKET_RESPONSE_IDX              4

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

/* Registers */
#define SIERRA_REG_RESOLUTION         0x01
#define SIERRA_REG_DATE               0x02
#define SIERRA_REG_FLASH_MODE         0x07
#define SIERRA_REG_NUM_PICS           0x0A
#define SIERRA_REG_LEFT_PICS          0x0B
#define SIERRA_REG_SPEED              0x11
#define SIERRA_REG_NAME               0x16
#define SIERRA_REG_BATTERY            0x1C

/* Register ops */
#define OP_SET_INT                    0x00
#define OP_GET_INT                    0x01
#define OP_GET_STRING                 0x04

/* Sierra speeds */
#define SIERRA_SPEED_9600             0x01
#define SIERRA_SPEED_19200            0x02
#define SIERRA_SPEED_57600            0x04
#define SIERRA_SPEED_115200           0x05
#endif
