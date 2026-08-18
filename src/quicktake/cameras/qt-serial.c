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
#include "decoders/qt-conv.h"
#include "zx02_decompress_in_place.h"

#define DEBUG_TIMING 0

extern uint8 scrw, scrh;

static char *camera_drivers[] = {
  "QT1X0.ZX",
  "QT200.ZX",
  NULL
};

uint8 serial_model = QT_MODEL_UNKNOWN;
char *cam_file_extension[] = {
  ".???",
  ".QTK",
  ".QTK",
  ".JPG",
};


#ifndef __CC65__
FILE *dbgfp = NULL;
#endif

extern unsigned char buffer[BUFFER_SIZE];

#pragma code-name(push, "RT_ONCE")

#ifdef __CC65__
uint8 load_driver(char *drv, CamSpeed speed) {
  gotox(0); clreol();
  if (zx02_decompress_in_place(drv, (char *)0xC00, (char *)0x1900) == 0) {
#ifndef DEBUG_THUMB
    if ((serial_model = cam_wakeup(speed)) != QT_MODEL_UNKNOWN) {
      return 0;
    }
#endif
  }
  return -1;
}
#else
static uint8 load_driver(char *drv, CamSpeed speed) {
  if ((serial_model = cam_wakeup(speed)) != QT_MODEL_UNKNOWN) {
    return 0;
  }
  return -1;
}
#endif

/* Connect to a QuickTake and detect its model */
uint8 cam_serial_connect(CamSpeed speed) {
  uint8 i;

  simple_serial_read_config();

  /* Set initial settings */
  simple_serial_close();
  simple_serial_set_speed(SER_BAUD_9600);

  camera_connected = 0;

  if (simple_serial_open() != 0) {
    cputs("Cannot open port\r\n");
    return -1;
  }

  /* Get last driver used, from extra serial parameters */
  if (ser_params.extra_parameters[0] && load_driver(ser_params.extra_parameters, speed) == 0) {
    goto load_done;
  }

  for (i = 0; IS_NOT_NULL(camera_drivers[i]); i++) {
    /* Don't retry the last driver, it just failed. */
    if (strcmp(ser_params.extra_parameters, camera_drivers[i]) && load_driver(camera_drivers[i], speed) == 0) {
      /* Register new default driver */
      strcpy(ser_params.extra_parameters, camera_drivers[i]);
      simple_serial_write_config();
      break;
    }
  }

load_done:
  cputs("\r\n");

  if (serial_model == QT_MODEL_UNKNOWN) {
    cputs("No camera detected. ");
    return -1;
  }

  cputs("Connected. Upgrading speed...\r\n");

  /* Upgrade to target speed */
  return cam_set_speed(speed);
}

/* Protocol-dependant camera functions */

#pragma code-name(pop)
#pragma code-name(push, "LC")

const char *cam_get_quality_str(uint8 mode) {
  switch(mode) {
    case QUALITY_STANDARD:
      return "standard";
    case QUALITY_HIGH:
      return "high";
    default:
      return "unknown";
  }
}

const char *cam_get_flash_str(uint8 mode) {
  switch(mode) {
    case FLASH_AUTO:
      return "automatic";
    case FLASH_OFF:
      return "disabled";
    case FLASH_ON:
      return "forced";
    default:
      return "unknown";
  }
}
#pragma code-name(pop)
