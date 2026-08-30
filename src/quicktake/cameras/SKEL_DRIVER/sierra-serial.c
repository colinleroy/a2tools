#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "a2_features.h"
#include "platform.h"
#include "extended_conio.h"
#include "progress_bar.h"
#include "simple_serial.h"
#include "sierra.h"
#include "../qt-serial.h"
#include "../../decoders/qt-conv.h"
#include "../../ui/ui.h"

#pragma code-name(push, "SIERRA")
#pragma rodata-name(push, "SIERRA")
#pragma data-name(push, "SIERRA")

/* Camera features */
#define sierra_features 0b0000000010000000
//                                ||||||||_ SET_CAMERA_NAME
//                                |||||||__ SET_CAMERA_TIME
//                                ||||||___ SET_QUALITY,
//                                |||||____ SET_FLASH,
//                                ||||_____ TAKE_PICTURE,
//                                |||______ GET_THUMBNAIL,
//                                ||_______ DELETE_PICTURES,
//                                |________ RESERVED,

/* Camera callbacks definitions */
static uint8 sierra_wakeup(CamSpeed speed);
static uint8 sierra_set_speed(CamSpeed speed);

/* Camera settings functions */
static uint8 sierra_get_information(void);

/* Camera pictures functions */
static uint8 sierra_get_picture(uint8 n_pic, int fd, off_t avail);
static uint8 sierra_get_thumbnail(uint8 n_pic, int fd, thumb_info *info);
static void sierra_get_filename(uint8 n_pic, char *dirname, char *filename);

/* Other functions, that this driver doesn't implement
 * but must exist and return -1
 */
static uint8 sierra_set_camera_name(const char *name);
static uint8 sierra_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second);
static uint8 sierra_set_quality(uint8 quality);
static uint8 sierra_set_flash(uint8 mode);
static uint8 sierra_take_picture(void);
static uint8 sierra_delete_pictures(void);

/* Camera thumbnail functions */
void sierra_thumb_histogram(void);
void sierra_load_thumb_data(uint8 line);

/* Modes strings */
static const char *sierra_get_quality_str(uint8 mode);
static const char *sierra_get_flash_str(uint8 mode);

/* Camera callbacks */
void *sierra_callbacks[] = {
  /* FEATURES */        (void *)sierra_features,
  /* WAKEUP */          sierra_wakeup,
  /* SET_SPEED */       sierra_set_speed,
  /* SET_CAMERA_NAME */ sierra_set_camera_name,
  /* SET_CAMERA_TIME */ sierra_set_camera_time,
  /* GET_INFORMATION */ sierra_get_information,
  /* SET_QUALITY */     sierra_set_quality,
  /* SET_FLASH */       sierra_set_flash,
  /* TAKE_PICTURE */    sierra_take_picture,
  /* GET_PICTURE */     sierra_get_picture,
  /* GET_THUMBNAIL */   sierra_get_thumbnail,
  /* DELETE_PICTURES */ sierra_delete_pictures,
  /* GET_FILENAME */    sierra_get_filename,
  /* THUMB_HISTOGRAM */ sierra_thumb_histogram,
  /* THUMB_LOAD_DATA */ sierra_load_thumb_data,
  /* GET_QUALITY_STR */ sierra_get_quality_str,
  /* GET_FLASH_STR */   sierra_get_flash_str,
};

#ifdef __CC65__
#define PC_DEBUG(op, str, len)
#else
static void PC_DEBUG(char *op, const char *str, int len) {
  if (do_debug) {
    printf("%s:", op);
    for (int i = 0; i < len; i++) {
      printf("%s %02X", i%16 == 0 ? "\n":"", (uint8)str[i]);
    }
    printf("\n");
  }
}
#endif

extern camera_info cam_info;

#pragma warn(unused-param, push, off)
/* Wakeup and detect a Sierra camera
 * Returns 0 if successful, -1 otherwise
 */
static uint8 sierra_wakeup(CamSpeed speed) {
  uint8 tries = 2, c;
  cputs("Pinging Sierra camera... ");

  simple_serial_set_speed(SER_BAUD_19200);
  simple_serial_set_parity(SER_PAR_EVEN);

  /* Flush shit */
  while (simple_serial_read_no_irq((char *)&c, 1) != EOF);
  return QT_MODEL_UNKNOWN;
}
#pragma warn(unused-param, pop)

static CamSpeed my_speed = SER_BAUD_9600;

/* Send the speed upgrade command */
static uint8 sierra_set_speed(CamSpeed speed) {
  return -1;
}

/* Get information from the camera */
static uint8 sierra_get_information(void) {
  return -1;
}

static void sierra_get_filename(uint8 n_pic, char *dirname, char *filename) {
  sprintf(filename, "%s%sIMAGE%d.JPG",
        IS_NOT_NULL(dirname)?dirname:"",
        IS_NOT_NULL(dirname)?"/":"", n_pic);
}

static uint8 sierra_get_picture(uint8 n_pic, int fd, off_t avail) {
  ui_get_image_header_str();
  ui_get_image_str(640, 480, 0UL);

  return -1;
}

static uint8 sierra_get_thumbnail(uint8 n_pic, int fd, thumb_info *info) {
  ui_get_thumbnail_str(n_pic);
  return -1;
}

#pragma warn(unused-param, push, off)
static uint8 sierra_set_camera_name(const char *name) {
  return -1;
}

static uint8 sierra_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second) {
  return -1;
}

static uint8 sierra_set_quality(uint8 quality) {
  return -1;
}

static uint8 sierra_set_flash(uint8 mode) {
  return -1;
}

static uint8 sierra_take_picture(void) {
  return -1;
}

static uint8 sierra_delete_pictures(void) {
  return -1;
}

static const char *sierra_get_quality_str(uint8 mode) {
  return "unknown";
}

static const char *sierra_get_flash_str(uint8 mode) {
  return "unknown";
}

#pragma warn(unused-param, pop)
