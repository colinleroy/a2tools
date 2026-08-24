#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
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

    /* Too fast for PCDC001? do it char by char */
    // simple_serial_write((char *)buffer, len+2);
    for (i = 0; i < len+2; i++) {
      simple_serial_putc(buffer[i]);
    }
  } else {
    simple_serial_putc(buffer[0]);
  }
}

#define sierra_write_ack() do { simple_serial_putc(SIERRA_PACKET_ACK); } while (0)

static void sierra_build_packet(uint8 type, uint16 length, uint8 op, uint8 reg) {
  buffer[PACKET_TYPE]     = type;
  buffer[PACKET_LENGTH]   = length & 0xFF;
  buffer[PACKET_LENGTH+1] = length >> 8;

  buffer[PACKET_OPERATION]= op;
  buffer[PACKET_REGISTER] = reg;
}

static uint8 sierra_write_int(uint8 reg, int32 value) {
  /* Note: libgphoto2 sends no value if value < 0? */
  sierra_build_packet(SIERRA_PACKET_COMMAND, 6, OP_SET_INT, reg);

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

static uint8 sierra_read_int(uint8 reg) {
  sierra_build_packet(SIERRA_PACKET_COMMAND, 2, OP_GET_INT, reg);
  sierra_write_packet();
  PC_DEBUG("read_int sent: ", buffer, 2+6);
  if (sierra_read_packet() == 0) {
    PC_DEBUG("read_int reply OK: ", buffer, 2+6);
    sierra_write_ack();
    return 0;
  }
  PC_DEBUG("read_int reply NOK: ", buffer, 2+6);
  return -1;
}

static uint8 sierra_read_string(uint8 reg) {
  sierra_build_packet(SIERRA_PACKET_COMMAND, 2, OP_GET_STRING, reg);
  sierra_write_packet();
  PC_DEBUG("read_string sent: ", buffer, 2+6);
  if (sierra_read_packet() == 0) {
    PC_DEBUG("read_string reply OK: ", buffer, packet_length+6);
    sierra_write_ack();
    return 0;
  }
  PC_DEBUG("read_string reply NOK: ", buffer, 2+6);
  return -1;
}

#pragma warn(unused-param, push, off)
/* Wakeup and detect a Sierra camera
 * Returns 0 if successful, -1 otherwise
 */
static uint8 sierra_wakeup(CamSpeed speed) {
  uint8 r, i;
  cputs("Pinging Sierra camera... ");

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
    PC_DEBUG("wakeup packet OK: ", buffer, 1);
  } else {
    PC_DEBUG("wakeup packet NOK: ", buffer, 10);
  }
  return r;
}
#pragma warn(unused-param, pop)

/* Send the speed upgrade command */
static uint8 sierra_set_speed(CamSpeed speed) {
  uint8 sierra_speed;

  /* PCDC001 can't do better reliably */
  speed = SER_BAUD_19200;
  switch(speed) {
  case SER_BAUD_9600:   sierra_speed = SIERRA_SPEED_9600;   break;
  case SER_BAUD_19200:  sierra_speed = SIERRA_SPEED_19200;  break;
  case SER_BAUD_57600:  sierra_speed = SIERRA_SPEED_57600;  break;
  case SER_BAUD_115200: sierra_speed = SIERRA_SPEED_115200; break;
  }

  first_packet = 1;
  sierra_write_int(SIERRA_REG_SPEED, sierra_speed);
  simple_serial_set_speed(speed);
  platform_msleep(10);
  return 0;
}

/* Get information from the camera */
static uint8 sierra_get_information(camera_info *info) {
  time_t cam_date;
  struct tm *tm_time;

  if (sierra_read_int(SIERRA_REG_NUM_PICS) != 0) {
    return -1;
  }
  info->num_pics = buffer[PACKET_RESPONSE_IDX]; /* Ignore +1/2/3 */

  if (sierra_read_int(SIERRA_REG_LEFT_PICS) != 0) {
    return -1;
  }
  info->left_pics = buffer[PACKET_RESPONSE_IDX]; /* Ignore +1/2/3 */

  if (sierra_read_int(SIERRA_REG_FLASH_MODE) != 0) {
    return -1;
  }
  info->flash_mode = buffer[PACKET_RESPONSE_IDX]; /* Ignore +1/2/3 */

  if (sierra_read_int(SIERRA_REG_RESOLUTION) != 0) {
    return -1;
  }
  info->quality_mode = buffer[PACKET_RESPONSE_IDX]; /* Ignore +1/2/3 */

  if (sierra_read_int(SIERRA_REG_BATTERY) != 0) {
    return -1;
  }
  info->battery_level = (buffer[PACKET_RESPONSE_IDX]*100)/256; /* Ignore +1/2/3 */

  if (sierra_read_int(SIERRA_REG_DATE) != 0) {
    return -1;
  }
#ifndef __CC65__
  cam_date     =  buffer[PACKET_RESPONSE_IDX+0]
               + (buffer[PACKET_RESPONSE_IDX+1] << 8)
               + (buffer[PACKET_RESPONSE_IDX+2] << 16)
               + (buffer[PACKET_RESPONSE_IDX+3] << 24);
#else
  /* Get size (24 bits big endian)*/
  ((unsigned char *)&cam_date)[0] = buffer[PACKET_RESPONSE_IDX+0];
  ((unsigned char *)&cam_date)[1] = buffer[PACKET_RESPONSE_IDX+1];
  ((unsigned char *)&cam_date)[2] = buffer[PACKET_RESPONSE_IDX+2];
  ((unsigned char *)&cam_date)[3] = buffer[PACKET_RESPONSE_IDX+3];
#endif
  tm_time = localtime(&cam_date);
  info->date.year   = tm_time->tm_year + 1900;
  info->date.month  = tm_time->tm_mon + 1;
  info->date.day    = tm_time->tm_mday;
  info->date.hour   = tm_time->tm_hour;
  info->date.minute = tm_time->tm_min;

  if (sierra_read_string(SIERRA_REG_NAME) != 0) {
    return -1;
  }
  strcpy(info->name, buffer+PACKET_RESPONSE_IDX);
  return 0;
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
  switch(mode) {
  case 0: return "high";
  case 1: return "low";
  }
  return "unknown";
}

static const char *sierra_get_flash_str(uint8 mode) {
  switch(mode) {
  case 0: return "automatic";
  case 1: return "forced";
  case 2: return "off";
  }
  return "unknown";
}

#pragma warn(unused-param, pop)
