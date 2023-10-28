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

extern uint8 scrw, scrh;

#ifndef __CC65__
FILE *dbgfp = NULL;

#define DUMP_START(name) do {       \
  dbgfp = fopen(name".dump", "wb"); \
} while (0)
#define DUMP_DATA(buf,size) do {    \
  fwrite(buf, 1, size, dbgfp);      \
} while (0)
#define DUMP_END() do {             \
  fclose(dbgfp);                    \
} while (0)

#else

#define DUMP_START(name)
#define DUMP_DATA(buf,size)
#define DUMP_END()

unsigned __fastcall__ sleep (unsigned wait) {
  int i;
  while (wait > 0) {
    for (i = 0; i < 30000; i++);
    wait--;
  }
  return 0;
}

#endif

#define BLOCK_SIZE 512
static char buffer[BLOCK_SIZE];

static int get_ack(void) {
  if (simple_serial_getc() != 0x00) {
    printf("unexpected reply\n");
    return -1;
  }
  return 0;
}

static void send_ack() {
  simple_serial_putc(0x06);
}

static void send_separator(void) {
  char str_start[] = {0x16,0x00,0x00,0x00,0x00,0x00,0x00};
  simple_serial_write(str_start, sizeof(str_start));
  get_ack();
}

static uint8 get_hello(void) {
  uint8 i;
  DUMP_START("qt_hello");

  for (i = 0; i < 7; i++) {
    int c = simple_serial_getc_with_timeout();
    if (c == EOF) {
      printf("Cannot connect (timeout at byte %d/7).\n", i + 1);
      DUMP_END();
      return -1;
    }
    DUMP_DATA(&c, 1);
  }
  DUMP_END();
  return 0;
}

static void send_hello(void) {
  char str_hello[] = {0x5A,0xA5,0x55,0x05,0x00,0x00,0x25,0x80,0x00,0x80,0x02,0x00,0x80};

  DUMP_START("qt_hello_reply");

  simple_serial_write(str_hello, sizeof(str_hello));
  simple_serial_read(buffer, 10);

  DUMP_DATA(buffer, 10);
  DUMP_END();
}

static uint8 send_command(const char *cmd, uint8 len, uint8 s_ack) {
  simple_serial_write(cmd, len);

  if (get_ack() != 0) {
    return -1;
  }
  if (s_ack)
    send_ack();

  return 0;
}

/* Get the photos summary */
static uint8 send_photo_summary_command(void) {
  //           {????,????,????,????,????,????,????,RESPONSE__SIZE,????}
  char str[] = {0x16,0x28,0x00,0x30,0x00,0x00,0x00,0x00,0x00,0x80,0x00};

  return send_command(str, sizeof str, 1);
}

#define PNUM_IDX 6
#define PSIZE_IDX 7
#define THUMBNAIL_SIZE 0x0960

/* Gets thumbnail of the photo (?)
 * At least, the data received is 2400 bytes long, which correspond
 * to raw 4-bit data for a 80x60 image.
 */
static uint8 send_photo_thumbnail_command(uint8 pnum) {
  //            {????,????,????,????,????,????,PNUM,RESPONSE__SIZE,????}
  char str[] = {0x16,0x28,0x00,0x00,0x00,0x00,0x01,0x00,0x09,0x60,0x00};

  str[PNUM_IDX] = pnum;
  return send_command(str, sizeof str, 1);
}

/* Gets photo header */
static uint8 send_photo_header_command(uint8 pnum) {
  //           {????,????,????,????,????,????,PNUM,RESPONSE__SIZE,????}
  char str[] = {0x16,0x28,0x00,0x21,0x00,0x00,0x01,0x00,0x00,0x40,0x00};

  str[PNUM_IDX] = pnum;
  return send_command(str, sizeof str, 1);
}

/* Gets photo data */
static uint8 send_photo_data_command(uint8 pnum, uint8 *picture_size) {
  //           {????,????,????,????,????,????,PNUM,RESPONSE__SIZE,????}
  char str[] = {0x16,0x28,0x00,0x10,0x00,0x00,0x01,0x00,0x70,0x80,0x00};

  str[PNUM_IDX] = pnum;
  memcpy(str + PSIZE_IDX, picture_size, 3);
  return send_command(str, sizeof str, 1);
}

void qt_set_camera_name(const char *name) {
  #define NAME_SET_IDX 13
  char str[] = {0x16,0x2a,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x22,0x00,0x02,0x20,
               0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
               0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};
  uint8 len;

  len = strlen(name);
  if (len > 31)
    len = 31;

  memcpy(str + NAME_SET_IDX, name, len);
  send_command(str, sizeof str, 0);
}

void qt_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second) {
  #define SET_MONTH_IDX 13
  #define SET_DAY_IDX   14
  #define SET_YEAR_IDX  15
  #define SET_HOUR_IDX  16
  #define SET_MIN_IDX   17
  #define SET_SEC_IDX   18
  //           {                                                                  mon  day year hour  min  sec
  char str[] = {0x16,0x2A,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x01,0x06,0x00,0x00,0x00,0x00,0x00,0x00};

  str[SET_DAY_IDX]   = day;
  str[SET_MONTH_IDX] = month;
  str[SET_YEAR_IDX]  = year;
  str[SET_HOUR_IDX]  = hour;
  str[SET_MIN_IDX]   = minute;
  str[SET_SEC_IDX]   = second;

  send_command(str, sizeof str, 0);
}

// #define SPD_IDX 13
// static uint8 set_speed(uint16 speed) {
//   char str_speed[] = {0x16,0x2A,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x05,0x00,0x03,0x03,0x08,0x04,0x00};
//   uint8 spd_code;
//   switch(speed) {
//     case 9600:
//       spd_code = B9600;
//       str_speed[SPD_IDX] = 0x08;
//       break;
//     case 19200:
//       spd_code = B19200;
//       str_speed[SPD_IDX] = 0x10;
//       break;
//   }
//   simple_serial_write(str_speed, sizeof str_speed);
//   tty_set_speed(spd_code);
//   sleep(1);
//
//   /* get ack */
//   if (get_ack() != 0) {
//     return -1;
//   }
//   send_ack();
//
//   /* Get a full kB of 0xaa ?? */
//   simple_serial_read(buffer, BLOCK_SIZE);
//   simple_serial_read(buffer, BLOCK_SIZE);
//
//   send_ack();
//   return get_ack();
// }


static void write_qtkt_header(FILE *fp) {
  char hdr[] = {0x71,0x6B,0x74,0x6B,0x00,0x00,0x00,0x04,0x00,0x00,0x73,0xE4,0x00,0x01};
  fwrite(hdr, 1, sizeof hdr, fp);
}

#define char_to_n_uint16(buf) (((uint8)((buf)[1]))<<8 | ((uint8)((buf)[0])))

void qt_get_picture(uint8 n_pic, const char *filename, uint8 full) {
  #define IMG_WIDTH_IDX  0x08
  #define IMG_HEIGHT_IDX 0x0A
  #define IMG_SIZE_IDX   0x05
  #define WH_OFFSET 544
  #define DATA_OFFSET 736

  uint16 i;
  FILE *picture;
  uint16 width, height;
  unsigned char pic_size_str[3];
  unsigned long pic_size_int;
  uint8 y;

  sleep(1);

  picture = fopen(filename,"wb");

  memset(buffer, 0, BLOCK_SIZE);

  if (full) {
    write_qtkt_header(picture);
    fwrite(buffer, 1, BLOCK_SIZE, picture);
    fwrite(buffer, 1, BLOCK_SIZE, picture);
  }

  printf("  Getting header...\n");

  DUMP_START("header");

  send_photo_header_command(n_pic);
  simple_serial_read(buffer, 64);

  DUMP_DATA(buffer, 64);
  DUMP_END();

  if (full) {
    /* Write the header */
    fseek(picture, 0x0E, SEEK_SET);
    fwrite(buffer + 4, 1, 64 - 4, picture);

    /* Get dimensions */
    width = char_to_n_uint16(buffer + IMG_WIDTH_IDX);
    height = char_to_n_uint16(buffer  + IMG_HEIGHT_IDX);

    /* Set them in the file */
    fseek(picture, WH_OFFSET, SEEK_SET);
    fwrite((char *)&height, 2, 1, picture);
    fwrite((char *)&width, 2, 1, picture);

    fseek(picture, DATA_OFFSET, SEEK_SET);
  } else {
    width = htons(80);
    height = htons(60);
  }

  DUMP_START("data");

  if (full) {
    memcpy(pic_size_str, buffer + IMG_SIZE_IDX, 3);
    send_photo_data_command(n_pic, pic_size_str);

#ifndef __CC65__
    pic_size_int = (pic_size_str[0]<<16) + (pic_size_str[1]<<8) + (pic_size_str[2]);
#else
    ((unsigned char *)&pic_size_int)[0] = pic_size_str[2];
    ((unsigned char *)&pic_size_int)[1] = pic_size_str[1];
    ((unsigned char *)&pic_size_int)[2] = pic_size_str[0];
    ((unsigned char *)&pic_size_int)[3] = 0;
#endif

  } else {
    send_photo_thumbnail_command(n_pic);
    pic_size_int = THUMBNAIL_SIZE;
  }

  printf("  Width %u, height %u, %lu bytes\n",
         ntohs(width), ntohs(height), pic_size_int);

  printf("  Getting data...\n");
  y = wherey();

  progress_bar(2, y, scrw - 2, 0, (uint16)(pic_size_int / BLOCK_SIZE));
  for (i = 0; i < (uint16)(pic_size_int / BLOCK_SIZE); i++) {

    simple_serial_read(buffer, BLOCK_SIZE);
    fwrite(buffer, 1, BLOCK_SIZE, picture);
    DUMP_DATA(buffer, BLOCK_SIZE);

    progress_bar(2, y, scrw - 2, i, (uint16)(pic_size_int / BLOCK_SIZE));

    send_ack();
  }
  simple_serial_read(buffer, (uint16)(pic_size_int % BLOCK_SIZE));
  fwrite(buffer, 1, pic_size_int % BLOCK_SIZE, picture);
  DUMP_DATA(buffer, pic_size_int % BLOCK_SIZE);

  progress_bar(2, y, scrw - 2, 100, 100);

  DUMP_END();
  fclose(picture);
}

uint8 qt_serial_connect(void) {
  printf("Connecting to Quicktake...\n");
#ifdef __CC65__
  simple_serial_set_speed(SER_BAUD_9600);
  #ifndef IIGS
  simple_serial_set_flow_control(SER_HS_NONE);
  #endif
#else
  simple_serial_set_speed(B9600);
#endif

  simple_serial_open();
#ifdef __CC65__
  #ifndef IIGS

  printf("Toggling printer port on...\n");
  simple_serial_acia_onoff(1, 1);
  sleep(1);
  printf("Toggling printer port off...\n");
  simple_serial_acia_onoff(1, 0);

  #else
  printf("Toggling DTR off...\n");
  simple_serial_dtr_onoff(0);
  #endif
#else
  printf("Toggling DTR off...\n");
  simple_serial_dtr_onoff(0);
#endif

  printf("Waiting for hello...\n");
  if (get_hello() != 0) {
    return -1;
  }
  printf("Sending hello...\n");
  send_hello();

  printf("Connected.\n");

#ifdef __CC65__
  simple_serial_set_parity(SER_PAR_EVEN);
#else
  simple_serial_set_parity(PARENB);
#endif
  printf("Parity set.\n");

  sleep(1);
  printf("Initializing...\n");

  send_separator();

  return 0;
}

const char *qt_get_mode_str(uint8 mode) {
  switch(mode) {
    case 1:  return "standard quality";
    case 2:  return "high quality";
    default: return "Unknown";
  }
}

void qt_get_information(uint8 *num_pics, uint8 *left_pics, uint8 *mode, char **name, struct tm *time) {
  #define NUM_PICS_IDX   0x04
  #define LEFT_PICS_IDX  0x06
  #define MODE_IDX       0x07
  #define MONTH_IDX      0x10
  #define DAY_IDX        0x11
  #define YEAR_IDX       0x12
  #define HOUR_IDX       0x13
  #define MIN_IDX        0x14
  #define NAME_IDX       0x2F

  printf("Getting information...\n");

  DUMP_START("summary");

  send_photo_summary_command();
  simple_serial_read(buffer, 128);

  DUMP_DATA(buffer, 128);
  DUMP_END();

  *num_pics = buffer[NUM_PICS_IDX];
  *left_pics = buffer[LEFT_PICS_IDX];
  *mode = buffer[MODE_IDX] - 1;

  time->tm_mday = buffer[DAY_IDX];
  time->tm_mon  = buffer[MONTH_IDX];
  time->tm_year = buffer[YEAR_IDX] + 2000; /* Year 2256 bug, here we come */
  time->tm_hour = buffer[HOUR_IDX];
  time->tm_min  = buffer[MIN_IDX];

  *name = trim(buffer + NAME_IDX);
}
