#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#ifndef __CC65__
#include <sys/ioctl.h>
#endif
#include "a2_features.h"
#include "platform.h"
#include "extended_conio.h"
#include "strtrim.h"
#include "progress_bar.h"
#include "simple_serial.h"
#include "../qt-serial.h"
#include "../qt-thumbs.h"
#include "../../decoders/qt-conv.h"
#include "../../ui/ui.h"

#pragma code-name(push, "QT1X0")
#pragma rodata-name(push, "QT1X0")
#pragma data-name(push, "QT1X0")
#pragma bss-name(push, "QT1X0")

/* Camera features */
#define qt1x0_features 0b0000000011111111
//                               ||||||||_ SET_CAMERA_NAME
//                               |||||||__ SET_CAMERA_TIME
//                               ||||||___ SET_QUALITY,
//                               |||||____ SET_FLASH,
//                               ||||_____ TAKE_PICTURE,
//                               |||______ GET_THUMBNAIL,
//                               ||_______ DELETE_PICTURES,
//                               |________ RESERVED,

/* Camera callbacks definitions */
static uint8 qt1x0_wakeup(CamSpeed speed);
static uint8 qt1x0_set_speed(CamSpeed speed);

/* Camera settings functions */
static uint8 qt1x0_set_camera_name(const char *name);
static uint8 qt1x0_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second);
static uint8 qt1x0_get_information(void);
static uint8 qt1x0_set_quality(uint8 quality);
static uint8 qt1x0_set_flash(uint8 mode);

/* Camera pictures functions */
static uint8 qt1x0_take_picture(void);
static uint8 qt1x0_get_picture(uint8 n_pic, int fd, off_t avail);
static uint8 qt1x0_get_thumbnail(uint8 n_pic, int fd);
static uint8 qt1x0_delete_pictures(void);
static void qt1x0_get_filename(uint8 n_pic, char *dirname, char *filename);

/* Camera thumbnail functions */
void qt1x0_thumb_histogram(void);
void qt1x0_load_thumb_data(uint8 line);

/* Modes strings */
static const char *qt1x0_get_quality_str(uint8 is_pic, uint8 mode);
static const char *qt1x0_get_flash_str(uint8 is_pic, uint8 mode);

/* Camera callbacks */  
void *qt1x0_callbacks[] = {
  /* FEATURES */        (void *)qt1x0_features,
  /* WAKEUP */          qt1x0_wakeup,
  /* SET_SPEED */       qt1x0_set_speed,
  /* SET_CAMERA_NAME */ qt1x0_set_camera_name,
  /* SET_CAMERA_TIME */ qt1x0_set_camera_time,
  /* GET_INFORMATION */ qt1x0_get_information,
  /* SET_QUALITY */     qt1x0_set_quality,
  /* SET_FLASH */       qt1x0_set_flash,
  /* TAKE_PICTURE */    qt1x0_take_picture,
  /* GET_PICTURE */     qt1x0_get_picture,
  /* GET_THUMBNAIL */   qt1x0_get_thumbnail,
  /* DELETE_PICTURES */ qt1x0_delete_pictures,
  /* GET_FILENAME */    qt1x0_get_filename,
  /* THUMB_HISTOGRAM */ qt1x0_thumb_histogram,
  /* THUMB_LOAD_DATA */ qt1x0_load_thumb_data,
  /* GET_QUALITY_STR */ qt1x0_get_quality_str,
  /* GET_FLASH_STR */   qt1x0_get_flash_str,
};

extern uint8 scrw, scrh;
extern uint8 do_debug;

#define QUALITY_HIGH     0x10
#define QUALITY_STANDARD 0x20
#define QUALITY_UNKNOWN  0xFF

#define FLASH_AUTO       0
#define FLASH_OFF        1
#define FLASH_ON         2
#define FLASH_UNKNOWN    0xFF

extern camera_info cam_info;
extern thumb_info th_info;

uint8 is_qt100; /* helper for thumbnailer */
#ifdef DEBUG_THUMB
  #if DEBUG_THUMB==100
  uint8 is_qt100 = 1;
  #endif
  #if DEBUG_THUMB==150
  uint8 is_qt100 = 0;
  #endif
#endif

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

static void qt1x0_get_filename(uint8 n_pic, char *dirname, char *filename) {
  sprintf(filename, "%s%sIMAGE%d.QTK",
          IS_NOT_NULL(dirname)?dirname:"",
          IS_NOT_NULL(dirname)?"/":"", n_pic);
}

/* Get the ack from the camera */
static uint8 get_ack(uint8 wait) {
  char c;
  while (wait--) {
    if (simple_serial_read_no_irq(&c, 1) == 0x00 && c == 0x00) {
      PC_DEBUG("ack OK", &c, 1);
      return 0;
    }
    PC_DEBUG("ack", &c, 1);
  }
  return -1;
}

/* Send an ack to the camera */
static void send_ack(void) {
  PC_DEBUG("sack", NULL, 0);
  simple_serial_putc(0x06);
}

/* Send a command to the camera */
static uint8 send_command(const char *cmd, uint8 len, uint8 ping, uint8 s_ack, uint8 wait) {
  static char ping_str[] = {0x16,0x00,0x00,0x00,0x00,0x00,0x00};
  if (ping) {
    PC_DEBUG("ping", ping_str, sizeof ping_str);
    simple_serial_write(ping_str, sizeof ping_str);
    if (get_ack(5) != 0)
      return -1;
  }

  if (len == 0) {
    return 0;
  }

  PC_DEBUG("write", cmd, len);
  simple_serial_write(cmd, len);
  if (get_ack(wait) != 0)
    return -1;

  if (s_ack)
    send_ack();

  return 0;
}

/* Get first data from the camera after connecting */
static uint8 get_hello(void) {
  
  if (simple_serial_read_no_irq((char *)buffer, 7) == EOF) {
    cputs("Timeout. ");
    return QT_MODEL_UNKNOWN;
  }
  PC_DEBUG("read hello", buffer, 7);

  if (buffer[0] != 0xA5) {
    return QT_MODEL_UNKNOWN;
  }

  return buffer[3] == 0xC8 ? QT_MODEL_150 : QT_MODEL_100;
}

/* Send our greeting to the camera, and inform it of the speed
 * we aim for
 */
static uint8 send_hello(CamSpeed speed) {
  #define SPD_IDX 0x06
  #define CHKSUM_IDX 0x0C
  static char str_hello[] = {0x5A,0xA5,0x55,0x05,0x00,0x00,0x25,0x80,0x00,0x80,0x02,0x00,0xFF};
  unsigned char chk, c;

  if (speed == SER_BAUD_19200) {
    str_hello[SPD_IDX]   = 0x4B;
    str_hello[SPD_IDX+1] = 0x00;
  } else { /* if (speed == SER_BAUD_57600) */
    str_hello[SPD_IDX]   = 0xE1;
    str_hello[SPD_IDX+1] = 0x00;
  }

  for (c = 0, chk = 0; c < CHKSUM_IDX; c++) {
    chk += str_hello[c];
  }
  str_hello[CHKSUM_IDX] = chk;

  PC_DEBUG("send hello", str_hello, sizeof str_hello);
  simple_serial_write(str_hello, sizeof str_hello);
  if (simple_serial_read_no_irq((char *)buffer, 10) == EOF) {
    cputs("Timeout. ");
    return -1;
  }
  PC_DEBUG("read hello reply", buffer, 10);

  if (buffer[0] != 0x00) {
    cprintf("Error ($%02X).\r\n", c);
    return -1;
  }

  return 0;
}

/* Wakeup and detect a QuickTake 100/150 by clearing DTR
 * Returns 0 if successful, -1 otherwise
 */
static uint8 qt1x0_wakeup(CamSpeed speed) {
  static uint8 model = QT_MODEL_UNKNOWN;

  cputs("Pinging QuickTake 1x0... ");

  simple_serial_set_speed(SER_BAUD_9600);
  simple_serial_set_parity(SER_PAR_NONE);

  /* The Apple II printer port being closed right now,
   * we have to set DTR before clearing it.
   */
#ifdef __CC65__
  if (!is_iigs) {
    simple_serial_slot_dtr_onoff(ser_params.printer_slot, 1);
    sleep(1);
    simple_serial_slot_dtr_onoff(ser_params.printer_slot, 0);
  } else
#endif
  {
    simple_serial_dtr_onoff(0);
  }

  if ((model = get_hello()) == QT_MODEL_UNKNOWN) {
    /* Re-up current port */
    if (is_iigs) {
      simple_serial_dtr_onoff(1);
    }
    return QT_MODEL_UNKNOWN;
  }

  is_qt100 = (model == QT_MODEL_100);

  if (speed == SER_BAUD_115200) {
    /* Use max possible speed */
    speed = is_iigs ? SER_BAUD_57600:SER_BAUD_19200;
  }
  if (send_hello(speed) != 0) {
    return QT_MODEL_UNKNOWN;
  }

  simple_serial_set_parity(SER_PAR_EVEN);
  cputs("Done. ");

  return model;
}

/* Send the speed upgrade command */
static uint8 qt1x0_set_speed(CamSpeed speed) {
#define SPD_CMD_IDX 0x0D
  static char str_speed[] = {0x16,0x2A,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x05,0x00,0x03,0x03,0x08,0x04,0x00};

  /* Seems useless but needed for IIc+ */
  sleep(1);

  if (speed == SER_BAUD_115200) {
    /* Use max possible speed */
    speed = is_iigs ? SER_BAUD_57600:SER_BAUD_19200;
  }

  switch(speed) {
    case SER_BAUD_19200:
      str_speed[SPD_CMD_IDX] = 0x10;
      break;

    case SER_BAUD_57600:
      str_speed[SPD_CMD_IDX] = 0x30;
      break;

    case SER_BAUD_9600:
    default:
      /* just ping */
      return send_command(NULL, 0, 1, 0, 0);
  }

  PC_DEBUG("write speed", str_speed, sizeof str_speed);
  if (send_command(str_speed, sizeof str_speed, 0, 1, 5) != 0) {
    cputs("Speed set command failed.\r\n");
    return -1;
  }

  platform_msleep(200);
  simple_serial_set_speed(speed);

  /* We don't care about the bytes we receive here */
  while(simple_serial_read_no_irq((char *)buffer, 256) != EOF);

  send_ack();
  return get_ack(5);
}

#define PNUM_IDX       0x06
#define PSIZE_IDX      0x07
#define FMT_IDX        0x03

#define HEADER_SIZE    0x40
#define INFO_SIZE      0x80
#define THUMBNAIL_SIZE 0x0960UL

#define INFORMATION    0x30
#define PHOTO_HEADER   0x21
#define PHOTO_FULL     0x10
#define PHOTO_THUMB    0x00

/* Responses indexes */
#define IMG_NUM_IDX     0x03
#define IMG_SIZE_IDX    0x05
#define IMG_WIDTH_IDX   0x08
#define IMG_HEIGHT_IDX  0x0A
#define IMG_MONTH_IDX   0x0D
#define IMG_DAY_IDX     0x0E
#define IMG_YEAR_IDX    0x0F
#define IMG_HOUR_IDX    0x10
#define IMG_MINUTE_IDX  0x11
#define IMG_SECOND_IDX  0x12
#define IMG_FLASH_IDX   0x13
#define IMG_QUALITY_IDX 0x18 /* (?) */

/* Gets photo header */
#define send_photo_header_command(pnum) \
        send_photo_data_command(pnum, PHOTO_HEADER, HEADER_SIZE)

#define send_get_information_command() \
        send_photo_data_command(0, INFORMATION, INFO_SIZE)

/* Gets photo data */
static uint8 send_photo_data_command(uint8 pnum, uint8 format, uint32 picture_size) {
  //                  {????,????,????,FMT ,????,????,PNUM,RESPONSE__SIZE,????}
  static char str[] = {0x16,0x28,0x00,0x10,0x00,0x00,0x01,0x00,0x70,0x80,0x00};

  str[PNUM_IDX]    = pnum;
  str[FMT_IDX]     = format;
  str[PSIZE_IDX]   = (picture_size >> 16) & 0xFF;
  str[PSIZE_IDX+1] = (picture_size >> 8)  & 0xFF;
  str[PSIZE_IDX+2] = (picture_size)       & 0xFF;

  return send_command(str, sizeof str, 1, 1, 5);
}

/* Take a picture */
static uint8 qt1x0_take_picture(void) {
  static char str[] = {0x16,0x1B,0x00,0x00,0x00,0x00,0x00};

  return send_command(str, sizeof str, 1, 0, 20);
}

/* Set the camera name */
static uint8 qt1x0_set_camera_name(const char *name) {
  #define NAME_SET_IDX 0x0D
  /* Not static so shorter names get spaces after them */
  char str[] = {0x16,0x2a,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x22,0x00,0x02,0x20,
               0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
               0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};
  uint8 len;

  len = strlen(name);
  if (len > 31)
    len = 31;

  memcpy(str + NAME_SET_IDX, name, len);

  return send_command(str, sizeof str, 1, 0, 5);
}

static uint8 receive_data(uint8 n_pic, uint8 type, uint32 size, int fd) {
  uint16 i;
  uint8 err = 0;
  uint16 blocks = (uint16)(size / BLOCK_SIZE);
  uint16 rem    = (uint16)(size % BLOCK_SIZE);

  progress_bar(2, wherey(), scrw - 2, 0, blocks);

  send_photo_data_command(n_pic, type, size);

  for (i = 0; i < blocks; i++) {
    /* No need to be smart, read more than one block and
     * batch multiple blocks writes, this isn't faster, on
     * the contrary. */
    if (simple_serial_read_no_irq((char *)buffer, BLOCK_SIZE) == EOF) {
      errno = EBUSY;
      return -1;
    }
    if (write(fd, buffer, BLOCK_SIZE) < BLOCK_SIZE) {
      err = -1;
      errno = EIO;
      /* Write error. But keep reading from serial,
       * otherwise we'll crash the camera. */
    }
    progress_bar(-1, -1, scrw - 2, i, blocks);

    send_ack();
  }

  if (simple_serial_read_no_irq((char *)buffer, rem) == EOF) {
    errno = EBUSY;
    return -1;
  }
  if (write(fd, buffer, rem) < rem) {
    err = -1;
    errno = EIO;
  }

  progress_bar(-1, -1, scrw - 2, 100, 100);

  return err;
}

#define char_to_n_uint16(buf) (((uint8)((buf)[1]))<<8 | ((uint8)((buf)[0])))

/* Get a picture from the camera to a file */
static uint8 qt1x0_get_picture(uint8 n_pic, int fd, off_t avail) {
  #define HDR_SKIP       0x04

  #define WH_OFFSET      0x220
  #define DATA_OFFSET    0x2E0

  uint16 width, height;
  unsigned long pic_size_int;
  static char hdr[] = {0x00,0x00,0x00,0x04,0x00,0x00,0x73,0xE4,0x00,0x01};

  /* Seems useless but needed for IIc+ */
  sleep(1);

  bzero(buffer, BLOCK_SIZE);

  ui_get_image_header_str();
  if (send_photo_header_command(n_pic) != 0) {
    errno = EIO;
    return -1;
  }

  if (simple_serial_read_no_irq((char *)buffer, 64) == EOF) {
    errno = EBUSY;
    return -1;
  }

#ifndef __CC65__
  pic_size_int = buffer[IMG_SIZE_IDX+2] 
               + (buffer[IMG_SIZE_IDX+1] << 8)
               + (buffer[IMG_SIZE_IDX+0] << 16);
#else
  /* Get size (24 bits big endian)*/
  ((unsigned char *)&pic_size_int)[0] = buffer[IMG_SIZE_IDX+2];
  ((unsigned char *)&pic_size_int)[1] = buffer[IMG_SIZE_IDX+1];
  ((unsigned char *)&pic_size_int)[2] = buffer[IMG_SIZE_IDX+0];
  ((unsigned char *)&pic_size_int)[3] = 0;
#endif

  if (pic_size_int > avail) {
    errno = ENOSPC;
    return -1;
  }

  /* Get dimensions */
  width = char_to_n_uint16(buffer + IMG_WIDTH_IDX);
  height = char_to_n_uint16(buffer  + IMG_HEIGHT_IDX);

  memcpy(buffer, serial_model == QT_MODEL_100 ? QKTK_MAGIC : QKTN_MAGIC, 4);

  /* Copy the header to 0x0E */
  memcpy(buffer+0x0E, buffer+HDR_SKIP, 64-HDR_SKIP);

  /* Write the start of the header */
  memcpy(buffer+4, hdr, sizeof(hdr));

  /* Set height & width */
  memcpy(buffer+WH_OFFSET, (char*)&height, 2);
  memcpy(buffer+WH_OFFSET+2, (char*)&width, 2);

  /* Write the header to file and seek to data offset. */
  write(fd, buffer, BUFFER_SIZE);
  lseek(fd, DATA_OFFSET, SEEK_SET);

  ui_get_image_str(ntohs(width), ntohs(height), pic_size_int);

  return receive_data(n_pic, PHOTO_FULL, pic_size_int, fd);
}

/* Get a thumnail from the camera to /RAM/THUMBNAIL */
static uint8 qt1x0_get_thumbnail(uint8 n_pic, int fd) {

  /* Seems useless but needed for IIc+ */
  sleep(1);

  bzero(buffer, BLOCK_SIZE);

  ui_get_image_header_str();
  if (send_photo_header_command(n_pic) != 0) {
    errno = EIO;
    return -1;
  }

  if (simple_serial_read_no_irq((char *)buffer, 64) == EOF) {
    errno = EBUSY;
    return -1;
  }

  th_info.quality_mode = buffer[IMG_QUALITY_IDX] == QUALITY_HIGH ? 0:1;
  th_info.flash_mode   = buffer[IMG_FLASH_IDX];
  th_info.date.year    = buffer[IMG_YEAR_IDX] + 2000;
  th_info.date.month   = buffer[IMG_MONTH_IDX];
  th_info.date.day     = buffer[IMG_DAY_IDX];
  th_info.date.hour    = buffer[IMG_HOUR_IDX];
  th_info.date.minute  = buffer[IMG_MINUTE_IDX];

  ui_get_thumbnail_str(n_pic);

  return receive_data(n_pic, PHOTO_THUMB, THUMBNAIL_SIZE, fd);
}

/* Delete all pictures from the camera */
static uint8 qt1x0_delete_pictures(void) {
  static char str[] = {0x16,0x29,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

  return send_command(str, sizeof str, 1, 0, 60);
}

/* Set the camera time */
static uint8 qt1x0_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second) {
  #define SET_MONTH_IDX 0x0D
  #define SET_DAY_IDX   0x0E
  #define SET_YEAR_IDX  0x0F
  #define SET_HOUR_IDX  0x10
  #define SET_MIN_IDX   0x11
  #define SET_SEC_IDX   0x12
  //                  {                                                                  mon  day year hour  min  sec
  static char str[] = {0x16,0x2A,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x01,0x06,0x00,0x00,0x00,0x00,0x00,0x00};

  str[SET_DAY_IDX]   = day;
  str[SET_MONTH_IDX] = month;
  str[SET_YEAR_IDX]  = year;
  str[SET_HOUR_IDX]  = hour;
  str[SET_MIN_IDX]   = minute;
  str[SET_SEC_IDX]   = second;

  return send_command(str, sizeof str, 1, 0, 5);
}

/* Set quality */
static uint8 qt1x0_set_quality(uint8 quality) {
  #define SET_QUALITY_IDX 0x0D
  //                  {????,????,????,????,????,????,????,????,????,????,????,????,????,QUAL,????}
  static char str[] = {0x16,0x2A,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x06,0x02,0x10,0x00};

  str[SET_QUALITY_IDX] = (quality % 2 == 0) ? QUALITY_HIGH:QUALITY_STANDARD;

  return send_command(str, sizeof str, 1, 0, 5);
}

/* Set flash mode */
static uint8 qt1x0_set_flash(uint8 mode) {
  #define SET_FLASH_IDX 0x0D
  //                  {????,????,????,????,????,????,????,????,????,????,????,????,FLSH,????}
  static char str[] = {0x16,0x2A,0x00,0x07,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x07,0x01,0x00};

  str[SET_FLASH_IDX] = (mode % 3);

  return send_command(str, sizeof str, 1, 0, 5);
}

/* Get information from the camera */
static uint8 qt1x0_get_information(void) {
  static char c;
  #define BATTERY_IDX    0x02 /* ?? 0xA7 = charging, full ; 0x63 = not charging, full */
  #define NUM_PICS_IDX   0x04
  #define LEFT_PICS_IDX  0x06
  #define MONTH_IDX      0x10
  #define DAY_IDX        0x11
  #define YEAR_IDX       0x12
  #define HOUR_IDX       0x13
  #define MIN_IDX        0x14
  #define SEC_IDX        0x15
  #define FLASH_IDX      0x16
  #define QUAL_IDX       0x1B
  #define NAME_IDX       0x2F

  if (send_get_information_command() != 0) {
    errno = EIO;
    return -1;
  }

  if (simple_serial_read_no_irq((char *)buffer, 128) == EOF) {
    errno = EBUSY;
    return -1;
  }
  PC_DEBUG("read information", buffer, 128);

  cam_info.num_pics     = buffer[NUM_PICS_IDX];
  cam_info.left_pics    = buffer[LEFT_PICS_IDX];
  cam_info.quality_mode = buffer[QUAL_IDX] == QUALITY_HIGH ? 0:1;
  cam_info.flash_mode   = buffer[FLASH_IDX];

  if (buffer[BATTERY_IDX] > 100) {
    cam_info.battery_level = buffer[BATTERY_IDX] / 2;
    cam_info.charging = 1;
  } else {
    cam_info.battery_level = buffer[BATTERY_IDX];
    cam_info.charging = 0;
  }

  cam_info.date.day    = buffer[DAY_IDX];
  cam_info.date.month  = buffer[MONTH_IDX];
  cam_info.date.year   = buffer[YEAR_IDX] + 2000; /* Year 2256 bug, here we come */
  cam_info.date.hour   = buffer[HOUR_IDX];
  cam_info.date.minute = buffer[MIN_IDX];

  for (c = 31; c ; c--) {
    if ((buffer+NAME_IDX)[c] != 0x20) {
      (buffer+NAME_IDX)[++c] = 0x00;
      break;
    }
  }
  strcpy(cam_info.name, (char *)(buffer + NAME_IDX));

  return 0;
}


static const char *qt1x0_get_quality_str(uint8 is_pic, uint8 mode) {
  switch(mode % 2) {
  case 0:  return "high";
  case 1:  return "standard";
  default: return "unknown";
  }
}

static const char *qt1x0_get_flash_str(uint8 is_pic, uint8 mode) {
  if (is_pic) {
    return mode == 1 ? "on":"off";
  }
  switch(mode % 3) {
  case FLASH_AUTO: return "automatic";
  case FLASH_OFF:  return "disabled";
  case FLASH_ON:   return "forced";
  default:         return "unknown";
  }
}
