#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "platform.h"
#include "extended_conio.h"
#include "extended_string.h"
#include "progress_bar.h"
#include "simple_serial.h"
#include "qt-serial.h"
#include "qt1x0-serial.h"
#include "qt-conv.h"

extern uint8 scrw, scrh;

static uint8 serial_model = 0;

#ifndef __CC65__
FILE *dbgfp = NULL;
#endif

char buffer[BLOCK_SIZE];

#pragma code-name(push, "LC")

static uint8 qt_set_speed(uint16 speed) {
  if (serial_model == QT_MODEL_1X0)
    return qt1x0_set_speed(speed);
  else
    return -1;
}

/* Connect to a QuickTake and detect its model */
uint8 qt_serial_connect(uint16 speed) {
  simple_serial_close();
  printf("Connecting to Quicktake...\n");

  /* Set initial speed */
#ifdef __CC65__
  simple_serial_set_speed(SER_BAUD_9600);
#else
  simple_serial_set_speed(B9600);
#endif
  if (simple_serial_open() != 0) {
    printf("Cannot open port\n");
    return -1;
  }
  simple_serial_flush();

  /* Try and detect a QuickTake 1x0 */
  if (qt_1x0_wakeup(speed) == 0) {
    serial_model = QT_MODEL_1X0;
  }

  /* Set parity to EVEN (all QuickTakes need it at that point) */
#ifdef __CC65__
  simple_serial_set_parity(SER_PAR_EVEN);
#else
  simple_serial_set_parity(PARENB);
#endif
  printf("Parity set.\n");

  /* TODO: wakeup a QuickTake 200 if we don't have a 1x0
   * connected
   */

  platform_sleep(1);
  printf("Initializing...\n");

  /* Upgrade to target speed */
  return qt_set_speed(speed);
}

/* Protocol-dependant camera functions */

uint8 qt_take_picture(void) {
  if (serial_model == QT_MODEL_1X0)
    return qt1x0_take_picture();
  else
    return -1;
}

void qt_set_camera_name(const char *name) {
  if (serial_model == QT_MODEL_1X0)
    qt1x0_set_camera_name(name);
}

void qt_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second) {
  if (serial_model == QT_MODEL_1X0)
    qt1x0_set_camera_time(day, month, year, hour, minute, second);
}

uint8 qt_get_picture(uint8 n_pic, const char *filename, uint8 full) {
  if (serial_model == QT_MODEL_1X0)
    return qt1x0_get_picture(n_pic, filename, full);
  else
    return -1;
}

uint8 qt_delete_pictures(void) {
  if (serial_model == QT_MODEL_1X0)
    return qt1x0_delete_pictures();
  else
    return -1;
}

#pragma code-name(pop)

uint8 qt_get_information(uint8 *num_pics, uint8 *left_pics, uint8 *quality_mode, uint8 *flash_mode, uint8 *battery_level, char **name, struct tm *time) {
  if (serial_model == QT_MODEL_1X0)
    return qt1x0_get_information(num_pics, left_pics, quality_mode, flash_mode, battery_level, name, time);
  else
    return -1;
}

/* Helper functions */
void write_qtk_header(FILE *fp, const char *pic_format) {
  char hdr[] = {0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x04,0x00,0x00,0x73,0xE4,0x00,0x01};

  memcpy(hdr, pic_format, 4);
  fwrite(hdr, 1, sizeof hdr, fp);
}

const char *qt_get_mode_str(uint8 mode) {
  switch(mode) {
    case 1:  return "standard quality";
    case 2:  return "high quality";
    default: return "Unknown";
  }
}

const char *qt_get_flash_str(uint8 mode) {
  switch(mode) {
    case 0:  return "automatic";
    case 1:  return "disabled";
    case 2:  return "forced";
    default: return "Unknown";
  }
}
