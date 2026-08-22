#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
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
#define dc50_features 0b0000000011100001
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
static uint8 dc50_send_and_read_response(uint8 num_blocks, uint16 response_len) {
  char *dest = (char *)buffer;
  uint8 i = num_blocks;

  if (dc50_send_command() != 0) {
    return -1;
  }
  while (i--) {
    if (dc50_read_response(dest, response_len) == 0) {
      dest += response_len;
      if (response_len) {
        /* FIXME: verify the checksum */
        simple_serial_putc(REP_CORRECT);
      }
    } else {
      return -1;
    }
  }
  if (response_len) {
    return wait_command_completion();
  } else {
    return 0;
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

  if (dc50_send_and_read_response(1, 0) != 0) {
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
#define TIME_IDX              12
#define COMPRESSION_MODE_IDX  19
#define FLASH_MODE_IDX        20
#define NUM_INTERNAL_PIC_IDX  35 // big-endian, 16 bits
#define NUM_CARD_PIC_IDX      51 // big-endian, 16-bits
#define NUM_INTERNAL_LEFT_PIC_IDX 43
#define NUM_CARD_LEFT_PIC_IDX 59
#define CAMERA_NAME_IDX       80

#define DC50_EPOCH            852094800UL  //Wed Jan 01 1997 05:00:00 GMT+0000

/* Get information from the camera */
static uint8 dc50_get_information(camera_info *info) {
  time_t int_time;
  struct tm *tm_time;

  init_packet(CMD_GET_STATUS);
  if (dc50_send_and_read_response(1, 256) != 0) {
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

  memcpy(info->name, buffer+CAMERA_NAME_IDX, 31);
  info->name[31] = '\0';
  /* Counter is 16 bits but there won't be more than 256 pictures. */
  if ((info->num_pics = buffer[NUM_CARD_PIC_IDX]) != 0) {
    /* we'll access card memory if there are pics on it. */
    info->left_pics   = buffer[NUM_CARD_LEFT_PIC_IDX];
    storage_target    = PIC_TARGET_CARD;
    info->name[31-8] = '\0'; /* room for " (card)" */
    strcat(info->name, " (card)");
  } else {
    info->num_pics    = buffer[NUM_INTERNAL_PIC_IDX];
    info->left_pics   = buffer[NUM_INTERNAL_LEFT_PIC_IDX];
    storage_target    = PIC_TARGET_CAM;
    info->name[31-12] = '\0'; /* room for " (internal)" */
    strcat(info->name, " (internal)");
  }

#ifndef __CC65__
  int_time     =  buffer[TIME_IDX+3]
               + (buffer[TIME_IDX+2] << 8)
               + (buffer[TIME_IDX+1] << 16)
               + (buffer[TIME_IDX+0] << 24);
#else
  /* Get size (24 bits big endian)*/
  ((unsigned char *)&int_time)[0] = buffer[TIME_IDX+3];
  ((unsigned char *)&int_time)[1] = buffer[TIME_IDX+2];
  ((unsigned char *)&int_time)[2] = buffer[TIME_IDX+1];
  ((unsigned char *)&int_time)[3] = buffer[TIME_IDX+0];
#endif

  int_time = (int_time >> 1) + DC50_EPOCH;
  tm_time = localtime(&int_time);
  info->date.year   = tm_time->tm_year + 1900;
  info->date.month  = tm_time->tm_mon + 1;
  info->date.day    = tm_time->tm_mday;
  info->date.hour   = tm_time->tm_hour;
  info->date.minute = tm_time->tm_min;

  return 0;
}

#define CMD_PIC_NUM 2

static uint8 dc50_get_picture_info(uint8 n_pic) {
  /* Get picture info */
  init_packet(CMD_CAM_PIC_INFO+storage_target);
  command_packet[CMD_PIC_NUM+1] = n_pic;
  if (storage_target == PIC_TARGET_CAM) {
    return dc50_send_and_read_response(1, 256);
  } else {
    return dc50_send_and_read_response(5, 256);
  }
}

#define INFO_BUF_PIC_TYPE_IDX 1
#define INFO_BUF_PIC_NAME_IDX 37

#define INFO_BUF_CARD_NAME_IDX 428 //Hardcoded, should I parse TIFF header? ugh

static void dc50_get_filename(uint8 n_pic, char *dirname, char *filename) {
  if (dc50_get_picture_info(n_pic) != 0) {
    sprintf(filename, "%s%sIMAGE%d.JPG",
          IS_NOT_NULL(dirname)?dirname:"",
          IS_NOT_NULL(dirname)?"/":"", n_pic);
  } else {
    sprintf(filename, "%s%s%s.KDC",
          IS_NOT_NULL(dirname)?dirname:"",
          IS_NOT_NULL(dirname)?"/":"",
          buffer+(storage_target == PIC_TARGET_CAM ? INFO_BUF_PIC_NAME_IDX : INFO_BUF_CARD_NAME_IDX));
  }
}

#define CAM_PIC_SIZE_IDX 8
#define CARD_PIC_SIZE_IDX 754
#define HEADER_LEN 19712

static uint8 dc50_get_picture(uint8 n_pic, int fd, off_t avail) {
  uint32 pic_size;
  uint8 c, blocks_to_read, d;

  if (dc50_get_picture_info(n_pic) != 0) {
    return -1;
  }

  if (storage_target == PIC_TARGET_CARD) {
#ifndef __CC65__
    pic_size     = buffer[CARD_PIC_SIZE_IDX+3]
                 + (buffer[CARD_PIC_SIZE_IDX+2] << 8)
                 + (buffer[CARD_PIC_SIZE_IDX+1] << 16);
#else
    /* Get size (24 bits big endian)*/
    ((unsigned char *)&pic_size)[0] = buffer[CARD_PIC_SIZE_IDX+3];
    ((unsigned char *)&pic_size)[1] = buffer[CARD_PIC_SIZE_IDX+2];
    ((unsigned char *)&pic_size)[2] = buffer[CARD_PIC_SIZE_IDX+1];
    ((unsigned char *)&pic_size)[3] = 0;
#endif

  } else {
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
  }

  if (pic_size > avail) {
    errno = ENOSPC;
    return -1;
  }

  ui_get_image_str(640, 480, pic_size);

  if (storage_target == PIC_TARGET_CARD) {
    /* Verify data */
    if (memcmp(buffer, "MM\0*", 4)) {
      errno = EINVAL;
      return -1;
    }
    write(fd, buffer, 256*5);
    bzero(buffer, sizeof buffer);
    for (d = 0; d <= (HEADER_LEN-1280) / sizeof buffer; d++) {
      write(fd, buffer, sizeof buffer);
    }
  } else {
    /* "Fix" file header for pictures from cam */
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
  }

  lseek(fd, HEADER_LEN, SEEK_SET);

  blocks_to_read = 1+ (pic_size >> 10); /* div 1024 */
  d = 0;
  progress_bar(2, wherey(), scrw - 2, 0, blocks_to_read);

  init_packet(CMD_GET_CAM_PIC+storage_target);
  command_packet[CMD_PIC_NUM+1] = n_pic;
  dc50_send_command();

  while (d++ < blocks_to_read) {
    if (dc50_read_response((char *)buffer, 1024) == 0) {
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
  uint8 blocks_to_read, d;

  init_packet(CMD_GET_CAM_THUMB+storage_target);
  command_packet[CMD_PIC_NUM+1] = n_pic;
  blocks_to_read = 3;
  d = 0;
  dc50_send_command();

  while (d++ < blocks_to_read) {
    if (dc50_read_response((char *)buffer, 1024) == 0) {
      write(fd, buffer, 1024);

      progress_bar(2, wherey(), scrw - 2, d, blocks_to_read);

      /* FIXME verify checksum */
      simple_serial_putc(REP_CORRECT);
    } else {
      cprintf("err at block %d\n", d);
      break;
    }
  }
  return wait_command_completion();
}

static uint8 dc50_send_packet(uint8 ctrl, uint16 len) {
  uint16 i = 0;
  uint8 chksum = 0;

  PC_DEBUG("Packet header", &ctrl, 1);
  simple_serial_putc(ctrl);
  while (i < len) {
    PC_DEBUG("Packet data", buffer+i, 1);
    simple_serial_putc(buffer[i]);
    chksum ^= buffer[i];
    i++;
  }
  PC_DEBUG("Packet checksum", &chksum, 1);
  simple_serial_putc(chksum);

  if (simple_serial_read_no_irq((char *)&ctrl, 1) == EOF) {
    PC_DEBUG("Timeout on reply", &ctrl, 1);
    return -1;
  }
  PC_DEBUG("Packet send reply", &ctrl, 1);
  if (ctrl != REP_CORRECT) {
    return -1;
  }
  return 0;
}

static uint8 dc50_set_camera_name(const char *name) {
  bzero(buffer, 58);
  memcpy(buffer, name, strlen(name));
  init_packet(CMD_SET_NAME);
  if (dc50_send_command() != 0
   || dc50_send_packet(CTRL_EOF, 58) != 0
   || wait_command_completion() != 0) {
    return -1;
  }
  return 0;
}

#pragma warn(unused-param, push, off)
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
  init_packet(storage_target == PIC_TARGET_CAM ? CMD_DELETE_CAM : CMD_DELETE_CARD);
  if (dc50_send_command() != 0
   || wait_command_completion() != 0) {
     return -1;
   }
  return 0;
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
