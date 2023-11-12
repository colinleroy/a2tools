#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "platform.h"
#include "extended_conio.h"
#include "extended_string.h"
#include "progress_bar.h"
#include "simple_serial.h"
#include "qt-serial.h"
#include "qt-conv.h"

extern uint8 scrw, scrh;

#pragma code-name(push, "LC")

/* Get the ack from the camera */
static uint8 get_ack(void) {
  uint8 wait = 20;

  /* about 10seconds wait */
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
  printf("Timeout.\n");
  return -1;
read:
  buffer[0] = (unsigned char)c;
  simple_serial_read(buffer + 1, 6);

  DUMP_START("qt_hello");
  DUMP_DATA(buffer, 7);
  DUMP_END();
  printf("Done.\n");
  return 0;
}

/* Send our greeting to the camera, and inform it of the speed
 * we aim for
 */
static uint8 send_hello(uint16 speed) {
  #define SPD_IDX 0x06
  #define CHK_IDX 0x0C
  char str_hello[] = {0x5A,0xA5,0x55,0x05,0x00,0x00,0x25,0x80,0x00,0x80,0x02,0x00,0x80};

  DUMP_START("qt_hello_reply");
  if (speed == 19200) {
    str_hello[SPD_IDX]   = 0x4B;
    str_hello[SPD_IDX+1] = 0x00;
    str_hello[CHK_IDX]   = 0x26;
  } else if (speed == 57600U) {
    str_hello[SPD_IDX]   = 0xE1;
    str_hello[SPD_IDX+1] = 0x00;
    str_hello[CHK_IDX]   = 0xBC;
  }

  simple_serial_write(str_hello, sizeof(str_hello));
  simple_serial_read(buffer, 10);

  DUMP_DATA(buffer, 10);
  DUMP_END();

  return 0;
}

/* Send a command to the camera */
static uint8 send_command(const char *cmd, uint8 len, uint8 s_ack) {
  simple_serial_write(cmd, len);

  if (get_ack() != 0) {
    return -1;
  }
  if (s_ack)
    send_ack();

  return 0;
}

/* Ping the camera */
static uint8 qt1x0_send_ping(void) {
  char str[] = {0x16,0x00,0x00,0x00,0x00,0x00,0x00};

  return send_command(str, sizeof str, 0);
}

#define PNUM_IDX       0x06
#define PSIZE_IDX      0x07
#define THUMBNAIL_SIZE 0x0960

/* Gets thumbnail of the photo (?)
 * At least, the data received is 2400 bytes long, which correspond
 * to raw 4-bit data for a 80x60 image.
 */
static uint8 send_photo_thumbnail_command(uint8 pnum) {
  //            {????,????,????,????,????,????,PNUM,RESPONSE__SIZE,????}
  char str[] = {0x16,0x28,0x00,0x00,0x00,0x00,0x01,0x00,0x09,0x60,0x00};

  str[PNUM_IDX] = pnum;

  if (qt1x0_send_ping() != 0) {
    return -1;
  }

  return send_command(str, sizeof str, 1);
}

/* Gets photo header */
static uint8 send_photo_header_command(uint8 pnum) {
  //           {????,????,????,????,????,????,PNUM,RESPONSE__SIZE,????}
  char str[] = {0x16,0x28,0x00,0x21,0x00,0x00,0x01,0x00,0x00,0x40,0x00};

  str[PNUM_IDX] = pnum;

  if (qt1x0_send_ping() != 0) {
    return -1;
  }

  return send_command(str, sizeof str, 1);
}

/* Gets photo data */
static uint8 send_photo_data_command(uint8 pnum, uint8 *picture_size) {
  //           {????,????,????,????,????,????,PNUM,RESPONSE__SIZE,????}
  char str[] = {0x16,0x28,0x00,0x10,0x00,0x00,0x01,0x00,0x70,0x80,0x00};

  str[PNUM_IDX] = pnum;
  memcpy(str + PSIZE_IDX, picture_size, 3);

  if (qt1x0_send_ping() != 0) {
    return -1;
  }

  return send_command(str, sizeof str, 1);
}

/* Get the camera information summary */
static uint8 send_get_information_command(void) {
  //           {????,????,????,????,????,????,????,RESPONSE__SIZE,????}
  char str[] = {0x16,0x28,0x00,0x30,0x00,0x00,0x00,0x00,0x00,0x80,0x00};

  if (qt1x0_send_ping() != 0) {
    return -1;
  }

  return send_command(str, sizeof str, 1);
}

/* Wakeup and detect a QuickTake 100/150 by clearing DTR
 * Returns 0 if successful, -1 otherwise
 */
uint8 qt1x0_wakeup(uint16 speed) {
#if defined(__CC65__) && !defined(IIGS)
    /* The Apple IIc printer being closed right now,
     * we have to set DTR before clearing it. */
    printf("Toggling printer port on...\n");
    simple_serial_acia_onoff(1, 1);
    platform_sleep(1);
    printf("Toggling printer port off...\n");
    simple_serial_acia_onoff(1, 0);
#else
  printf("Toggling DTR off...\n");
  simple_serial_dtr_onoff(0);
#endif

  printf("Waiting for QuickTake 1x0 hello... ");
  if (get_hello() != 0) {
#if !defined(__CC65__) || defined(IIGS)
    /* Re-up current port */
    printf("Toggling DTR on...\n");
    simple_serial_dtr_onoff(1);
#endif
    return -1;
  }
  printf("Sending hello...\n");
  send_hello(speed);

  printf("Connected to QuickTake 1x0.\n");
  return 0;
}

/* Send the speed upgrade command */
uint8 qt1x0_set_speed(uint16 speed, int first_sleep, int second_sleep) {
#define SPD_CMD_IDX 0x0D
  char str_speed[] = {0x16,0x2A,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x05,0x00,0x03,0x03,0x08,0x04,0x00};
  int spd_code;

  switch(speed) {
    case 9600:
      return qt1x0_send_ping();

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
  }

  /* This part is very sensitive to timing and I
   * didn't yet figure out how to make things square.
   */
  platform_sleep(first_sleep);

  printf("Setting speed to %u...\n", speed);
  simple_serial_write(str_speed, sizeof str_speed);

  /* get ack */
  if (get_ack() != 0) {
    printf("Speed set command failed.\n");
    return -1;
  }
  send_ack();

  platform_msleep(second_sleep);
  simple_serial_set_speed(spd_code);

  /* We don't care about the bytes we receive here */
  simple_serial_flush();

  send_ack();
  return get_ack();
}

/* Take a picture */
uint8 qt1x0_take_picture(void) {
  char str[] = {0x16,0x1B,0x00,0x00,0x00,0x00,0x00};

  if (qt1x0_send_ping() != 0) {
    return -1;
  }

  return send_command(str, sizeof str, 0);
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
  return send_command(str, sizeof str, 0);
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

  return send_command(str, sizeof str, 0);
}

static void receive_data(uint32 size, FILE *fp) {
  uint8 y = wherey();
  uint16 i;

  DUMP_START("data");

  printf("  Getting data...\n");

  progress_bar(2, y, scrw - 2, 0, (uint16)(size / BLOCK_SIZE));

  for (i = 0; i < (uint16)(size / BLOCK_SIZE); i++) {

    simple_serial_read(buffer, BLOCK_SIZE);
    fwrite(buffer, 1, BLOCK_SIZE, fp);
    DUMP_DATA(buffer, BLOCK_SIZE);

    progress_bar(-1, -1, scrw - 2, i, (uint16)(size / BLOCK_SIZE));

    send_ack();
  }
  simple_serial_read(buffer, (uint16)(size % BLOCK_SIZE));
  fwrite(buffer, 1, size % BLOCK_SIZE, fp);
  DUMP_DATA(buffer, size % BLOCK_SIZE);

  progress_bar(-1, -1, scrw - 2, 100, 100);

  DUMP_END();
}

#define char_to_n_uint16(buf) (((uint8)((buf)[1]))<<8 | ((uint8)((buf)[0])))

/* Get a picture from the camera to a file */
uint8 qt1x0_get_picture(uint8 n_pic, const char *filename) {
  /* Interesting bytes from the header */
  #define IMG_NUM_IDX    0x03
  #define IMG_SIZE_IDX   0x05
  #define IMG_WIDTH_IDX  0x08
  #define IMG_HEIGHT_IDX 0x0A
  #define IMG_MONTH_IDX  0x0D
  #define IMG_DAY_IDX    0x0E
  #define IMG_YEAR_IDX   0x0F
  #define IMG_HOUR_IDX   0x10
  #define IMG_MINUTE_IDX 0x11
  #define IMG_SECOND_IDX 0x12
  #define IMG_FLASH_IDX  0x13
  #define QUALITY_IDX    0x18 /* (?) */
  #define HDR_SKIP       0x04

  #define WH_OFFSET      0x220
  #define DATA_OFFSET    0x2E0

  FILE *picture;
  uint16 width, height;
  unsigned char pic_size_str[3];
  unsigned long pic_size_int;
  const char *format;

  platform_sleep(1);

  if (qt1x0_send_ping() != 0) {
    return -1;
  }

  picture = fopen(filename,"wb");

  memset(buffer, 0, BLOCK_SIZE);

  printf("  Getting header...\n");

  DUMP_START("header");

  if (send_photo_header_command(n_pic) != 0)
    return -1;

  simple_serial_read(buffer, 64);

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

  /* Get dimensions */
  width = char_to_n_uint16(buffer + IMG_WIDTH_IDX);
  height = char_to_n_uint16(buffer  + IMG_HEIGHT_IDX);

  format = QTKN_MAGIC; /* Default to QuickTake 150 format */

  /* QuickTake 150 pictures are better compressed
   * FIXME: This is a bad way to detect format
   */
  if (ntohs(width) == 640 && pic_size_int == 115200UL) {
    format = QTKT_MAGIC;
  }
  if (ntohs(width) == 320 && pic_size_int == 28800UL) {
    format = QTKT_MAGIC;
  }

  /* Write the start of the header */
  write_qtk_header(picture, format);
  fwrite(buffer, 1, BLOCK_SIZE, picture);
  fwrite(buffer, 1, BLOCK_SIZE, picture);

  /* Write the rest of the header */
  fseek(picture, 0x0E, SEEK_SET);
  fwrite(buffer + HDR_SKIP, 1, 64 - HDR_SKIP, picture);

  /* Set them in the file */
  fseek(picture, WH_OFFSET, SEEK_SET);
  fwrite((char *)&height, 2, 1, picture);
  fwrite((char *)&width, 2, 1, picture);

  fseek(picture, DATA_OFFSET, SEEK_SET);

  printf("  Width %u, height %u, %lu bytes (%s)\n",
         ntohs(width), ntohs(height), pic_size_int, format);

  send_photo_data_command(n_pic, pic_size_str);

  receive_data(pic_size_int, picture);

  fclose(picture);
  return 0;
}

#pragma code-name(pop)
#pragma code-name(push, "LOWCODE")

/* Get a thumnail from the camera to /RAM/THUMBNAIL */
uint8 qt1x0_get_thumbnail(uint8 n_pic) {
  FILE *picture;
  uint16 width, height;
  unsigned long pic_size_int;
  const char *format;

  platform_sleep(1);

  if (qt1x0_send_ping() != 0) {
    return -1;
  }

  picture = fopen(THUMBNAIL_NAME, "wb");

  memset(buffer, 0, BLOCK_SIZE);

  printf("  Getting header...\n");

  DUMP_START("header");

  if (send_photo_header_command(n_pic) != 0)
    return -1;

  simple_serial_read(buffer, 64);

  DUMP_DATA(buffer, 64);
  DUMP_END();

  width = htons(80);
  height = htons(60);
  pic_size_int = THUMBNAIL_SIZE;
  format = "thumbnail";

  DUMP_START("data");

  printf("  Width %u, height %u, %lu bytes (%s)\n",
         ntohs(width), ntohs(height), pic_size_int, format);

  send_photo_thumbnail_command(n_pic);

  receive_data(pic_size_int, picture);

  fclose(picture);
  return 0;
}

/* Delete all pictures from the camera */
uint8 qt1x0_delete_pictures(void) {
  char str[] = {0x16,0x29,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

  if (qt1x0_send_ping() != 0) {
    return -1;
  }

  return send_command(str, sizeof str, 0);
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

  return send_command(str, sizeof str, 0);
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

  return send_command(str, sizeof str, 0);
}

/* Get information from the camera */
uint8 qt1x0_get_information(uint8 *num_pics, uint8 *left_pics, uint8 *quality_mode, uint8 *flash_mode, uint8 *battery_level, char **name, struct tm *time) {
  #define BATTERY_IDX    0x02 /* ?? */
  #define NUM_PICS_IDX   0x04
  #define LEFT_PICS_IDX  0x06
  #define MODE_IDX       0x07
  #define MONTH_IDX      0x10
  #define DAY_IDX        0x11
  #define YEAR_IDX       0x12
  #define HOUR_IDX       0x13
  #define MIN_IDX        0x14
  #define SEC_IDX        0x15
  #define FLASH_IDX      0x16
  #define NAME_IDX       0x2F

  printf("Getting information...\n");

  DUMP_START("summary");

  if (qt1x0_send_ping() != 0) {
    return -1;
  }

  if (send_get_information_command() != 0)
    return -1;

  simple_serial_read(buffer, 128);

  DUMP_DATA(buffer, 128);
  DUMP_END();


  *num_pics     = buffer[NUM_PICS_IDX];
  *left_pics    = buffer[LEFT_PICS_IDX];
  *quality_mode = buffer[MODE_IDX];
  *flash_mode   = buffer[FLASH_IDX];
  *battery_level= buffer[BATTERY_IDX];

  time->tm_mday = buffer[DAY_IDX];
  time->tm_mon  = buffer[MONTH_IDX];
  time->tm_year = buffer[YEAR_IDX] + 2000; /* Year 2256 bug, here we come */
  time->tm_hour = buffer[HOUR_IDX];
  time->tm_min  = buffer[MIN_IDX];

  *name = trim(buffer + NAME_IDX);
  return 0;
}
#pragma code-name(pop)
