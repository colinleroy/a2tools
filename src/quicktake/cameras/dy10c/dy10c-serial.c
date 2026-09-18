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
#include "dy10c.h"
#include "dy10c-read-response.h"
#include "../pc-debug.h"
#include "../qt-serial.h"
#include "../../decoders/qt-conv.h"
#include "../../ui/ui.h"

#pragma code-name(push, "DY10C")
#pragma rodata-name(push, "DY10C")
#pragma data-name(push, "DY10C")
#pragma bss-name(push, "DY10C")

/* Protocol notes. Seems close to the DC 50 but there are catches.
 * First of all Parity is even instead of none.
 * Indexes in info reply are all different.
 * No image header to add (if I mirror the Mac application).
 * No camera name field or setting.
 * Time epoch is different but algorithm is the same.
 * Little-endian whereas DC-50 is big-endian.
 * Apparently no internal/card storage distinction, I hope card
 * info appears automatically at same indexes if a card is
 * inserted. My DC-50 card is rejected ("Er1", can't do anything).
 * Uncharted territory for now.
 */

/* Camera features */
#define dy10c_features 0b0000000011111110
//                               ||||||||_ SET_CAMERA_NAME
//                               |||||||__ SET_CAMERA_TIME
//                               ||||||___ SET_QUALITY,
//                               |||||____ SET_FLASH,
//                               ||||_____ TAKE_PICTURE,
//                               |||______ GET_THUMBNAIL,
//                               ||_______ DELETE_PICTURES,
//                               |________ RESERVED,

/* Camera callbacks definitions */
static uint8 dy10c_wakeup(CamSpeed speed);
static uint8 dy10c_set_speed(CamSpeed speed);

/* Camera settings functions */
static uint8 dy10c_get_information(void);

/* Camera pictures functions */
static uint8 dy10c_get_picture(uint8 n_pic, int fd, off_t avail);
static uint8 dy10c_get_thumbnail(uint8 n_pic, int fd);
static void dy10c_get_filename(uint8 n_pic, char *dirname, char *filename);

/* Other functions, that this driver doesn't implement
 * but must exist and return -1
 */
static uint8 dy10c_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second);
static uint8 dy10c_set_quality(uint8 quality);
static uint8 dy10c_set_flash(uint8 mode);
static uint8 dy10c_take_picture(void);
static uint8 dy10c_delete_pictures(void);

/* Camera thumbnail functions */
void dy10c_thumb_histogram(void);
void dy10c_load_thumb_data(uint8 line);

static const char *dy10c_get_quality_str(uint8 is_pic, uint8 mode);
static const char *dy10c_get_flash_str(uint8 is_pic, uint8 mode);

/* Camera callbacks */
void *dy10c_callbacks[] = {
  /* FEATURES */        (void *)dy10c_features,
  /* WAKEUP */          dy10c_wakeup,
  /* SET_SPEED */       dy10c_set_speed,
  /* SET_CAMERA_NAME */ NULL,
  /* SET_CAMERA_TIME */ dy10c_set_camera_time,
  /* GET_INFORMATION */ dy10c_get_information,
  /* SET_QUALITY */     dy10c_set_quality,
  /* SET_FLASH */       dy10c_set_flash,
  /* TAKE_PICTURE */    dy10c_take_picture,
  /* GET_PICTURE */     dy10c_get_picture,
  /* GET_THUMBNAIL */   dy10c_get_thumbnail,
  /* DELETE_PICTURES */ dy10c_delete_pictures,
  /* GET_FILENAME */    dy10c_get_filename,
  /* THUMB_HISTOGRAM */ dy10c_thumb_histogram,
  /* THUMB_LOAD_DATA */ dy10c_load_thumb_data,
  /* GET_QUALITY_STR */ dy10c_get_quality_str,
  /* GET_FLASH_STR */   dy10c_get_flash_str,
};

static char command_packet[8];

/* Zero command packet and set command */
static void init_packet(char command) {
  command_packet[0] = command;
  bzero(command_packet+1, 6);
  command_packet[7] = 0x1A;
}

extern camera_info cam_info;
extern thumb_info th_info;

#pragma warn(unused-param, push, off)
/* Wakeup and detect a DY10C camera
 * Returns 0 if successful, -1 otherwise
 */
static uint8 dy10c_wakeup(CamSpeed speed) {
  uint8 c, tries = 3;
  cputs("Pinging Kodak DY10C...");

again:
  simple_serial_set_speed(SER_BAUD_9600);
  simple_serial_set_parity(SER_PAR_EVEN);

  simple_serial_send_break(250);
  sleep(1);

  /* Flush shit */
  while (simple_serial_read_no_irq((char *)&c, 1) != EOF);

  if (dy10c_set_speed(speed) == 0) {
    return QT_MODEL_DY10C;
  } else if (tries--) {
    goto again;
  }
  return QT_MODEL_UNKNOWN;
}
#pragma warn(unused-param, pop)

static CamSpeed my_speed = SER_BAUD_9600;

/* Are we reading from card or internal storage */
static uint8 card_present;

/* Wait for ack after a command requiring it */
static uint8 wait_command_completion(void) {
  char c;
  do {
    /* Wait for a character... */
    while (simple_serial_read_no_irq((char *)&c, 1) == EOF);
    PC_DEBUG_PRINTF("completion: %02X\n", c);
    /* Is camera still busy? If so wait */
    if (c == REP_BUSY) {
      continue;
    }
    if (c == REP_COMPLETE) {
      /* We're done! */
      /* Dycam 10-C needs this at least after deletion and time setting */
      sleep(1);
      return 0;
    }
    /* Anything else is an error. */
    return -1;
  } while (1);
}

/* Send the command (packet previously inited with init_packet()) */
static uint8 dy10c_send_command(void) {
  uint8 c;
  PC_DEBUG_BUFFER("CMD", command_packet, 8);

  /* Send */
  simple_serial_write(command_packet, 8);

  /* Wait to get an answer, */
  if (simple_serial_read_no_irq((char *)&c, 1) != 0) {
    PC_DEBUG_PRINTF("No response %02X\n", c);
    return -1;
  }

  /* and verify it's accepted */
  PC_DEBUG_PRINTF("response %02X\n", c);
  if (c != REP_ACK) {
    return -1;
  }
  return 0;
}

/* Help: send a command and read the reply. */
static uint8 dy10c_send_and_read_response(uint8 num_blocks, uint16 block_len) {
  char *dest = (char *)buffer;
  uint8 i = num_blocks;

  if (dy10c_send_command() != 0) {
    return -1;
  }
  while (i--) {
    if (dy10c_read_response(dest, block_len) == 0) {
      dest += block_len;
      if (block_len) {
        /* FIXME: verify the checksum */
        simple_serial_putc(REP_CORRECT);
      }
    } else {
      return -1;
    }
  }
  if (block_len) {
    return wait_command_completion();
  } else {
    return 0;
  }
}

/* Send the speed upgrade command */
static uint8 dy10c_set_speed(CamSpeed speed) {
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

  if (dy10c_send_and_read_response(1, 0) != 0) {
    return -1;
  }

  platform_msleep(300);

  /* Toggle speed */
  simple_serial_set_speed(speed);

  /* Verify communication by getting info from the camera */
  if (dy10c_get_information() == 0) {
    my_speed = speed;
    return 0;
  }
  return -1;
}

#define BATTERY_STATUS_IDX         8
#define AC_STATUS_IDX              9
#define TIME_IDX                   14
#define COMPRESSION_MODE_IDX       23
#define FLASH_MODE_IDX             24
#define TIMER_MODE_IDX             29
#define NUM_INTERNAL_PIC_IDX       9
#define NUM_INTERNAL_PIC_LEFT_IDX  11
// #define NUM_CARD_PIC_IDX      51

#define DY10C_EPOCH            757375200UL  // 31/12/1993 23:00:00

static void dy10c_time_to_camera_date(time_t int_time, camera_date *date) {
  struct tm *tm_time;

  int_time = (int_time >> 1) + DY10C_EPOCH;
  tm_time = localtime(&int_time);
  date->year   = tm_time->tm_year + 1900;
  date->month  = tm_time->tm_mon + 1;
  date->day    = tm_time->tm_mday;
  date->hour   = tm_time->tm_hour;
  date->minute = tm_time->tm_min;
}

/* Get information from the camera */
static uint8 dy10c_get_information(void) {
  time_t int_time;

  init_packet(CMD_GET_STATUS);
  if (dy10c_send_and_read_response(1, 256) != 0) {
    return -1;
  }

  // switch(buffer[BATTERY_STATUS_IDX]) {
  // case 0: /* full  */ buffer[BATTERY_STATUS_IDX] = 90; break;
  // case 1: /* low   */ buffer[BATTERY_STATUS_IDX] = 50; break;
  // case 2: /* empty */ buffer[BATTERY_STATUS_IDX] = 10; break;
  // }
  // cam_info.battery_level = buffer[BATTERY_STATUS_IDX];

  cam_info.flash_mode    = buffer[FLASH_MODE_IDX];
  cam_info.quality_mode  = buffer[COMPRESSION_MODE_IDX];

  // cam_info.charging      = buffer[AC_STATUS_IDX];

  /* FIXME */
  strcpy(cam_info.name, "Dycam 10-C");

  /* Prepare data as if there is a card */
  // cam_info.num_pics   = buffer[NUM_CARD_PIC_IDX];
  // cam_info.left_pics  = buffer[NUM_CARD_PIC_LEFT_IDX];
  // card_present     = 1;
  // cam_info.name[31-8] = '\0'; /* room for " (card)" */
  // strcat(cam_info.name, " (card)");

  /* FIXME handle card */
  if (1) {
    /* No card */
    cam_info.num_pics    = buffer[NUM_INTERNAL_PIC_IDX];
    cam_info.left_pics   = buffer[NUM_INTERNAL_PIC_LEFT_IDX];
    card_present      = 0;
    cam_info.name[31-12] = '\0'; /* room for " (internal)" */
    strcat(cam_info.name, " (internal)");
  }

#ifndef __CC65__
  int_time     = (buffer[TIME_IDX+3] << 24)
               + (buffer[TIME_IDX+2] << 16)
               + (buffer[TIME_IDX+1] << 8)
               + (buffer[TIME_IDX+0] << 0);
#else
  ((unsigned char *)&int_time)[0] = buffer[TIME_IDX+0];
  ((unsigned char *)&int_time)[1] = buffer[TIME_IDX+1];
  ((unsigned char *)&int_time)[2] = buffer[TIME_IDX+2];
  ((unsigned char *)&int_time)[3] = buffer[TIME_IDX+3];
#endif

  dy10c_time_to_camera_date(int_time, &(cam_info.date));
  return 0;
}

#define CMD_PIC_NUM 2

static uint8 dy10c_get_picture_info(uint8 n_pic) {
  /* Get picture info */
  init_packet(card_present ? CMD_CARD_PIC_INFO : CMD_CAM_PIC_INFO);
  command_packet[CMD_PIC_NUM+1] = n_pic;
  if (card_present) {
    return dy10c_send_and_read_response(5, 256);
  } else {
    return dy10c_send_and_read_response(1, 256);
  }
}

#define INFO_BUF_PIC_TYPE_IDX 1
#define INFO_BUF_PIC_NAME_IDX 37

#define INFO_BUF_CARD_NAME_IDX 428 //Hardcoded, should I parse TIFF header? ugh

static void dy10c_get_filename(uint8 n_pic, char *dirname, char *filename) {
  sprintf(filename, "%s%sIMAGE%d.DCT",
        IS_NOT_NULL(dirname)?dirname:"",
        IS_NOT_NULL(dirname)?"/":"", n_pic);
}

#define CAM_PIC_SIZE_IDX 6
#define CARD_PIC_SIZE_IDX 754

#define HEADER_LEN 1280

static uint8 dy10c_get_picture(uint8 n_pic, int fd, off_t avail) {
  uint32 pic_size;
  uint8 c, blocks_to_read, d;

  if (dy10c_get_picture_info(n_pic) != 0) {
    return -1;
  }

  if (card_present) {
    /* FIXME */ exit(1);
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
    pic_size     = (buffer[CAM_PIC_SIZE_IDX+3] << 24)
                 + (buffer[CAM_PIC_SIZE_IDX+2] << 16)
                 + (buffer[CAM_PIC_SIZE_IDX+1] << 8)
                 + (buffer[CAM_PIC_SIZE_IDX]);
#else
    /* Get size */
    ((unsigned char *)&pic_size)[0] = buffer[CAM_PIC_SIZE_IDX];
    ((unsigned char *)&pic_size)[1] = buffer[CAM_PIC_SIZE_IDX+1];
    ((unsigned char *)&pic_size)[2] = buffer[CAM_PIC_SIZE_IDX+2];
    ((unsigned char *)&pic_size)[3] = buffer[CAM_PIC_SIZE_IDX+3];
#endif
  }

  pic_size += HEADER_LEN;

  if (pic_size > avail) {
    errno = ENOSPC;
    return -1;
  }

  ui_get_image_str(640, 480, pic_size);

  blocks_to_read = (pic_size >> 10); /* div 1024 */
  d = 0;
  progress_bar(2, wherey(), scrw - 2, 0, blocks_to_read);

  init_packet(card_present ? CMD_GET_CARD_PIC : CMD_GET_CAM_PIC);
  command_packet[CMD_PIC_NUM+1] = n_pic;
  dy10c_send_command();

  while (d++ < blocks_to_read) {
    PC_DEBUG_PRINTF("Getting block %d/%d (%d)\n", d, blocks_to_read, pic_size);
    if (dy10c_read_response((char *)buffer, 1024) == 0) {
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

static uint8 dy10c_get_thumbnail(uint8 n_pic, int fd) {
  uint8 blocks_to_read, d;
  time_t int_time;
  ui_get_image_header_str();
  if (dy10c_get_picture_info(n_pic) != 0) {
    return -1;
  }

  ui_get_thumbnail_str(n_pic);
  // th_info.flash_mode   = buffer[PIC_FLASH_IDX];   /* 0 = not fired, 1 = fired */
  // th_info.quality_mode = buffer[PIC_QUALITY_IDX]; /* same as cam_get_quality_str */

// #ifndef __CC65__
//   int_time     =  buffer[TIME_IDX+3]
//                + (buffer[TIME_IDX+2] << 8)
//                + (buffer[TIME_IDX+1] << 16)
//                + (buffer[TIME_IDX+0] << 24);
// #else
//   ((unsigned char *)&int_time)[0] = buffer[TIME_IDX+3];
//   ((unsigned char *)&int_time)[1] = buffer[TIME_IDX+2];
//   ((unsigned char *)&int_time)[2] = buffer[TIME_IDX+1];
//   ((unsigned char *)&int_time)[3] = buffer[TIME_IDX+0];
// #endif
//   dy10c_time_to_camera_date(int_time, &(th_info.date));

  init_packet(card_present ? CMD_GET_CARD_THUMB : CMD_GET_CAM_THUMB);
  command_packet[CMD_PIC_NUM+1] = n_pic;
  blocks_to_read = 8;
  d = 0;
  dy10c_send_command();

  while (d++ < blocks_to_read) {
    if (dy10c_read_response((char *)buffer, 1024) == 0) {
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

#pragma warn(unused-param, push, off)
static uint8 dy10c_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second) {
  struct tm date;
  time_t stamp;

  date.tm_mday = day;
  date.tm_mon  = month-1;
  date.tm_year = year+100;
  date.tm_hour = hour;
  date.tm_min  = minute;
  date.tm_sec  = second;

  stamp = mktime(&date);
  stamp -= DY10C_EPOCH;
  stamp <<= 1;

  init_packet(CMD_SET_TIME);
  command_packet[2] = (stamp)       & 0xFF;
  command_packet[3] = (stamp >> 8)  & 0xFF;
  command_packet[4] = (stamp >> 16) & 0xFF;
  command_packet[5] = (stamp >> 24) & 0xFF;

  if (dy10c_send_command() != 0
   || wait_command_completion() != 0) {
     return -1;
  }
  return 0;
}

static uint8 dy10c_command(uint8 command, uint8 param) {
  init_packet(command);
  command_packet[2] = param;
  if (dy10c_send_command() != 0
   || wait_command_completion() != 0) {
     return -1;
   }
  return 0;

}

static uint8 dy10c_set_quality(uint8 quality) {
  return dy10c_command(CMD_SET_QUALITY, quality % 3);
}

static uint8 dy10c_set_flash(uint8 mode) {
  return dy10c_command(CMD_SET_FLASH, mode % 3);
}

static uint8 dy10c_take_picture(void) {
  if (dy10c_command(card_present ? CMD_TAKE_PICTURE_CARD : CMD_TAKE_PICTURE_CAM, 0) != 0) {
    return -1;
  }
  /* Camera returns completion before being ready... */
  sleep(5);
  return 0;
}

static uint8 dy10c_delete_pictures(void) {
  return dy10c_command(card_present ? CMD_DELETE_CARD : CMD_DELETE_CAM, 0);
}


static const char *dy10c_get_quality_str(uint8 is_pic, uint8 mode) {
  if (mode == 0xFF) {
    return "unknown";
  }
  /* we return %s quality, not compression, so invert */
  switch (mode % 3) {
  case 0:  return "high";
  case 1:  return "medium";
  case 2:  return "low";
  }
  #ifndef __CC65__
  return "unknown";
  #endif
}

static const char *dy10c_get_flash_str(uint8 is_pic, uint8 mode) {
  if (mode == 0xFF) {
    return "unknown";
  }
  if (is_pic) {
    return mode ? "on":"off";
  }
  switch(mode % 3) {
  case 0:  return "automatic";
  case 1:  return "forced";
  case 2:  return "disabled";
  }
  #ifndef __CC65__
  return "unknown";
  #endif
}

#pragma warn(unused-param, pop)
