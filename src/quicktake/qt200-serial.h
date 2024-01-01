#ifndef __qt_200_serial_h
#define __qt_200_serial_h

#include <stdio.h>
#include <sys/types.h>
#include "qt-serial.h"

/* Connection functions */
uint8 qt200_wakeup(void);
uint8 qt200_set_speed(uint16 speed);

/* Camera settings functions */
uint8 qt200_get_information(camera_info *info);

/* Camera pictures functions */
uint8 qt200_get_picture(uint8 n_pic, FILE *picture, off_t avail);

#endif
