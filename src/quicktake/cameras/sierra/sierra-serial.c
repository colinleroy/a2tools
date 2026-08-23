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
static uint8 sierra_get_information(camera_info *info);

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

#ifdef __CC65__
/* Use UI's info struct to spare memory */
extern camera_info cam_info;
#else
static camera_info cam_info;
#endif

#define packet_type (buffer[0])
#define packet_subtype (buffer[1])
#define packet_length ((buffer[2])|(buffer[3] << 8))

static uint8 sierra_read_packet(void) {
  /* either one byte or longer. length in bytes 2-3 */
  if (simple_serial_read_no_irq((char *)buffer, 1) == EOF) {
    return EOF;
  }
  if (packet_type == SIERRA_PACKET_DATA ||
      packet_type == SIERRA_PACKET_DATA_END ||
      packet_type == SIERRA_PACKET_COMMAND) {
    /* Read subtype and length */
    if (simple_serial_read_no_irq((char *)(buffer+1), 3) == EOF) {
      return EOF;
    }
    /* Read two more bytes for the checksum */
    if (simple_serial_read_no_irq((char *)(buffer+4), packet_length+2) != EOF) {
      return 0;
    }
  } else {
    /* We read the single-byte "packet" */
    return 0;
  }
  return EOF;
}

static void sierra_flush(void) {
  uint8 r;
  while (simple_serial_read_no_irq((char *)&r, 1) != EOF);
}

uint8 first_packet;
static void sierra_write_packet(void) {
  uint16 i, len, chksum;
  if (buffer[0] == SIERRA_PACKET_COMMAND) {
    buffer[PACKET_SUBTYPE]  = first_packet ? SIERRA_SUBPACKET_CMD_FIRST : SIERRA_SUBPACKET_CMD;
    first_packet = 0;

    len = packet_length + 4; /* header length */

    chksum = 0;
    for (i = 4; i < len; i++) {
      chksum += buffer[i];
    }
    buffer[len]   = chksum        & 0xFF;
    buffer[len+1] = (chksum >> 8) & 0xFF;
    simple_serial_write((char *)buffer, len+2);
  } else {
    simple_serial_putc(buffer[0]);
  }
}

static uint8 sierra_write_int(uint8 reg, int32 value) {
  /* Note: libgphoto2 sends no value if value < 0? */
  bzero(buffer, 10);
  buffer[PACKET_TYPE]     = SIERRA_PACKET_COMMAND;
  buffer[PACKET_LENGTH]   = 6 & 0xFF;
  buffer[PACKET_LENGTH+1] = 6 >> 8;
  buffer[PACKET_REGISTER] = reg;

  /* TODO: optimize that, the assembly is going to be ugly */
  buffer[PACKET_VALUE]    = value         & 0xFF;
  buffer[PACKET_VALUE+1]  = (value >> 8)  & 0xFF;
  buffer[PACKET_VALUE+2]  = (value >> 16) & 0xFF;
  buffer[PACKET_VALUE+3]  = (value >> 24) & 0xFF;

  sierra_write_packet();
  PC_DEBUG("write_int sent: ", buffer, packet_length+6);

  if (sierra_read_packet() == 0
   && (buffer[0] == SIERRA_PACKET_ENQ|| buffer[0] == SIERRA_PACKET_ACK)) {
     PC_DEBUG("write_int reply OK: ", buffer, packet_length+6);
    return 0;
  }
  PC_DEBUG("write_int reply NOK: ", buffer, packet_length+6);
  return -1;
}

#pragma warn(unused-param, push, off)
/* Wakeup and detect a Sierra camera
 * Returns 0 if successful, -1 otherwise
 */
static uint8 sierra_wakeup(CamSpeed speed) {
  uint8 r;
  cputs("Pinging Sierra camera... ");

  first_packet = 1;

  /* Parity first because resets speed to 9600 on PC, a bug in
   * my lib that I don't want to investigate right now. */
  simple_serial_set_parity(SER_PAR_NONE);
  simple_serial_set_speed(SER_BAUD_19200);

  /* Flush shit */
  sierra_flush();
  r = QT_MODEL_UNKNOWN;

  simple_serial_putc(0x00);
  if (sierra_read_packet() == 0 && buffer[0] == SIERRA_PACKET_NAK) {
    r = QT_MODEL_SIERRA;
  }
  PC_DEBUG("wakeup packet: ", buffer, 10);
  return r;
}
#pragma warn(unused-param, pop)

/* Default speed for Sierra cameras */
static CamSpeed my_speed = SER_BAUD_19200;

/* Send the speed upgrade command */
static uint8 sierra_set_speed(CamSpeed speed) {
  uint8 sierra_speed;
  switch(speed) {
  case SER_BAUD_9600:   sierra_speed = SIERRA_SPEED_9600;   break;
  case SER_BAUD_19200:  sierra_speed = SIERRA_SPEED_19200;  break;
  case SER_BAUD_115200: sierra_speed = SIERRA_SPEED_115200; break;
  }
  sierra_write_int(SIERRA_REG_SPEED, sierra_speed);
  simple_serial_set_speed(speed);
  sierra_flush();
  return 0;
}

/* Get information from the camera */
static uint8 sierra_get_information(camera_info *info) {
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
