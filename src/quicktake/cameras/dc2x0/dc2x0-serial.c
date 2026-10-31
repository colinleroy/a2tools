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
#include "dc2x0.h"
#include "dc2x0-read-response.h"
#include "../pc-debug.h"
#include "../qt-serial.h"
#include "../../decoders/qt-conv.h"
#include "../../ui/ui.h"

#pragma code-name(push, "DC2X0")
#pragma rodata-name(push, "DC2X0")
#pragma data-name(push, "DC2X0")
#pragma bss-name(push, "DC2X0")

/* Camera features */
#define dc2x0_features 0b0000000011111111
//                               ||||||||_ SET_CAMERA_NAME
//                               |||||||__ SET_CAMERA_TIME
//                               ||||||___ SET_QUALITY,
//                               |||||____ SET_FLASH,
//                               ||||_____ TAKE_PICTURE,
//                               |||______ GET_THUMBNAIL,
//                               ||_______ DELETE_PICTURES,
//                               |________ RESERVED,

/* Camera callbacks definitions */
static uint8 dc2x0_wakeup(CamSpeed speed);
static uint8 dc2x0_set_speed(CamSpeed speed);

/* Camera settings functions */
static uint8 dc2x0_get_information(void);

/* Camera pictures functions */
static uint8 dc2x0_get_picture(uint8 n_pic, int fd, off_t avail);
static uint8 dc2x0_get_thumbnail(uint8 n_pic, int fd);
static void dc2x0_get_filename(uint8 n_pic, char *dirname, char *filename);

/* Other functions, that this driver doesn't implement
 * but must exist and return -1
 */
static uint8 dc2x0_set_camera_name(const char *name);
static uint8 dc2x0_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second);
static uint8 dc2x0_set_quality(uint8 quality);
static uint8 dc2x0_set_flash(uint8 mode);
static uint8 dc2x0_take_picture(void);
static uint8 dc2x0_delete_pictures(void);

/* Camera thumbnail functions */
void dc2x0_thumb_histogram(void);
void dc2x0_load_thumb_data(uint8 line);

static const char *dc2x0_get_quality_str(uint8 is_pic, uint8 mode);
static const char *dc2x0_get_flash_str(uint8 is_pic, uint8 mode);

/* Camera callbacks */
void *dc2x0_callbacks[] = {
  /* FEATURES */        (void *)dc2x0_features,
  /* WAKEUP */          dc2x0_wakeup,
  /* SET_SPEED */       dc2x0_set_speed,
  /* SET_CAMERA_NAME */ dc2x0_set_camera_name,
  /* SET_CAMERA_TIME */ dc2x0_set_camera_time,
  /* GET_INFORMATION */ dc2x0_get_information,
  /* SET_QUALITY */     dc2x0_set_quality,
  /* SET_FLASH */       dc2x0_set_flash,
  /* TAKE_PICTURE */    dc2x0_take_picture,
  /* GET_PICTURE */     dc2x0_get_picture,
  /* GET_THUMBNAIL */   dc2x0_get_thumbnail,
  /* DELETE_PICTURES */ dc2x0_delete_pictures,
  /* GET_FILENAME */    dc2x0_get_filename,
  /* THUMB_HISTOGRAM */ dc2x0_thumb_histogram,
  /* THUMB_LOAD_DATA */ dc2x0_load_thumb_data,
  /* GET_QUALITY_STR */ dc2x0_get_quality_str,
  /* GET_FLASH_STR */   dc2x0_get_flash_str,
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

/* Send the command (packet previously inited with init_packet()) */
static uint8 dc2x0_send_command(void) {
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
    /* We're done! */
    if (c == REP_COMPLETE) {
      return 0;
    }
    /* Anything else is an error. */
    return -1;
  } while (1);
}

static uint8 dx2x0_initialize(void) {
  init_packet(CMD_INITIALIZE);
  if (dc2x0_send_command() != 0) {
    return -1;
  }
  return wait_command_completion();
}

#pragma warn(unused-param, push, off)
/* Wakeup and detect a dc2x0 camera
 * Returns 0 if successful, -1 otherwise
 */
static uint8 dc2x0_wakeup(CamSpeed speed) {
  uint8 c, tries = 3;
  cputs("Pinging Kodak DC2x0...");

again:
  simple_serial_set_speed(SER_BAUD_9600);
  simple_serial_set_parity(SER_PAR_NONE);

  simple_serial_send_break(500);
  sleep(1);

  /* Flush shit */
  while (simple_serial_read_no_irq((char *)&c, 1) != EOF);

  if (dx2x0_initialize() != 0) {
    return QT_MODEL_UNKNOWN;
  }
  if (dc2x0_set_speed(speed) == 0) {
    return QT_MODEL_DC2X0;
  } else if (tries--) {
    goto again;
  }
  return QT_MODEL_UNKNOWN;
}
#pragma warn(unused-param, pop)

static CamSpeed my_speed = SER_BAUD_9600;

/* Help: send a command and read the reply. */
static uint8 dc2x0_send_and_read_response(uint8 num_blocks, uint16 block_len) {
  char *dest = (char *)buffer;
  uint8 i = num_blocks;

  if (dc2x0_send_command() != 0) {
    return -1;
  }
  while (i--) {
    if (dc2x0_read_response(dest, block_len) == 0) {
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
static uint8 dc2x0_set_speed(CamSpeed speed) {
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

  if (dc2x0_send_command() != 0) {
    return -1;
  }

  platform_msleep(600);

  /* Toggle speed */
  simple_serial_set_speed(speed);

  /* Verify communication by getting info from the camera */
  if (dc2x0_get_information() == 0) {
    my_speed = speed;
    return 0;
  }
  return -1;
}

#define CAMERA_TYPE_IDX       1
#define BATTERY_STATUS_IDX    8
#define AC_STATUS_IDX         9
#define TIME_IDX              12
#define COMPRESSION_MODE_IDX  19
#define FLASH_MODE_IDX        20
#define PICTURE_SIZE_IDX      22
#define FILE_TYPE_IDX         23
#define TIMER_MODE_IDX        29
#define NUM_CARD_PIC_IDX      57
#define CAMERA_NAME_IDX       90

#define PIC_QUALITY_IDX       4
#define PIC_FLASH_IDX         16

#define DC2X0_EPOCH           852073200UL //Wed Jan 01 1997 00:00:00 GMT+0100

static void dc2x0_time_to_camera_date(time_t int_time, camera_date *date) {
  struct tm *tm_time;

  int_time = (int_time >> 1) + DC2X0_EPOCH;
  tm_time = localtime(&int_time);
  date->year   = tm_time->tm_year + 1900;
  date->month  = tm_time->tm_mon + 1;
  date->day    = tm_time->tm_mday;
  date->hour   = tm_time->tm_hour;
  date->minute = tm_time->tm_min;
}

/* Camera tells how many pics left on each quality setting */
static uint8 pics_left_on_card[] = {59, 61, 63};
static uint8 pics_left_on_cam[] = {43, 45, 47};

/* Get information from the camera */
static uint8 dc2x0_get_information(void) {
  time_t int_time;

  init_packet(CMD_GET_STATUS);
  if (dc2x0_send_and_read_response(1, 256) != 0) {
    return -1;
  }

  switch(buffer[BATTERY_STATUS_IDX]) {
  case 0: /* full  */ cam_info.battery_level = 90; break;
  case 1: /* low   */ cam_info.battery_level = 50; break;
  case 2: /* empty */ cam_info.battery_level = 10; break;
  }

  cam_info.flash_mode    = buffer[FLASH_MODE_IDX];
  cam_info.quality_mode  = buffer[COMPRESSION_MODE_IDX] - 1; /* Index 0-2 instead of 1-3 */

  cam_info.charging      = buffer[AC_STATUS_IDX];

  if (buffer[CAMERA_NAME_IDX]) {
    memcpy(cam_info.name, buffer+CAMERA_NAME_IDX, 31);
  } else {
    strcpy(cam_info.name, "Kodak DC2x0");
  }
  cam_info.name[31] = '\0';

  /* Prepare data as if there is a card */
  cam_info.num_pics   = buffer[NUM_CARD_PIC_IDX];
  cam_info.left_pics  = buffer[pics_left_on_card[cam_info.quality_mode]];

#ifndef __CC65__
  int_time     =  buffer[TIME_IDX+3]
               + (buffer[TIME_IDX+2] << 8)
               + (buffer[TIME_IDX+1] << 16)
               + (buffer[TIME_IDX+0] << 24);
#else
  ((unsigned char *)&int_time)[0] = buffer[TIME_IDX+3];
  ((unsigned char *)&int_time)[1] = buffer[TIME_IDX+2];
  ((unsigned char *)&int_time)[2] = buffer[TIME_IDX+1];
  ((unsigned char *)&int_time)[3] = buffer[TIME_IDX+0];
#endif

  dc2x0_time_to_camera_date(int_time, &(cam_info.date));
  return 0;
}

#define CMD_PIC_NUM 2

static uint8 dc2x0_get_picture_info(uint8 n_pic) {
  /* Get picture info */
  init_packet(CMD_CARD_PIC_INFO);
  command_packet[CMD_PIC_NUM+1] = n_pic - 1;
  return dc2x0_send_and_read_response(1, 256);
}

#define INFO_BUF_PIC_TYPE_IDX 1
#define INFO_BUF_PIC_NAME_IDX 32

static void dc2x0_get_filename(uint8 n_pic, char *dirname, char *filename) {
  if (dc2x0_get_picture_info(n_pic) != 0) {
    sprintf(filename, "%s%sIMAGE%d.JPG",
          IS_NOT_NULL(dirname)?dirname:"",
          IS_NOT_NULL(dirname)?"/":"", n_pic);
  } else {
    sprintf(filename, "%s%s%s",
          IS_NOT_NULL(dirname)?dirname:"",
          IS_NOT_NULL(dirname)?"/":"",
          buffer+(INFO_BUF_PIC_NAME_IDX));
  }
}

#define CAM_PIC_SIZE_IDX 8

static uint8 dc2x0_get_picture(uint8 n_pic, int fd, off_t avail) {
  uint32 pic_size;
  uint8 blocks_to_read, d;

  if (dc2x0_get_picture_info(n_pic) != 0) {
    return -1;
  }

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
  PC_DEBUG_PRINTF("Picture size %d\n", pic_size);
  ui_get_image_str(640, 480, pic_size);

  blocks_to_read = 1+ (pic_size >> 10); /* div 1024 */
  d = 0;
  progress_bar(2, wherey(), scrw - 2, 0, blocks_to_read);

  init_packet(CMD_GET_CARD_PIC);
  command_packet[CMD_PIC_NUM+1] = n_pic-1;
  dc2x0_send_command();

  while (d++ < blocks_to_read) {
    if (dc2x0_read_response((char *)buffer, 1024) == 0) {
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

static uint8 dc2x0_get_thumbnail(uint8 n_pic, int fd) {
  uint8 blocks_to_read, d;
  time_t int_time;
  ui_get_image_header_str();
  if (dc2x0_get_picture_info(n_pic) != 0) {
    return -1;
  }

  ui_get_thumbnail_str(n_pic);
  th_info.flash_mode   = buffer[PIC_FLASH_IDX];   /* 0 = not fired, 1 = fired */
  th_info.quality_mode = buffer[PIC_QUALITY_IDX]; /* same as cam_get_quality_str */

#ifndef __CC65__
  int_time     =  buffer[TIME_IDX+3]
               + (buffer[TIME_IDX+2] << 8)
               + (buffer[TIME_IDX+1] << 16)
               + (buffer[TIME_IDX+0] << 24);
#else
  ((unsigned char *)&int_time)[0] = buffer[TIME_IDX+3];
  ((unsigned char *)&int_time)[1] = buffer[TIME_IDX+2];
  ((unsigned char *)&int_time)[2] = buffer[TIME_IDX+1];
  ((unsigned char *)&int_time)[3] = buffer[TIME_IDX+0];
#endif
  dc2x0_time_to_camera_date(int_time, &(th_info.date));

  init_packet(CMD_GET_CARD_THUMB);
  command_packet[CMD_PIC_NUM+1] = n_pic-1;
  blocks_to_read = 4;
  d = 0;
  dc2x0_send_command();

  while (d++ < blocks_to_read) {
    if (dc2x0_read_response((char *)buffer, 1024) == 0) {
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

static uint8 dc2x0_send_packet(uint8 ctrl, uint16 len) {
  uint16 i = 0;
  uint8 chksum = 0;

  PC_DEBUG_PRINTF("Packet header %02X\n", ctrl);
  simple_serial_putc(ctrl);
  PC_DEBUG_BUFFER("Packet data", buffer, len);
  while (i < len) {
    simple_serial_putc(buffer[i]);
    chksum ^= buffer[i];
    i++;
  }
  simple_serial_putc(chksum);
  PC_DEBUG_PRINTF("Packet checksum %02X\n", chksum);

  if (simple_serial_read_no_irq((char *)&ctrl, 1) == EOF) {
    PC_DEBUG_PRINTF("Timeout on reply %02X\n", ctrl);
    return -1;
  }
  PC_DEBUG_PRINTF("Packet send reply %02X\n", ctrl);
  if (ctrl != REP_CORRECT) {
    return -1;
  }
  return 0;
}

static uint8 dc2x0_set_camera_name(const char *name) {
  bzero(buffer, 58);
  memcpy(buffer, name, strlen(name));
  init_packet(CMD_SET_NAME);
  if (dc2x0_send_command() != 0
   || dc2x0_send_packet(CTRL_EOF, 58) != 0
   || wait_command_completion() != 0) {
    return -1;
  }
  return 0;
}

#pragma warn(unused-param, push, off)
static uint8 dc2x0_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second) {
  struct tm date;
  time_t stamp;

  date.tm_mday = day;
  date.tm_mon  = month-1;
  date.tm_year = year+100;
  date.tm_hour = hour;
  date.tm_min  = minute;
  date.tm_sec  = second;

  stamp = mktime(&date);
  stamp -= DC2X0_EPOCH;
  stamp <<= 1;

  init_packet(CMD_SET_TIME);
  command_packet[2] = (stamp >> 24) & 0xFF;
  command_packet[3] = (stamp >> 16) & 0xFF;
  command_packet[4] = (stamp >> 8)  & 0xFF;
  command_packet[5] = (stamp)       & 0xFF;

  if (dc2x0_send_command() != 0
   || wait_command_completion() != 0) {
     return -1;
   }
  return 0;
}

static uint8 dc2x0_command(uint8 command, uint8 param) {
  init_packet(command);
  command_packet[2] = param;
  if (dc2x0_send_command() != 0
   || wait_command_completion() != 0) {
     return -1;
   }
  return 0;

}

static uint8 dc2x0_set_quality(uint8 quality) {
  quality = (quality % 3) + 1; /* Indexed 1-3 */
  return dc2x0_command(CMD_SET_QUALITY, quality);
}

static uint8 dc2x0_set_flash(uint8 mode) {
  return dc2x0_command(CMD_SET_FLASH, mode % 3);
}

static uint8 dc2x0_take_picture(void) {
  if (dc2x0_command(CMD_TAKE_PICTURE_CARD, 0) != 0) {
    return -1;
  }
  /* Camera returns completion before being ready... */
  sleep(5);
  return 0;
}

static uint8 dc2x0_delete_pictures(void) {
  return dc2x0_command(CMD_DELETE_CARD, 0);
}


static const char *dc2x0_get_quality_str(uint8 is_pic, uint8 mode) {
  if (mode == 0xFF) {
    return "unknown";
  }
  // 1=best, 2= better, 3 = good
  /* we return %s quality, not compression, so invert */
  switch (mode % 3) {
  case 0:  return "best";
  case 1:  return "better";
  case 2:  return "good";
  }
  #ifndef __CC65__
  return "unknown";
  #endif
}

static const char *dc2x0_get_flash_str(uint8 is_pic, uint8 mode) {
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
