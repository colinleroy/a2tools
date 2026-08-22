#ifndef _DC50_H
#define _DC50_H

#include "platform.h"

extern uint8 scrw, scrh;
extern uint8 do_debug;

#define CMD_SET_SPEED     0x41
#define CMD_GET_STATUS    0x7f

#define CMD_GET_CAM_PIC   0x51
#define CMD_GET_CARD_PIC  0x61

#define CMD_CAM_PIC_INFO  0x55
#define CMD_CARD_PIC_INFO 0x65

#define CMD_GET_CAM_THUMB   0x56
#define CMD_GET_CARD_THUMB  0x66

#define CMD_DELETE_CAM    0x7A
#define CMD_DELETE_CARD   0x7B

#define CMD_SET_NAME      0x9E

#define PIC_TARGET_CAM    0x00
#define PIC_TARGET_CARD   0x10
#if (CMD_CAM_PIC_INFO+PIC_TARGET_CARD) != CMD_CARD_PIC_INFO
#error Unexpected picture source value
#endif

// $DC50_ERASE_IMAGE_IN_CAMERA  = 0x7A;
// $DC50_ERASE_IMAGE_IN_CARD    = 0x7B;
// $DC50_TAKE_PICTURE_TO_CAMERA = 0x77;
// $DC50_TAKE_PICTURE_TO_CARD   = 0x7C;
// $DC50_INITIALIZE             = 0x7E;
// $DC50_STATUS                 = 0x7F;


#define REP_COMPLETE    0x00
#define CTRL_EOF        0x80
#define REP_ACK         0xd1
#define REP_CORRECT     0xd2
#define REP_NACK        0xe1
#define REP_EXEC_ERR    0xe2
#define REP_ILLEGAL     0xe3
#define REP_BUSY        0xf0

#endif
