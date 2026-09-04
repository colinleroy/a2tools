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
#pragma bss-name(push, "DC50")

/* Camera features */
#define dc50_features 0b0000000011111111
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
static uint8 dc50_get_information(void);

/* Camera pictures functions */
static uint8 dc50_get_picture(uint8 n_pic, int fd, off_t avail);
static uint8 dc50_get_thumbnail(uint8 n_pic, int fd);
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

static const char *dc50_get_quality_str(uint8 is_pic, uint8 mode);
static const char *dc50_get_flash_str(uint8 is_pic, uint8 mode);

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

extern camera_info cam_info;
extern thumb_info th_info;

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
static uint8 card_present;

/* Wait for ack after a command requiring it */
static uint8 wait_command_completion(void) {
  char c;
  do {
    /* Wait for a character... */
    while (simple_serial_read_no_irq((char *)&c, 1) == EOF);
    PC_DEBUG("completion", &c, 1);
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

/* Help: send a command and read the reply. */
static uint8 dc50_send_and_read_response(uint8 num_blocks, uint16 block_len) {
  char *dest = (char *)buffer;
  uint8 i = num_blocks;

  if (dc50_send_command() != 0) {
    return -1;
  }
  while (i--) {
    if (dc50_read_response(dest, block_len) == 0) {
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
  if (dc50_get_information() == 0) {
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
#define TIMER_MODE_IDX        29
#define NUM_INTERNAL_PIC_IDX  35
#define NUM_CARD_PIC_IDX      51
#define CAMERA_NAME_IDX       80

#define PIC_QUALITY_IDX       4
#define PIC_FLASH_IDX         16

#define DC50_EPOCH            852094800UL  //Wed Jan 01 1997 05:00:00 GMT+0000

static void dc50_time_to_camera_date(time_t int_time, camera_date *date) {
  struct tm *tm_time;

  int_time = (int_time >> 1) + DC50_EPOCH;
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
static uint8 dc50_get_information(void) {
  time_t int_time;

  init_packet(CMD_GET_STATUS);
  if (dc50_send_and_read_response(1, 256) != 0) {
    return -1;
  }

  switch(buffer[BATTERY_STATUS_IDX]) {
  case 0: /* full  */ buffer[BATTERY_STATUS_IDX] = 90; break;
  case 1: /* low   */ buffer[BATTERY_STATUS_IDX] = 50; break;
  case 2: /* empty */ buffer[BATTERY_STATUS_IDX] = 10; break;
  }
  cam_info.battery_level = buffer[BATTERY_STATUS_IDX];

  cam_info.flash_mode    = buffer[FLASH_MODE_IDX];
  cam_info.quality_mode  = buffer[COMPRESSION_MODE_IDX];

  cam_info.charging      = buffer[AC_STATUS_IDX];

  memcpy(cam_info.name, buffer+CAMERA_NAME_IDX, 31);
  cam_info.name[31] = '\0';

  /* Prepare data as if there is a card */
  cam_info.num_pics   = buffer[NUM_CARD_PIC_IDX];
  cam_info.left_pics  = buffer[pics_left_on_card[cam_info.quality_mode]];
  card_present     = 1;
  cam_info.name[31-8] = '\0'; /* room for " (card)" */
  strcat(cam_info.name, " (card)");

  if (cam_info.num_pics == 0 && cam_info.left_pics == 0) {
    /* No card */
    cam_info.num_pics    = buffer[NUM_INTERNAL_PIC_IDX];
    cam_info.left_pics   = buffer[pics_left_on_cam[cam_info.quality_mode]];
    card_present      = 0;
    cam_info.name[31-12] = '\0'; /* room for " (internal)" */
    strcat(cam_info.name, " (internal)");
  }

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

  dc50_time_to_camera_date(int_time, &(cam_info.date));
  return 0;
}

#define CMD_PIC_NUM 2

static uint8 dc50_get_picture_info(uint8 n_pic) {
  /* Get picture info */
  init_packet(card_present ? CMD_CARD_PIC_INFO : CMD_CAM_PIC_INFO);
  command_packet[CMD_PIC_NUM+1] = n_pic;
  if (card_present) {
    return dc50_send_and_read_response(5, 256);
  } else {
    return dc50_send_and_read_response(1, 256);
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
          buffer+(card_present ? INFO_BUF_CARD_NAME_IDX : INFO_BUF_PIC_NAME_IDX));
  }
}

#define CAM_PIC_SIZE_IDX 8
#define CARD_PIC_SIZE_IDX 754

/* Hardcoded even if loses space, because dcraw hardcodes it, and I want my
 * downloads to be decodable by dcraw. */
#define HEADER_LEN 19712

static uint8 minimal_exif_header[] = {
    // TIFF header, big endian
    0x4D, 0x4D, 0x00, 0x2A,
    // IFD0 offset
    0x00, 0x00, 0x00, 0x08,

    // IFD0: 3 tags
    0x00, 0x03,

    // Tag 1: Make (0x010F), ASCII, count=5, offset=0x0070
    0x01, 0x0F,
    0x00, 0x02,
    0x00, 0x00, 0x00, 0x05,
    0x00, 0x00, 0x00, 0x70,

    // Tag 2: Model (0x0110), ASCII, count=10, offset=0x0086
    0x01, 0x10,
    0x00, 0x02,
    0x00, 0x00, 0x00, 0x0A,
    0x00, 0x00, 0x00, 0x76,

    // Tag 3: SubIFD (0x014A), LONG, offset=0x0032
    0x01, 0x4A,
    0x00, 0x04,
    0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x32,

    // Next IFD pointer: nope
    0x00, 0x00, 0x00, 0x00,

    // --- SubIFD at offset 0x0032 ---
    0x00, 0x04,   // 4 tags

    // Width (0x0100), SHORT = 756
    0x01, 0x00,
    0x00, 0x03,
    0x00, 0x00, 0x00, 0x01,
    0x02, 0xF4, 0x00, 0x00,

    // Height (0x0101), SHORT = 504
    0x01, 0x01,
    0x00, 0x03,
    0x00, 0x00, 0x00, 0x01,
    0x01, 0xF8, 0x00, 0x00,

    // StripOffset (0x0111), LONG = 19712 (HEADER_LEN)
    0x01, 0x11,
    0x00, 0x04,
    0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x4D, 0x00,

    // CompressedBitsPerPixel (0x9102), RATIONAL, offset=0x0068
    0x91, 0x02,
    0x00, 0x05,
    0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x68,

    // Next IFD pointer: nope
    0x00, 0x00, 0x00, 0x00,

    // RATIONAL data at offset 104 (CompressedBitsPerPixel)
    0x00, 0x00, 0x00, 0xFF,   // numerator, to be set according to image quality (At 0x6B)
    0x00, 0x00, 0x00, 0x64,   // denominator = 100

    // ASCII data at offset 112 (Make)
    'K','o','d','a','k', 0x00,

    // ASCII data at offset 118 (Model)
    'K','o','d','a','k',' ','D','C','5','0', 0x00
};

static uint8 dc50_get_picture(uint8 n_pic, int fd, off_t avail) {
  uint32 pic_size;
  uint8 c, blocks_to_read, d;

  if (dc50_get_picture_info(n_pic) != 0) {
    return -1;
  }

  if (card_present) {
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

  if (pic_size + HEADER_LEN > avail) {
    errno = ENOSPC;
    return -1;
  }

  ui_get_image_str(640, 480, pic_size);

  /* Images from card come with the EXIF header. From internal storage,
   * they don't. Adapt accordingly. */
  if (card_present) {
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
    write(fd, minimal_exif_header, sizeof(minimal_exif_header));
    /* Remember quality */
    c = buffer[4] == 0x00 ? 0xF3 : 0x98;

    /* Fill with blank */
    bzero(buffer, sizeof buffer);
    for (d = 0; d <= HEADER_LEN / sizeof buffer; d++) {
      write(fd, buffer, sizeof buffer);
    }

    /* 0x6B is the CompressedBitsPerPixel offset in our minimal EXIF header. */
    lseek(fd, 0x6B, SEEK_SET);
    write(fd, &c, 1);
  }

  lseek(fd, HEADER_LEN, SEEK_SET);

  blocks_to_read = 1+ (pic_size >> 10); /* div 1024 */
  d = 0;
  progress_bar(2, wherey(), scrw - 2, 0, blocks_to_read);

  init_packet(card_present ? CMD_GET_CARD_PIC : CMD_GET_CAM_PIC);
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

static uint8 dc50_get_thumbnail(uint8 n_pic, int fd) {
  uint8 blocks_to_read, d;

  ui_get_image_header_str();
  if (dc50_get_picture_info(n_pic) != 0) {
    return -1;
  }

  ui_get_thumbnail_str(n_pic);
  th_info.flash_mode   = buffer[PIC_FLASH_IDX];   /* 0 = not fired, 1 = fired */
  th_info.quality_mode = buffer[PIC_QUALITY_IDX]; /* same as cam_get_quality_str */

  init_packet(card_present ? CMD_GET_CARD_THUMB : CMD_GET_CAM_THUMB);
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
      return -1;
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
  struct tm date;
  time_t stamp;

  date.tm_mday = day;
  date.tm_mon  = month-1;
  date.tm_year = year+100;
  date.tm_hour = hour;
  date.tm_min  = minute;
  date.tm_sec  = second;

  stamp = mktime(&date);
  stamp -= DC50_EPOCH;
  stamp <<= 1;

  init_packet(CMD_SET_TIME);
  command_packet[2] = (stamp >> 24) & 0xFF;
  command_packet[3] = (stamp >> 16) & 0xFF;
  command_packet[4] = (stamp >> 8)  & 0xFF;
  command_packet[5] = (stamp)       & 0xFF;

  if (dc50_send_command() != 0
   || wait_command_completion() != 0) {
     return -1;
   }
  return 0;
}

static uint8 dc50_command(uint8 command, uint8 param) {
  init_packet(command);
  command_packet[2] = param;
  if (dc50_send_command() != 0
   || wait_command_completion() != 0) {
     return -1;
   }
  return 0;

}

static uint8 dc50_set_quality(uint8 quality) {
  return dc50_command(CMD_SET_QUALITY, quality % 3);
}

static uint8 dc50_set_flash(uint8 mode) {
  return dc50_command(CMD_SET_FLASH, mode % 3);
}

static uint8 dc50_take_picture(void) {
  if (dc50_command(card_present ? CMD_TAKE_PICTURE_CARD : CMD_TAKE_PICTURE_CAM, 0) != 0) {
    return -1;
  }
  /* Camera returns completion before being ready... */
  sleep(5);
  return 0;
}

static uint8 dc50_delete_pictures(void) {
  return dc50_command(card_present ? CMD_DELETE_CARD : CMD_DELETE_CAM, 0);
}


static const char *dc50_get_quality_str(uint8 is_pic, uint8 mode) {
  if (mode == 0xFF) {
    return "unknown";
  }
  /* we return %s quality, not compression, so invert */
  switch (mode % 3) {
  case 0:  return "high";
  case 1:  return "medium";
  case 2:  return "low";
  }
}

static const char *dc50_get_flash_str(uint8 is_pic, uint8 mode) {
  if (mode == 0xFF) {
    return "unknown";
  }
  switch(mode % 3) {
  case 0:  return "automatic";
  case 1:  return "forced";
  case 2:  return "disabled";
  }
}

#pragma warn(unused-param, pop)
