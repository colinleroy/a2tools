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

static uint8 sierra_reset(void);

#ifdef __CC65__
#define PC_DEBUG_BUFFER(op, str, len)
#define PC_DEBUG_PRINTF(...)
#else
#define PC_DEBUG_PRINTF(...) do { if (do_debug) printf(__VA_ARGS__); } while (0)

static void PC_DEBUG_BUFFER(char *op, const char *str, int len) {
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

uint16 sierra_response_len;
uint8 sierra_response_continues;
uint8 sierra_packet_type;

static uint8 sierra_read_packet(void) {
  uint8 header[3];
  /* either one byte or longer. length in bytes 2-3 */
  if (simple_serial_read_no_irq((char *)&sierra_packet_type, 1) == EOF) {
    PC_DEBUG_PRINTF("timeout reading one byte\n");
    return EOF;
  }
  if (sierra_packet_type == SIERRA_PACKET_DATA ||
      sierra_packet_type == SIERRA_PACKET_DATA_END ||
      sierra_packet_type == SIERRA_PACKET_COMMAND) {

    sierra_response_continues = (sierra_packet_type == SIERRA_PACKET_DATA);
    /* Read subtype and length */
    if (simple_serial_read_no_irq((char *)header, 3) == EOF) {
      return EOF;
    }
    PC_DEBUG_BUFFER("HEADER ", header, 3);
    /* Ignore subtype for now */
    sierra_response_len = header[1] | (header[2] << 8);
    /* Read actual data */
    if (simple_serial_read_no_irq((char *)buffer, sierra_response_len) == EOF) {
      return EOF;
    }
    PC_DEBUG_BUFFER("RESPONSE ", buffer, sierra_response_len);

    /* Read two more bytes for the checksum */
    return simple_serial_read_no_irq((char *)header, 2);
  } else if (sierra_packet_type == SIERRA_PACKET_SESSION_END) {
    PC_DEBUG_PRINTF("got %02X\n", sierra_packet_type);
    return -1;
  } else {
    PC_DEBUG_PRINTF("got %02X\n", sierra_packet_type);
    /* We read the single-byte "packet" */
    return 0;
  }
}

static void sierra_flush(void) {
  uint8 r;
  while (simple_serial_read_no_irq((char *)&r, 1) != EOF);
}

uint8 first_packet = 1;

static void sierra_write_packet(void) {
  uint16 i, len, chksum;
  simple_serial_putc(buffer[0]);
  if (buffer[0] == SIERRA_PACKET_COMMAND) {
    simple_serial_putc(first_packet ? SIERRA_SUBPACKET_CMD_FIRST : SIERRA_SUBPACKET_CMD);
    first_packet = 0;

    simple_serial_putc(buffer[2]);
    simple_serial_putc(buffer[3]);
    len = ((buffer[2])|(buffer[3] << 8)) + 4; /* header length */

    chksum = 0;
    for (i = 4; i < len; i++) {
      simple_serial_putc(buffer[i]);
      chksum += buffer[i];
    }
    simple_serial_putc(chksum & 0xFF);
    simple_serial_putc(chksum >> 8);
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
  uint8 tries = 0;
try_again:
  sierra_build_packet(SIERRA_PACKET_COMMAND, 6, OP_SET_INT, reg);

  /* TODO: optimize that, the assembly is going to be ugly */
  buffer[PACKET_VALUE]    = value         & 0xFF;
  buffer[PACKET_VALUE+1]  = (value >> 8)  & 0xFF;
  buffer[PACKET_VALUE+2]  = (value >> 16) & 0xFF;
  buffer[PACKET_VALUE+3]  = (value >> 24) & 0xFF;

  sierra_write_packet();
  PC_DEBUG_BUFFER("write_int sent: ", buffer, 6+6);

  if (sierra_read_packet() != 0) {
    if (sierra_packet_type == SIERRA_PACKET_SESSION_END) {
      if (tries++ < 3) {
        sierra_reset();
        goto try_again;
      }
    }
    return -1;
  }
  PC_DEBUG_PRINTF("write_int reply: %d", sierra_packet_type);
  if (sierra_packet_type == SIERRA_PACKET_ENQ|| sierra_packet_type == SIERRA_PACKET_ACK) {
    return 0;
  }
  return -1;
}

static uint8 sierra_read_int(uint8 reg) {
  uint8 tries = 0;
try_again:

  sierra_build_packet(SIERRA_PACKET_COMMAND, 2, OP_GET_INT, reg);
  sierra_write_packet();
  PC_DEBUG_BUFFER("read_int sent: ", buffer, 6);
  if (sierra_read_packet() == 0) {
    PC_DEBUG_PRINTF("read_int reply OK: %d", sierra_packet_type);
    PC_DEBUG_BUFFER("read_int details: ", buffer, 6);
    sierra_write_ack();
    return 0;
  } else if (sierra_packet_type == SIERRA_PACKET_SESSION_END) {
    if (tries++ < 3) {
      sierra_reset();
      goto try_again;
    }
  }
  PC_DEBUG_BUFFER("read_int reply NOK: ", buffer, 6);
  return -1;
}

static uint8 sierra_write_string(uint8 reg, const char *value, uint16 len) {
  uint8 tries = 0;
try_again:
  sierra_build_packet(SIERRA_PACKET_COMMAND, len+2, OP_SET_STRING, reg);

  /* FIXME doesn't handle packets longer than max packet size */
  memcpy(buffer+6, value, len);

  sierra_write_packet();
  PC_DEBUG_BUFFER("write_string sent: ", buffer, 6+6);

  if (sierra_read_packet() != 0) {
    if (sierra_packet_type == SIERRA_PACKET_SESSION_END) {
      if (tries++ < 3) {
        sierra_reset();
        goto try_again;
      }
    }
    return -1;
  }
  PC_DEBUG_PRINTF("write_string reply: %d", sierra_packet_type);
  if (sierra_packet_type == SIERRA_PACKET_ENQ|| sierra_packet_type == SIERRA_PACKET_ACK) {
    return 0;
  }
  return -1;
}

static uint8 sierra_read_string(uint8 reg) {
  uint8 tries = 0;
try_again:

  sierra_build_packet(SIERRA_PACKET_COMMAND, 2, OP_GET_STRING, reg);
  sierra_write_packet();
  PC_DEBUG_BUFFER("read_string sent: ", buffer, 2+6);
  if (sierra_read_packet() == 0) {
    PC_DEBUG_PRINTF("packet type %02X length %d\n", sierra_packet_type, sierra_response_len);
    PC_DEBUG_BUFFER("read_string reply OK: ", buffer, sierra_response_len);
    sierra_write_ack();
    return 0;
  } else if (sierra_packet_type == SIERRA_PACKET_SESSION_END) {
    if (tries++ < 3) {
      sierra_reset();
      goto try_again;
    }
  }
  PC_DEBUG_BUFFER("read_string reply NOK: ", buffer, 2+6);
  return -1;
}

static CamSpeed my_speed;

static char speed_set_packet[] = {SIERRA_PACKET_COMMAND, SIERRA_SUBPACKET_CMD_FIRST, 0x06, 0x00, 0x00, 
                                  SIERRA_REG_SPEED, SIERRA_SPEED_19200, 0x00, 0x00,
                                  0x00, SIERRA_REG_SPEED+SIERRA_SPEED_19200, 0x00};
static uint8 sierra_reset(void) {
  static uint8 first_reset = 1;
  uint8 tries = 0;
  PC_DEBUG_PRINTF("Resetting camera\n");

try_again:
  /* Parity first because resets speed to 9600 on PC, a bug in
   * my lib that I don't want to investigate right now. */
  simple_serial_set_parity(SER_PAR_NONE);
  simple_serial_set_speed(SER_BAUD_19200);

  if (first_reset) {
    /* Flush shit */
    sierra_flush();
    first_reset = 0;
  }

  /* Do the reset without interfering with buffer, so that we can reset anytime. */
  simple_serial_putc(0x00);
  if (simple_serial_read_no_irq((char *)&sierra_packet_type, 1) == 0
   && sierra_packet_type == SIERRA_PACKET_NAK) {
    uint8 sierra_speed, chksum, i;
    /* PCDC001 can't do better reliably */
    switch(my_speed) {
    case SER_BAUD_9600:   sierra_speed = SIERRA_SPEED_9600;   break;
    case SER_BAUD_19200:  sierra_speed = SIERRA_SPEED_19200;  break;
    case SER_BAUD_57600:  sierra_speed = SIERRA_SPEED_57600;  break;
    case SER_BAUD_115200: sierra_speed = SIERRA_SPEED_115200; break;
    }
    speed_set_packet[6]  = sierra_speed;
    speed_set_packet[10] = SIERRA_REG_SPEED+sierra_speed;
    for (i = 0; i < sizeof speed_set_packet; i++) {
      simple_serial_putc(speed_set_packet[i]);
    }
    PC_DEBUG_BUFFER("Reset: sent ", speed_set_packet, sizeof speed_set_packet);
    if (simple_serial_read_no_irq((char *)&sierra_packet_type, 1) == 0
     && sierra_packet_type == SIERRA_PACKET_ACK) {
      simple_serial_set_speed(my_speed);
      platform_msleep(10);
      first_packet = 0;

      return 0;
    }
  } else if (sierra_packet_type == SIERRA_PACKET_SESSION_END && tries++ < 3) {
    platform_msleep(500);
    goto try_again;
  } 

  return -1;
}

#pragma warn(unused-param, push, off)
/* Wakeup and detect a Sierra camera
 * Returns 0 if successful, -1 otherwise
 */
static uint8 sierra_wakeup(CamSpeed speed) {
  uint8 r;
  cputs("Pinging Sierra camera... ");
  if (speed == SER_BAUD_115200) {
    my_speed = is_iigs ? SER_BAUD_57600:SER_BAUD_19200;
  } else {
    my_speed = SER_BAUD_19200;
  }

  r = QT_MODEL_UNKNOWN;
  if (sierra_reset() == 0) {
    r = QT_MODEL_SIERRA;
  }
  return r;
}

/* Done at reset time */
static uint8 sierra_set_speed(CamSpeed speed) {
  return 0;
}
#pragma warn(unused-param, pop)

/* Get information from the camera */
static uint8 sierra_get_information(camera_info *info) {
  // time_t cam_date;
  // struct tm *tm_time;
  /* Use our own static cam_info for codesize */
  if (sierra_read_int(SIERRA_REG_NUM_PICS) != 0) {
    goto out_err;
  }
  cam_info.num_pics = buffer[0]; /* Ignore +1/2/3 */

  if (sierra_read_int(SIERRA_REG_LEFT_PICS) != 0) {
    goto out_err;
  }
  cam_info.left_pics = buffer[0]; /* Ignore +1/2/3 */

  if (sierra_read_int(SIERRA_REG_FLASH_MODE) != 0) {
    goto out_err;
  }
  cam_info.flash_mode = buffer[0]; /* Ignore +1/2/3 */

  if (sierra_read_int(SIERRA_REG_RESOLUTION) != 0) {
    goto out_err;
  }
  cam_info.quality_mode = buffer[0]; /* Ignore +1/2/3 */

  if (sierra_read_int(SIERRA_REG_BATTERY) != 0) {
    goto out_err;
  }
  cam_info.battery_level = (buffer[0]*100)/256; /* Ignore +1/2/3 */

// Date is not correct on at least Epson PhotoPC PCDC001, so skip that
//   if (sierra_read_int(SIERRA_REG_DATE) != 0) {
//     goto out_err;
//   }
// #ifndef __CC65__
//   cam_date     =  buffer[0]
//                + (buffer[1] << 8)
//                + (buffer[2] << 16)
//                + (buffer[3] << 24);
// #else
//   /* Get size (24 bits big endian)*/
//   ((unsigned char *)&cam_date)[0] = buffer[0];
//   ((unsigned char *)&cam_date)[1] = buffer[1];
//   ((unsigned char *)&cam_date)[2] = buffer[2];
//   ((unsigned char *)&cam_date)[3] = buffer[3];
// #endif
//   tm_time = localtime(&cam_date);
//   cam_info.date.year   = tm_time->tm_year + 1900;
//   cam_info.date.month  = tm_time->tm_mon + 1;
//   cam_info.date.day    = tm_time->tm_mday;
//   cam_info.date.hour   = tm_time->tm_hour;
//   cam_info.date.minute = tm_time->tm_min;

  if (sierra_read_string(SIERRA_REG_NAME) != 0) {
    goto out_err;
  }
  strcpy(cam_info.name, (char *)buffer);
  memcpy(info, &cam_info, sizeof(cam_info));
  return 0;
out_err:
  return -1;
}

static void sierra_get_filename(uint8 n_pic, char *dirname, char *filename) {
  sprintf(filename, "%s%sIMAGE%d.JPG",
        IS_NOT_NULL(dirname)?dirname:"",
        IS_NOT_NULL(dirname)?"/":"", n_pic);
}

static uint8 sierra_get_picture_data(uint8 n_pic, int fd, off_t avail, uint8 full_size) {
  off_t pic_size;
  uint8 n_blocks, cur_block;

  ui_get_image_header_str();

  if (sierra_write_int(SIERRA_REG_PIC_NUM , n_pic) != 0
   || sierra_read_int(full_size ? SIERRA_REG_PIC_SIZE : SIERRA_REG_THUMB_SIZE) != 0) {
    goto out_err;
  }

#ifndef __CC65__
  pic_size     =  buffer[0]
               + (buffer[1] << 8)
               + (buffer[2] << 16)
               + (buffer[3] << 24);
#else
  /* Get size (24 bits big endian)*/
  ((unsigned char *)&pic_size)[0] = buffer[0];
  ((unsigned char *)&pic_size)[1] = buffer[1];
  ((unsigned char *)&pic_size)[2] = buffer[2];
  ((unsigned char *)&pic_size)[3] = buffer[3];
#endif

  if (full_size) {
    if (pic_size > avail) {
      errno = ENOSPC;
      goto out_err;
    }
    ui_get_image_str(640, 480, pic_size);
  } else {
    ui_get_thumbnail_str(n_pic);
  }

  n_blocks = (pic_size >> 11) + 1;
  progress_bar(2, wherey(), scrw - 2, 0, n_blocks);

  sierra_build_packet(SIERRA_PACKET_COMMAND, 2, OP_GET_STRING,
                      full_size ? SIERRA_REG_PIC_DATA : SIERRA_REG_THUMB_DATA);
  sierra_write_packet();
  PC_DEBUG_PRINTF("packet type %02X length %d\n", sierra_packet_type, sierra_response_len);
  PC_DEBUG_BUFFER("read_string reply OK: ", buffer, sierra_response_len);

  do {
    if (sierra_read_packet() != 0) {
      PC_DEBUG_PRINTF("Read string failed\n");
    goto out_err;
    }
    PC_DEBUG_PRINTF("Received %d bytes\n", sierra_response_len);
    write(fd, buffer, sierra_response_len);
    progress_bar(2, wherey(), scrw - 2, ++cur_block, n_blocks);

    sierra_write_ack();
  } while (sierra_response_continues);
  PC_DEBUG_PRINTF("done\n");
  return 0;
out_err:
  return -1;
}

static uint8 sierra_get_picture(uint8 n_pic, int fd, off_t avail) {
  return sierra_get_picture_data(n_pic, fd, avail, 1);
}

static uint8 sierra_get_thumbnail(uint8 n_pic, int fd, thumb_info *info) {
  return sierra_get_picture_data(n_pic, fd, 0, 0);
}

#pragma warn(unused-param, push, off)
static uint8 sierra_set_camera_name(const char *name) {
  return sierra_write_string(SIERRA_REG_NAME, name, strlen(name));
}

static uint8 sierra_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second) {
  return -1;
}

static uint8 sierra_set_quality(uint8 quality) {
  if (quality > 2) {
    quality = 1;
  }
  return sierra_write_int(SIERRA_REG_RESOLUTION, quality);
}

static uint8 sierra_set_flash(uint8 mode) {
  return sierra_write_int(SIERRA_REG_FLASH_MODE, mode % 3);
}

static uint8 sierra_take_picture(void) {
  return -1;
}

static uint8 sierra_delete_pictures(void) {
  return -1;
}

static const char *sierra_get_quality_str(uint8 mode) {
  if (mode > 2) {
    mode = 1;
  }
  switch(mode) {
  case 2: return "high";
  case 1: return "low";
  }
  return "unknown";
}

static const char *sierra_get_flash_str(uint8 mode) {
  switch(mode % 3) {
  case 0: return "automatic";
  case 1: return "forced";
  case 2: return "off";
  }
  return "unknown";
}

#pragma warn(unused-param, pop)
