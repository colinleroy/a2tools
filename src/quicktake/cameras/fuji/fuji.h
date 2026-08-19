#ifndef _FUJI_H
#define _FUJI_H

#include "platform.h"

extern uint8 scrw, scrh;
extern uint8 do_debug;

#define STX 0x02 /* Start of data */
#define ETX 0x03 /* End of data */
#define EOT 0x04 /* End of session */
#define ENQ 0x05 /* Enquiry */
#define ACK 0x06
#define ESC 0x10
#define ETB 0x17 /* End of transmission block */
#define NAK 0x15

#define CMD_ACK 0x00
#define CMD_NAK 0x01

#define FUJI_CMD_PIC_GET_THUMB 0x00
#define FUJI_CMD_PIC_GET_DATA  0x02
#define FUJI_CMD_SPEED         0x07
#define FUJI_CMD_GET_INFO      0x09
#define FUJI_CMD_PIC_NAME      0x0A
#define FUJI_CMD_PIC_COUNT     0x0B
#define FUJI_CMD_PIC_SIZE      0x17

#endif
