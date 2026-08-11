#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#ifndef __CC65__
#include <sys/ioctl.h>
#endif
#include "clrzone.h"
#include "platform.h"
#include "extended_conio.h"
#include "progress_bar.h"
#include "simple_serial.h"
#include "qt-serial.h"
#include "qt-conv.h"
#include "zx02_decompress_in_place.h"

#define DEBUG_TIMING 0

extern uint8 scrw, scrh;

static const char *camera_drivers[] = {
  "QT1X0.ZX",
  "QT200.ZX",
  NULL
};

uint8 serial_model = QT_MODEL_UNKNOWN;

#ifndef __CC65__
FILE *dbgfp = NULL;
#endif

extern unsigned char buffer[BUFFER_SIZE];

#define CAM_WAKEUP          0
#define CAM_SET_SPEED       1
#define CAM_SET_CAMERA_NAME 2
#define CAM_SET_CAMERA_TIME 3
#define CAM_GET_INFORMATION 4
#define CAM_SET_QUALITY     5
#define CAM_SET_FLASH       6
#define CAM_TAKE_PICTURE    7
#define CAM_GET_PICTURE     8
#define CAM_GET_THUMBNAIL   9
#define CAM_DELETE_PICTURES 10

#define CAM_CAN_SET_CAMERA_NAME 0x01
#define CAM_CAN_SET_CAMERA_TIME 0x02
#define CAM_CAN_SET_QUALITY     0x04
#define CAM_CAN_SET_FLASH       0x08
#define CAM_CAN_TAKE_PICTURE    0x10
#define CAM_CAN_GET_THUMBNAIL   0x20
#define CAM_CAN_DELETE_PICTURES 0x40

extern uint8 cam_features;

#pragma code-name(push, "RT_ONCE")

char last_driver[] = "\0\0\0\0\0\0\0\0\0";
uint8 tried_default = 0;
char *default_driver(int mode) {
  int fd = open("LASTDRV", mode);
  if (fd == -1) {
    return NULL;
  }
  if (mode == O_RDONLY) {
    read(fd, last_driver, 8);
  } else {
    write(fd, last_driver, 8);
  }
  close(fd);
}

static uint8 load_driver(char *drv, uint16 speed) {
  gotox(0); clreol();
  if (zx02_decompress_in_place(drv, (char *)0xC00, (char *)0x1900) == 0) {
    if ((serial_model = cam_wakeup(speed)) != QT_MODEL_UNKNOWN) {
      return 0;
    }
  }
  return -1;
}

/* Connect to a QuickTake and detect its model */
uint8 qt_serial_connect(uint16 speed) {
  uint8 i;

  /* Set initial settings */
  simple_serial_close();
#ifdef __CC65__
  simple_serial_set_speed(SER_BAUD_9600);
  simple_serial_set_parity(SER_PAR_NONE);
#else
  simple_serial_set_speed(B9600);
  simple_serial_set_parity(0);
#endif
  camera_connected = 0;

  if (simple_serial_open() != 0) {
    cputs("Cannot open port\r\n");
    return -1;
  }

  /* Get last driver used */
  default_driver(O_RDONLY);
  if (last_driver[0] && load_driver(last_driver, speed) == 0) {
    goto load_done;
  }

  for (i = 0; IS_NOT_NULL(camera_drivers[i]); i++) {
    /* Don't retry the last driver, it just failed. */
    if (strcmp(last_driver, camera_drivers[i]) && load_driver(camera_drivers[i], speed) == 0) {
      /* Register new default driver */
      strcpy(last_driver, camera_drivers[i]);
      default_driver(O_WRONLY|O_CREAT);
      break;
    }
  }

load_done:
  cputs("\r\n");

  if (serial_model == QT_MODEL_UNKNOWN) {
    cputs("No camera detected. ");
    return -1;
  }

  cputs("Initializing...\r\n");

  /* Upgrade to target speed */
  return cam_set_speed(speed);
}

/* Protocol-dependant camera functions */

#pragma code-name(pop)
#pragma code-name(push, "LC")

uint8 qt_take_picture(void) {
  if (cam_features & CAM_CAN_TAKE_PICTURE)
    return cam_take_picture();
  else
    return -1;
}

uint8 qt_set_camera_name(const char *name) {
  if (cam_features & CAM_CAN_SET_CAMERA_NAME)
    return cam_set_camera_name(name);
  else
    return -1;
}

uint8 qt_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second) {
  if (cam_features & CAM_CAN_SET_CAMERA_TIME)
    return cam_set_camera_time(day, month, year, hour, minute, second);
  else
    return -1;
}

uint8 qt_set_quality(uint8 quality) {
  if (cam_features & CAM_CAN_SET_QUALITY)
    return cam_set_quality(quality);
  else
    return -1;
}

uint8 qt_set_flash(uint8 mode) {
  if (cam_features & CAM_CAN_SET_FLASH)
    return cam_set_flash(mode);
  else
    return -1;
}

uint8 qt_get_thumbnail(uint8 n_pic, int fd, thumb_info *info) {
  if (cam_features & CAM_CAN_GET_THUMBNAIL)
    return cam_get_thumbnail(n_pic, fd, info);
  else
    return -1;
}


uint8 qt_delete_pictures(void) {
  if (cam_features & CAM_CAN_DELETE_PICTURES)
    return cam_delete_pictures();
  else
    return -1;
}

const char *qt_get_quality_str(uint8 mode) {
  switch(mode) {
    case QUALITY_STANDARD:
      return "standard quality";
    case QUALITY_HIGH:
      return "high quality";
    default:
      return "Unknown";
  }
}

const char *qt_get_flash_str(uint8 mode) {
  switch(mode) {
    case FLASH_AUTO:
      return "automatic";
    case FLASH_OFF:
      return "disabled";
    case FLASH_ON:
      return "forced";
    default:
      return "Unknown";
  }
}
#pragma code-name(pop)
