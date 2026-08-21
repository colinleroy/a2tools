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
#include "dc50.h"
#include "dc50-read-response.h"
#include "../qt-serial.h"
#include "../../decoders/qt-conv.h"
#include "../../ui/ui.h"

#pragma code-name(push, "DC50")
#pragma rodata-name(push, "DC50")
#pragma data-name(push, "DC50")

/* Camera features */
#define dc50_features 0b0000000010000000
//                              ||||||||_ SET_CAMERA_NAME
//                              |||||||__ SET_CAMERA_TIME
//                              ||||||___ SET_QUALITY,
//                              |||||____ SET_FLASH,
//                              ||||_____ TAKE_PICTURE,
//                              |||______ GET_THUMBNAIL,
//                              ||_______ DELETE_PICTURES,
//                              |________ RESERVED,

/* Camera callbacks definitions */
static uint8 dc50_wakeup(CamSpeed speed);
static uint8 dc50_set_speed(CamSpeed speed);

/* Camera settings functions */
static uint8 dc50_get_information(camera_info *info);

/* Camera pictures functions */
static uint8 dc50_get_picture(uint8 n_pic, int fd, off_t avail);
static uint8 dc50_get_thumbnail(uint8 n_pic, int fd, thumb_info *info);
static void dc50_get_filename(uint8 n_pic, char *dirname, char *filename);

/* Other functions, that this driver doesn't implement
 * but must exist and return -1
 */
static uint8 dc50_set_camera_name(const char *name);
static uint8 dc50_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second);
static uint8 dc50_set_quality(uint8 quality);
static uint8 dc50_set_flash(uint8 mode);
static uint8 dc50_take_picture(void);
static uint8 dc50_delete_pictures(void);

/* Camera thumbnail functions */
void dc50_thumb_histogram(void);
void dc50_load_thumb_data(uint8 line);

static const char *dc50_get_quality_str(uint8 mode);
static const char *dc50_get_flash_str(uint8 mode);

/* Camera callbacks */
void *dc50_callbacks[] = {
  /* FEATURES */        (void *)dc50_features,
  /* WAKEUP */          dc50_wakeup,
  /* SET_SPEED */       dc50_set_speed,
  /* SET_CAMERA_NAME */ dc50_set_camera_name,
  /* SET_CAMERA_TIME */ dc50_set_camera_time,
  /* GET_INFORMATION */ dc50_get_information,
  /* SET_QUALITY */     dc50_set_quality,
  /* SET_FLASH */       dc50_set_flash,
  /* TAKE_PICTURE */    dc50_take_picture,
  /* GET_PICTURE */     dc50_get_picture,
  /* GET_THUMBNAIL */   dc50_get_thumbnail,
  /* DELETE_PICTURES */ dc50_delete_pictures,
  /* GET_FILENAME */    dc50_get_filename,
  /* THUMB_HISTOGRAM */ dc50_thumb_histogram,
  /* THUMB_LOAD_DATA */ dc50_load_thumb_data,
  /* GET_QUALITY_STR */ dc50_get_quality_str,
  /* GET_FLASH_STR */   dc50_get_flash_str,
};

static char command_packet[8];

/* Zero command packet and set command */
void init_packet(char command) {
  command_packet[0] = command;
  bzero(command_packet+1, 6);
  command_packet[7] = 0x1A;
}

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

#pragma warn(unused-param, push, off)
/* Wakeup and detect a DC50 camera
 * Returns 0 if successful, -1 otherwise
 */
static uint8 dc50_wakeup(CamSpeed speed) {
  uint8 c, tries = 3;
  cputs("Pinging Kodak DC50...");

again:
  simple_serial_set_speed(SER_BAUD_9600);
  simple_serial_set_parity(SER_PAR_NONE);

  simple_serial_send_break(250);
  sleep(1);

  /* Flush shit */
  while (simple_serial_read_no_irq((char *)&c, 1) != EOF);

  if (dc50_set_speed(speed) == 0) {
    return QT_MODEL_DC50;
  } else if (tries--) {
    goto again;
  }
  return QT_MODEL_UNKNOWN;
}
#pragma warn(unused-param, pop)

static CamSpeed my_speed = SER_BAUD_9600;

/* Are we reading from card or internal storage */
static uint8 storage_target;

/* Wait for ack after a command requiring it */
static uint8 wait_command_completion(void) {
  char c;
  do {
    while (simple_serial_read_no_irq((char *)&c, 1) == EOF);
    PC_DEBUG("completion", &c, 1);
    if (c == REP_BUSY) {
      continue;
    }
    if (c == REP_COMPLETE) {
      return 0;
    }
    return -1;
  } while (1);
}

/* Send the command (packet previously inited with init_packet()) */
static uint8 dc50_send_command(void) {
  uint8 c;
  PC_DEBUG("CMD", command_packet, 8);

  /* Send */
  simple_serial_write(command_packet, 8);

  /* Wait to get an answer, */
  if (simple_serial_read_no_irq((char *)&c, 1) != 0) {
    PC_DEBUG("No response", &c, 1);
    return -1;
  }

  /* and verify it's accepted */
  PC_DEBUG("response", &c, 1);
  if (c != REP_ACK) {
    return -1;
  }
  return 0;
}

/* Help: send a command and read the reply. Reserved for use with
 * commands that require a single packet response */
static uint8 dc50_send_and_read_response(uint16 response_len) {
  uint8 c;

  if (dc50_send_command() != 0) {
    return -1;
  }

  if (dc50_read_response(response_len) == 0) {
    if (response_len) {
      /* FIXME: verify the checksum */
      simple_serial_putc(REP_CORRECT);
      return wait_command_completion();
    } else {
      return 0;
    }
  } else {
    return -1;
  }
}

/* Send the speed upgrade command */
static uint8 dc50_set_speed(CamSpeed speed) {
  #define SPD_IDX 2

  init_packet(CMD_SET_SPEED);
  switch(speed) {
    case SER_BAUD_9600:
      command_packet[SPD_IDX] = 0x96;
      command_packet[SPD_IDX+1]= 0x00;
      break;

    case SER_BAUD_19200:
      command_packet[SPD_IDX] = 0x19;
      command_packet[SPD_IDX+1]= 0x20;
      break;

    case SER_BAUD_57600:
      command_packet[SPD_IDX] = 0x57;
      command_packet[SPD_IDX+1]= 0x60;
      break;

    case SER_BAUD_115200:
      command_packet[SPD_IDX] = 0x11;
      command_packet[SPD_IDX+1]= 0x52;
      break;
  }

  if (dc50_send_and_read_response(0) != 0) {
    return -1;
  }

  platform_msleep(300);

  /* Toggle speed */
  simple_serial_set_speed(speed);

  /* Verify communication by getting info from the camera */
  if (dc50_get_information(&cam_info) == 0) {
    my_speed = speed;
    return 0;
  }
  return -1;
}

#define BATTERY_STATUS_IDX    8
#define AC_STATUS_IDX         9
#define COMPRESSION_MODE_IDX  19
#define FLASH_MODE_IDX        20
#define NUM_INTERNAL_PIC_IDX  35 // big-endian, 16 bits
#define NUM_CARD_PIC_IDX      51 // big-endian, 16-bits
#define NUM_INTERNAL_LEFT_PIC_IDX 45
#define NUM_CARD_LEFT_PIC_IDX 61
#define CAMERA_NAME_IDX       80

/* Get information from the camera */
static uint8 dc50_get_information(camera_info *info) {
  init_packet(CMD_GET_STATUS);
  if (dc50_send_and_read_response(256) != 0) {
    return -1;
  }

  switch(buffer[BATTERY_STATUS_IDX]) {
  case 0: /* full  */ buffer[BATTERY_STATUS_IDX] = 90; break;
  case 1: /* low   */ buffer[BATTERY_STATUS_IDX] = 50; break;
  case 2: /* empty */ buffer[BATTERY_STATUS_IDX] = 10; break;
  }
  info->battery_level = buffer[BATTERY_STATUS_IDX];

  info->flash_mode = buffer[FLASH_MODE_IDX];
  info->quality_mode = buffer[COMPRESSION_MODE_IDX];

  info->charging      = buffer[AC_STATUS_IDX];
  /* Counter is 16 bits but there won't be more than 256 pictures. */
  if ((info->num_pics = buffer[NUM_CARD_PIC_IDX]) != 0) {
    /* we'll access card memory if there are pics on it. */
    info->left_pics   = buffer[NUM_CARD_LEFT_PIC_IDX];
    storage_target    = PIC_TARGET_CARD;
  } else {
    info->num_pics    = buffer[NUM_INTERNAL_PIC_IDX];
    info->left_pics   = buffer[NUM_INTERNAL_LEFT_PIC_IDX];
    storage_target    = PIC_TARGET_CAM;
  }

  memcpy(info->name, buffer+CAMERA_NAME_IDX, 31);
  info->name[31] = '\0';
  return 0;
}

#define CMD_PIC_NUM 2

static uint8 dc50_get_picture_info(uint8 n_pic) {
  /* Get picture info */
  init_packet(CMD_CAM_PIC_INFO+storage_target);
  command_packet[CMD_PIC_NUM+1] = n_pic;

  return dc50_send_and_read_response(256);
}

#define INFO_BUF_PIC_TYPE_IDX 1
#define INFO_BUF_PIC_NAME_IDX 37

static void dc50_get_filename(uint8 n_pic, char *dirname, char *filename) {
  if (dc50_get_picture_info(n_pic) != 0) {
    sprintf(filename, "%s%sIMAGE%d.JPG",
          IS_NOT_NULL(dirname)?dirname:"",
          IS_NOT_NULL(dirname)?"/":"", n_pic);
  } else {
    sprintf(filename, "%s%s%s.%s",
          IS_NOT_NULL(dirname)?dirname:"",
          IS_NOT_NULL(dirname)?"/":"",
          buffer+INFO_BUF_PIC_NAME_IDX,
          buffer[INFO_BUF_PIC_TYPE_IDX] == 1 ? "KDC":"JPG");
  }
}

#define CAM_PIC_SIZE_IDX 8
#define HEADER_LEN 19712

static uint8 dc50_get_picture(uint8 n_pic, int fd, off_t avail) {
  uint32 pic_size;
  uint8 c, blocks_to_read, d;

  if (dc50_get_picture_info(n_pic) != 0) {
    return -1;
  }

  // FIXME handle card pictures

#ifndef __CC65__
  pic_size     = buffer[CAM_PIC_SIZE_IDX+3] 
               + (buffer[CAM_PIC_SIZE_IDX+2] << 8)
               + (buffer[CAM_PIC_SIZE_IDX+1] << 16);
#else
  /* Get size (24 bits big endian)*/
  ((unsigned char *)&pic_size)[0] = buffer[CAM_PIC_SIZE_IDX+3];
  ((unsigned char *)&pic_size)[1] = buffer[CAM_PIC_SIZE_IDX+2];
  ((unsigned char *)&pic_size)[2] = buffer[CAM_PIC_SIZE_IDX+1];
  ((unsigned char *)&pic_size)[3] = 0;
#endif

  if (pic_size > avail) {
    errno = ENOSPC;
    return -1;
  }


  ui_get_image_str(640, 480, pic_size);

  write(fd, "MM\0*", 4);
  /* Remember quality */
  c = buffer[4] == 0x00 ? 0xF3 : 0x98;

  /* Fill with blank */
  bzero(buffer, sizeof buffer);
  for (d = 0; d <= HEADER_LEN / sizeof buffer; d++) {
    write(fd, buffer, sizeof buffer);
  }

  lseek(fd, 1063, SEEK_SET);
  write(fd, &c, 1);
  lseek(fd, HEADER_LEN, SEEK_SET);

  blocks_to_read = 1+ (pic_size >> 10); /* div 1024 */
  d = 0;
  progress_bar(2, wherey(), scrw - 2, 0, blocks_to_read);

  init_packet(CMD_GET_CAM_PIC+storage_target);
  command_packet[CMD_PIC_NUM+1] = n_pic;
  dc50_send_command();

  while (d++ < blocks_to_read) {
    if (dc50_read_response(1024) == 0) {
      write(fd, buffer, 1024);

      progress_bar(2, wherey(), scrw - 2, d, blocks_to_read);

      /* FIXME verify checksum */
      simple_serial_putc(REP_CORRECT);
    } else {
      return -1;
    }
  }
  return wait_command_completion();
}

static uint8 dc50_get_thumbnail(uint8 n_pic, int fd, thumb_info *info) {
  return -1;
}

#pragma warn(unused-param, push, off)
static uint8 dc50_set_camera_name(const char *name) {
  return -1;
}

static uint8 dc50_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second) {
  return -1;
}

static uint8 dc50_set_quality(uint8 quality) {
  return -1;
}

static uint8 dc50_set_flash(uint8 mode) {
  return -1;
}

static uint8 dc50_take_picture(void) {
  return -1;
}

static uint8 dc50_delete_pictures(void) {
  return -1;
}


static const char *dc50_get_quality_str(uint8 mode) {
  switch (mode % 4) {
    /* we return %s quality, not compression, so invert */
    case 0: return "full";
    case 1: return "high";
    case 2: return "medium";
    case 3: return "low";
  }
  return NULL;
}

static const char *dc50_get_flash_str(uint8 mode) {
  switch(mode % 5) {
    case 2: return "off";
    case 1: return "fill";
    case 4: return "fill red-eye";
    case 0: return "auto";
    case 3: return "auto red-eye";
  }
  return NULL;
}

#pragma warn(unused-param, pop)
