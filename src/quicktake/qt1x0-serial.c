#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#ifndef __CC65__
#include <sys/ioctl.h>
#endif
#include "platform.h"
#include "extended_conio.h"
#include "strtrim.h"
#include "progress_bar.h"
#include "simple_serial.h"
#include "qt-serial.h"
#include "qt-conv.h"
#include "a2_features.h"

extern uint8 scrw, scrh;

static uint8 qt1x0_send_ping(void);

/* Get the ack from the camera */
static uint8 get_ack(uint8 wait) {
  while (wait--) {
    if (simple_serial_getc_with_timeout() == 0x00) {
      return 0;
    }
  }
  return -1;
}

/* Send an ack to the camera */
static void send_ack() {
  simple_serial_putc(0x06);
}

/* Get first data from the camera after connecting */
static uint8 get_hello(void) {
  int c;
  uint8 wait;

  wait = 5;
  while (wait--) {
    c = simple_serial_getc_with_timeout();
    if (c != EOF) {
      goto read;
    }
  }
  return QT_MODEL_UNKNOWN;

read:
  buffer[0] = (unsigned char)c;
  if (buffer[0] != 0xA5) {
    return QT_MODEL_UNKNOWN;
  }
  simple_serial_read((char *)buffer + 1, 6);

  DUMP_START("qt_hello");
  DUMP_DATA(buffer, 7);
  DUMP_END();

  return buffer[3] == 0xC8 ? QT_MODEL_150 : QT_MODEL_100;
}

#pragma code-name(push, "LOWCODE")

/* Send our greeting to the camera, and inform it of the speed
 * we aim for
 */
static uint8 send_hello(uint16 speed) {
  #define SPD_IDX 0x06
  #define CHKSUM_IDX 0x0C
  char str_hello[] = {0x5A,0xA5,0x55,0x05,0x00,0x00,0x25,0x80,0x00,0x80,0x02,0x00,0xFF};
  int c, chk;

  if (speed == 19200) {
    str_hello[SPD_IDX]   = 0x4B;
    str_hello[SPD_IDX+1] = 0x00;
  } else if (speed == 57600U) {
    str_hello[SPD_IDX]   = 0xE1;
    str_hello[SPD_IDX+1] = 0x00;
  }

  for (c = 0, chk = 0; c < CHKSUM_IDX; c++) {
    chk += str_hello[c];
  }
  str_hello[CHKSUM_IDX] = chk & 0xFF;

  DUMP_START("qt_speed");
  DUMP_DATA(str_hello, CHKSUM_IDX+1);
  DUMP_END();

  simple_serial_write(str_hello, sizeof(str_hello));
  if ((c = simple_serial_getc_with_timeout()) == EOF) {
    cputs("Timeout.\r\n");
    return -1;
  }
  if (c != 0x00) {
    printf("Error ($%02x).\n", c);
    return -1;
  }
  buffer[0] = c;
  simple_serial_read((char *)buffer + 1, 9);

  DUMP_START("qt_hello_reply");
  DUMP_DATA(buffer, 10);
  DUMP_END();

  return 0;
}

#pragma code-name(pop)
#pragma code-name(push, "RT_ONCE")
/* Wakeup and detect a QuickTake 100/150 by clearing DTR
 * Returns 0 if successful, -1 otherwise
 */
uint8 qt1x0_wakeup(uint16 speed) {
  static uint8 model = QT_MODEL_UNKNOWN;

  cputs("Pinging QuickTake 1x0... ");
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
    cputs("Timeout. ");
    /* Re-up current port */
    if (is_iigs) {
      simple_serial_dtr_onoff(1);
    }
    return QT_MODEL_UNKNOWN;
  }
  if (send_hello(speed) != 0) {
    return QT_MODEL_UNKNOWN;
  }

  cputs("Done. ");
  return model;
}

/* Send the speed upgrade command */
uint8 qt1x0_set_speed(uint16 speed) {
#define SPD_CMD_IDX 0x0D
  char str_speed[] = {0x16,0x2A,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x05,0x00,0x03,0x03,0x08,0x04,0x00};
  int spd_code;

  /* Seems useless but needed for IIc+ */
  sleep(1);

  switch(speed) {
    case 19200:
#ifdef __CC65__
      spd_code = SER_BAUD_19200;
#else
      spd_code = B19200;
#endif
      str_speed[SPD_CMD_IDX] = 0x10;
      break;

    case 57600U:
#ifdef __CC65__
      spd_code = SER_BAUD_57600;
#else
      spd_code = B57600;
#endif
      str_speed[SPD_CMD_IDX] = 0x30;
      break;

    case 9600:
    default:
      return qt1x0_send_ping();
  }

  printf("Setting speed to %u...\n", speed);
  simple_serial_write(str_speed, sizeof str_speed);

  /* get ack */
  if (get_ack(5) != 0) {
    cputs("Speed set command failed.\r\n");
    return -1;
  }
  send_ack();

  platform_msleep(200);
  simple_serial_set_speed(spd_code);

  /* We don't care about the bytes we receive here */
  simple_serial_flush();

  send_ack();
  return get_ack(5);
}

/* End of RT_ONCE segment */
#pragma code-name(pop)

/* Send a command to the camera */
static uint8 send_command(const char *cmd, uint8 len, uint8 s_ack, uint8 wait) {
  simple_serial_write(cmd, len);

  if (get_ack(wait) != 0) {
    return -1;
  }
  if (s_ack)
    send_ack();

  return 0;
}

/* Ping the camera */
static uint8 qt1x0_send_ping(void) {
  char str[] = {0x16,0x00,0x00,0x00,0x00,0x00,0x00};

  return send_command(str, sizeof str, 0, 5);
}

#define PNUM_IDX       0x06
#define PSIZE_IDX      0x07
#define THUMBNAIL_SIZE 0x0960UL

/* Gets thumbnail of the photo (?)
 * At least, the data received is 2400 bytes long, which correspond
 * to raw 4-bit data for a 80x60 image.
 */
static uint8 send_photo_thumbnail_command(uint8 pnum) {
  //            {????,????,????,FMT?,????,????,PNUM,RESPONSE__SIZE,????}
  char str[] = {0x16,0x28,0x00,0x00,0x00,0x00,0x01,0x00,0x09,0x60,0x00};

  str[PNUM_IDX] = pnum;

  if (qt1x0_send_ping() != 0) {
    return -1;
  }

  return send_command(str, sizeof str, 1, 5);
}

#pragma code-name(push, "LC")

/* Gets photo header */
static uint8 send_photo_header_command(uint8 pnum) {
  //           {????,????,????,FMT?,????,????,PNUM,RESPONSE__SIZE,????}
  char str[] = {0x16,0x28,0x00,0x21,0x00,0x00,0x01,0x00,0x00,0x40,0x00};
  /* Interesting bytes from the header */
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

  str[PNUM_IDX] = pnum;

  if (qt1x0_send_ping() != 0) {
    return -1;
  }

  return send_command(str, sizeof str, 1, 5);
}

/* Gets photo data */
static uint8 send_photo_data_command(uint8 pnum, uint8 *picture_size) {
  //           {????,????,????,FMT?,????,????,PNUM,RESPONSE__SIZE,????}
  char str[] = {0x16,0x28,0x00,0x10,0x00,0x00,0x01,0x00,0x70,0x80,0x00};

  str[PNUM_IDX] = pnum;
  memcpy(str + PSIZE_IDX, picture_size, 3);

  if (qt1x0_send_ping() != 0) {
    return -1;
  }

  return send_command(str, sizeof str, 1, 5);
}

/* Get the camera information summary */
static uint8 send_get_information_command(void) {
  //           {????,????,????,????,????,????,????,RESPONSE__SIZE,????}
  char str[] = {0x16,0x28,0x00,0x30,0x00,0x00,0x00,0x00,0x00,0x80,0x00};

  if (qt1x0_send_ping() != 0) {
    return -1;
  }

  return send_command(str, sizeof str, 1, 5);
}

/* Take a picture */
uint8 qt1x0_take_picture(void) {
  char str[] = {0x16,0x1B,0x00,0x00,0x00,0x00,0x00};

  if (qt1x0_send_ping() != 0) {
    return -1;
  }

  return send_command(str, sizeof str, 0, 20);
}

/* Set the camera name */
uint8 qt1x0_set_camera_name(const char *name) {
  #define NAME_SET_IDX 0x0D
  char str[] = {0x16,0x2a,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x22,0x00,0x02,0x20,
               0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
               0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};
  uint8 len;

  len = strlen(name);
  if (len > 31)
    len = 31;

  if (qt1x0_send_ping() != 0) {
    return - 1;
  }

  memcpy(str + NAME_SET_IDX, name, len);
  return send_command(str, sizeof str, 0, 5);
}

/* Set the camera time */
uint8 qt1x0_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second) {
  #define SET_MONTH_IDX 0x0D
  #define SET_DAY_IDX   0x0E
  #define SET_YEAR_IDX  0x0F
  #define SET_HOUR_IDX  0x10
  #define SET_MIN_IDX   0x11
  #define SET_SEC_IDX   0x12
  //           {                                                                  mon  day year hour  min  sec
  char str[] = {0x16,0x2A,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x01,0x06,0x00,0x00,0x00,0x00,0x00,0x00};

  str[SET_DAY_IDX]   = day;
  str[SET_MONTH_IDX] = month;
  str[SET_YEAR_IDX]  = year;
  str[SET_HOUR_IDX]  = hour;
  str[SET_MIN_IDX]   = minute;
  str[SET_SEC_IDX]   = second;

  if (qt1x0_send_ping() != 0) {
    return -1;
  }

  return send_command(str, sizeof str, 0, 5);
}

static uint8 receive_data(uint32 size, FILE *fp) {
  uint8 y = wherey();
  uint16 i;
  uint8 err = 0;
  uint16 blocks = (uint16)(size / BLOCK_SIZE);
  uint16 rem    = (uint16)(size % BLOCK_SIZE);

  DUMP_START("data");

  cputs("  Getting data...\r\n");

  progress_bar(2, y, scrw - 2, 0, blocks);

  for (i = 0; i < blocks; i++) {
    /* No need to be smart, read more than one block and
     * batch multiple blocks writes, this isn't faster, on
     * the contrary. */
    simple_serial_read((char *)buffer, BLOCK_SIZE);
    if (fwrite(buffer, 1, BLOCK_SIZE, fp) < BLOCK_SIZE) {
      err = -1;
    }
    DUMP_DATA(buffer, BLOCK_SIZE);

    progress_bar(-1, -1, scrw - 2, i, blocks);

    send_ack();
  }

  simple_serial_read((char *)buffer, rem);
  if (fwrite(buffer, 1, rem, fp) < rem) {
    err = -1;
  }

  DUMP_DATA(buffer, rem);

  progress_bar(-1, -1, scrw - 2, 100, 100);

  DUMP_END();
  return err;
}

#define char_to_n_uint16(buf) (((uint8)((buf)[1]))<<8 | ((uint8)((buf)[0])))

/* Get a picture from the camera to a file */
uint8 qt1x0_get_picture(uint8 n_pic, FILE *picture, off_t avail) {
  #define HDR_SKIP       0x04

  #define WH_OFFSET      0x220
  #define DATA_OFFSET    0x2E0

  uint16 width, height;
  unsigned char pic_size_str[3];
  unsigned long pic_size_int;
  uint8 status_line;
  const char *format;
  char hdr[] = {0x00,0x00,0x00,0x04,0x00,0x00,0x73,0xE4,0x00,0x01};

  /* Seems useless but needed for IIc+ */
  sleep(1);

  if (qt1x0_send_ping() != 0) {
    return -1;
  }

  bzero(buffer, BLOCK_SIZE);

  status_line = wherey();
  cputs("  Getting header...\r\n");

  DUMP_START("header");

  if (send_photo_header_command(n_pic) != 0)
    return -1;

  simple_serial_read((char *)buffer, 64);

  DUMP_DATA(buffer, 64);
  DUMP_END();

  /* Get size */
  memcpy(pic_size_str, buffer + IMG_SIZE_IDX, 3);

#ifndef __CC65__
  pic_size_int = (pic_size_str[0]<<16) + (pic_size_str[1]<<8) + (pic_size_str[2]);
#else
  ((unsigned char *)&pic_size_int)[0] = pic_size_str[2];
  ((unsigned char *)&pic_size_int)[1] = pic_size_str[1];
  ((unsigned char *)&pic_size_int)[2] = pic_size_str[0];
  ((unsigned char *)&pic_size_int)[3] = 0;
#endif

  if (pic_size_int > avail) {
    cputs("  Not enough space available.\r\n");
    return -1;
  }

  /* Get dimensions */
  width = char_to_n_uint16(buffer + IMG_WIDTH_IDX);
  height = char_to_n_uint16(buffer  + IMG_HEIGHT_IDX);

  format = QTKN_MAGIC; /* Default to QuickTake 150 format */

  if (serial_model == QT_MODEL_100) {
    format = QTKT_MAGIC;
  }

  /* Copy the header to 0x0E */
  memcpy(buffer+0x0E, buffer+HDR_SKIP, 64-HDR_SKIP);

  /* Write the start of the header */
  memcpy(buffer, format, 4);
  memcpy(buffer+4, hdr, sizeof(hdr));

  /* Set height & width */
  memcpy(buffer+WH_OFFSET, (char*)&height, 2);
  memcpy(buffer+WH_OFFSET+2, (char*)&width, 2);

  /* Write the header to file and seek to data offset. */
  fwrite(buffer, 1, BUFFER_SIZE, picture);
  fseek(picture, DATA_OFFSET, SEEK_SET);

  printf("  Width %u, height %u, %lu bytes (%s)\n",
         ntohs(width), ntohs(height), pic_size_int, format);

  gotoxy(0, status_line);
  cputs("  Getting picture...\r\n");
  gotoy(status_line+2);

  send_photo_data_command(n_pic, pic_size_str);

  return receive_data(pic_size_int, picture);
}

#pragma code-name(pop)
#pragma code-name(push, "LOWCODE")

/* Get a thumnail from the camera to /RAM/THUMBNAIL */
uint8 qt1x0_get_thumbnail(uint8 n_pic, FILE *picture, thumb_info *info) {
  uint8 status_line;

  /* Seems useless but needed for IIc+ */
  sleep(1);

  if (qt1x0_send_ping() != 0) {
    return -1;
  }

  bzero(buffer, BLOCK_SIZE);

  status_line = wherey();
  cputs("  Getting header...\r\n");

  DUMP_START("header");

  if (send_photo_header_command(n_pic) != 0)
    return -1;

  simple_serial_read((char *)buffer, 64);

  DUMP_DATA(buffer, 64);
  DUMP_END();

  info->quality_mode = buffer[IMG_QUALITY_IDX];
  info->flash_mode   = buffer[IMG_FLASH_IDX];
  info->date.year    = buffer[IMG_YEAR_IDX] + 2000;
  info->date.month   = buffer[IMG_MONTH_IDX];
  info->date.day     = buffer[IMG_DAY_IDX];
  info->date.hour    = buffer[IMG_HOUR_IDX];
  info->date.minute  = buffer[IMG_MINUTE_IDX];

  DUMP_START("data");

  printf("  Width %u, height %u, %lu bytes (%s)\n",
         THUMB_WIDTH, THUMB_HEIGHT, THUMBNAIL_SIZE, "thumbnail");

  gotoxy(0, status_line);
  cputs("  Getting thumbnail...\r\n");
  gotoy(status_line+2);
  send_photo_thumbnail_command(n_pic);

  return receive_data(THUMBNAIL_SIZE, picture);
}

/* Delete all pictures from the camera */
uint8 qt1x0_delete_pictures(void) {
  char str[] = {0x16,0x29,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

  if (qt1x0_send_ping() != 0) {
    return -1;
  }

  if (serial_model == QT_MODEL_100)
    return send_command(str, sizeof str, 0, 20);
  else
    return send_command(str, sizeof str, 0, 60);
}

/* Set quality */
uint8 qt1x0_set_quality(uint8 quality) {
  #define SET_QUALITY_IDX 0x0D
  //           {????,????,????,????,????,????,????,????,????,????,????,????,????,QUAL,????}
  char str[] = {0x16,0x2A,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x06,0x02,0x10,0x00};

  if (qt1x0_send_ping() != 0) {
    return -1;
  }
  str[SET_QUALITY_IDX] = (quality == QUALITY_HIGH ? 0x10 : 0x20);

  return send_command(str, sizeof str, 0, 5);
}

/* Set flash mode */
uint8 qt1x0_set_flash(uint8 mode) {
  #define SET_FLASH_IDX 0x0D
  //           {????,????,????,????,????,????,????,????,????,????,????,????,????,FLSH}
  char str[] = {0x16,0x2A,0x00,0x07,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x07,0x01,0x00};

  if (qt1x0_send_ping() != 0) {
    return -1;
  }
  str[SET_FLASH_IDX] = mode;

  return send_command(str, sizeof str, 0, 5);
}

/* Get information from the camera */
uint8 qt1x0_get_information(camera_info *info) {
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

  cputs("Getting information...\r\n");

  DUMP_START("summary");

  if (qt1x0_send_ping() != 0) {
    return -1;
  }

  if (send_get_information_command() != 0)
    return -1;

  simple_serial_read((char *)buffer, 128);

  DUMP_DATA(buffer, 128);
  DUMP_END();


  info->num_pics     = buffer[NUM_PICS_IDX];
  info->left_pics    = buffer[LEFT_PICS_IDX];
  info->quality_mode = buffer[QUAL_IDX];
  info->flash_mode   = buffer[FLASH_IDX];
  info->battery_level= buffer[BATTERY_IDX];
  if (buffer[BATTERY_IDX] > 100) {
    info->battery_level = buffer[BATTERY_IDX] / 2;
    info->charging = 1;
  } else {
    info->battery_level = buffer[BATTERY_IDX];
    info->charging = 0;
  }

  info->date.day    = buffer[DAY_IDX];
  info->date.month  = buffer[MONTH_IDX];
  info->date.year   = buffer[YEAR_IDX] + 2000; /* Year 2256 bug, here we come */
  info->date.hour   = buffer[HOUR_IDX];
  info->date.minute = buffer[MIN_IDX];

  info->name = trim((char *)buffer + NAME_IDX);
  return 0;
}
#pragma code-name(pop)
