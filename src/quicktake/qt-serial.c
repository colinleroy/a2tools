#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#ifndef __CC65__
#include <sys/ioctl.h>
#endif
#include "platform.h"
#include "dgets.h"
#include "extended_conio.h"
#include "extended_string.h"
#include "progress_bar.h"
#include "simple_serial.h"
#include "qt-serial.h"
#include "qt1x0-serial.h"
#include "qt200-serial.h"
#include "qt-conv.h"

#define DEBUG_TIMING 0

extern uint8 scrw, scrh;

uint8 serial_model = QT_MODEL_UNKNOWN;

#ifndef __CC65__
FILE *dbgfp = NULL;
#endif

unsigned char buffer[BLOCK_SIZE];

#pragma code-name(push, "RT_ONCE")

/* Connect to a QuickTake and detect its model */
uint8 qt_serial_connect(uint16 speed) {
  simple_serial_close();

  /* Set initial speed */
#ifdef __CC65__
  simple_serial_set_speed(SER_BAUD_9600);
  simple_serial_set_flow_control(SER_HS_NONE);
#else
  simple_serial_set_speed(B9600);
#endif
  if (simple_serial_open() != 0) {
    cputs("Cannot open port\r\n");
    return -1;
  }
  simple_serial_flush();

  /* Try and detect a QuickTake 1x0 */
  serial_model = qt1x0_wakeup(speed);

  /* Set parity to EVEN (all QuickTakes need it at that point) */
#ifdef __CC65__
  simple_serial_set_parity(SER_PAR_EVEN);
#else
  simple_serial_set_parity(PARENB);
#endif

  if (serial_model == QT_MODEL_UNKNOWN) {
    if (qt200_wakeup() == 0) {
      serial_model = QT_MODEL_200;
    }
  }

  cputs("\r\n");

  if (serial_model == QT_MODEL_UNKNOWN) {
    cputs("No camera connected.\r\n");
    return -1;
  }

  cputs("Initializing...\r\n");

  /* Upgrade to target speed */
  if (serial_model != QT_MODEL_200)
    return qt1x0_set_speed(speed);
  else
    return qt200_set_speed(speed);
}

/* Protocol-dependant camera functions */

#pragma code-name(pop)
#pragma code-name(push, "LC")

uint8 qt_take_picture(void) {
  if (serial_model != QT_MODEL_200)
    return qt1x0_take_picture();
  else
    return -1;
}

uint8 qt_set_camera_name(const char *name) {
  if (serial_model != QT_MODEL_200)
    return qt1x0_set_camera_name(name);
  else
    return -1;
}

uint8 qt_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second) {
  if (serial_model != QT_MODEL_200)
    return qt1x0_set_camera_time(day, month, year, hour, minute, second);
  else
    return -1;
}

uint8 qt_set_quality(uint8 quality) {
  if (serial_model != QT_MODEL_200)
    return qt1x0_set_quality(quality);
  else
    return -1;
}

uint8 qt_set_flash(uint8 mode) {
  if (serial_model != QT_MODEL_200)
    return qt1x0_set_flash(mode);
  else
    return -1;
}

uint8 qt_get_picture(uint8 n_pic, FILE *picture) {
  if (serial_model != QT_MODEL_200)
    return qt1x0_get_picture(n_pic, picture);
  else
    return qt200_get_picture(n_pic, picture);
}

uint8 qt_get_thumbnail(uint8 n_pic, FILE *picture, thumb_info *info) {
  if (serial_model != QT_MODEL_200)
    return qt1x0_get_thumbnail(n_pic, picture, info);
  else
    return -1;
}

#pragma code-name(pop)

uint8 qt_delete_pictures(void) {
  if (serial_model != QT_MODEL_200)
    return qt1x0_delete_pictures();
  else
    return -1;
}

#pragma code-name(push, "LOWCODE")

uint8 qt_get_information(camera_info *info) {
  if (serial_model != QT_MODEL_200)
    return qt1x0_get_information(info);
  else
    return qt200_get_information(info);
}

/* Helper functions */
void write_qtk_header(FILE *fp, const char *pic_format) {
  char hdr[] = {0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x04,0x00,0x00,0x73,0xE4,0x00,0x01};

  memcpy(hdr, pic_format, 4);
  fwrite(hdr, 1, sizeof hdr, fp);
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
