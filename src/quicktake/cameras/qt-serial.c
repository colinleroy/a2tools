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

uint8 serial_model = QT_MODEL_UNKNOWN;
char *cam_file_extension[] = {
  ".???", /* QT_MODEL_UNKNOWN */
  ".QTK", /* QT_MODEL_100 */
  ".QTK", /* QT_MODEL_150 */
  ".JPG", /* QT_MODEL_200 */
  ".KDC", /* QT_MODEL_DC50 */
  ".JPG", /* QT_MODEL_SIERRA */
};


#ifndef __CC65__
FILE *dbgfp = NULL;
#endif

extern unsigned char buffer[BUFFER_SIZE];

#pragma code-name(push, "RT_ONCE")

#ifdef __CC65__
uint8 load_driver(char *drv, CamSpeed speed) {
  gotox(0); clreol();
  if (zx02_decompress_in_place(drv, (char *)0xC00, (char *)0x1A00) == 0) {
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
  simple_serial_read_config();

  /* Set initial settings */
  simple_serial_close();

  camera_connected = 0;

  if (simple_serial_open() != 0) {
    cputs("Cannot open port\r\n");
    return -1;
  }

  /* Get last driver used, from extra serial parameters */
  if (load_driver(ser_params.extra_parameters, speed) != 0) {
    cputs("No camera detected. ");
    return -1;
  }

  cputs("Connected. Upgrading speed...\r\n");

  /* Upgrade to target speed */
  return cam_set_speed(speed);
}

/* Protocol-dependant camera functions */

#pragma code-name(pop)
