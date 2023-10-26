#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "platform.h"
#include "extended_conio.h"
#include "extended_string.h"
#include "simple_serial.h"

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
  long i;
  while (wait > 0) {
    for (i = 0; i < 65534L; i++);
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

static void get_hello(void) {
  uint8 i;
  DUMP_START("qt_hello");

  for (i = 0; i < 7; i++) {
    char c = simple_serial_getc();
    printf("=> 0x%02x\n", c);
    DUMP_DATA(&c, 1);
  }

  DUMP_END();
}

static void send_hello(void) {
  char str_hello[] = {0x5A,0xA5,0x55,0x05,0x00,0x00,0x25,0x80,0x00,0x80,0x02,0x00,0x80};

  DUMP_START("qt_hello_reply");
  
  simple_serial_write(str_hello, sizeof(str_hello));
  simple_serial_read(buffer, 10);

  DUMP_DATA(buffer, 10);
  DUMP_END();
}

static uint8 send_command(const char *cmd, uint8 len) {
  simple_serial_write(cmd, len);

  if (get_ack() != 0) {
    return -1;
  }
  send_ack();
  return 0;
}

/* Get the photos summary */
static uint8 send_photo_summary_command(void) {
  //           {????,????,????,????,????,????,????,RESPONSE__SIZE,????}
  char str[] = {0x16,0x28,0x00,0x30,0x00,0x00,0x00,0x00,0x00,0x80,0x00};

  return send_command(str, sizeof str);
}

#define PNUM_IDX 6
#define PSIZE_IDX 7

/* Gets thumbnail of the photo (?) */
// static uint8 send_photo_thumbnail_command(uint8 pnum) {
//   //            {????,????,????,????,????,????,PNUM,RESPONSE__SIZE,????}
//   uint8 str[] = {0x16,0x28,0x00,0x00,0x00,0x00,0x01,0x00,0x09,0x60,0x00};
// 
//   str[PNUM_IDX] = pnum;
//   return send_command(str, sizeof str);
// }

/* Gets photo header */
static uint8 send_photo_header_command(uint8 pnum) {
  //           {????,????,????,????,????,????,PNUM,RESPONSE__SIZE,????}
  char str[] = {0x16,0x28,0x00,0x21,0x00,0x00,0x01,0x00,0x00,0x40,0x00};

  str[PNUM_IDX] = pnum;
  return send_command(str, sizeof str);
}

/* Gets photo data */
static uint8 send_photo_data_command(uint8 pnum, uint8 *picture_size) {
  //           {????,????,????,????,????,????,PNUM,RESPONSE__SIZE,????}
  char str[] = {0x16,0x28,0x00,0x10,0x00,0x00,0x01,0x00,0x70,0x80,0x00};

  str[PNUM_IDX] = pnum;
  memcpy(str + PSIZE_IDX, picture_size, 3);
  return send_command(str, sizeof str);
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

void qt_get_picture(uint8 n_pic, const char *filename) {
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

  sleep(1);

  picture = fopen(filename,"wb");
  write_qtkt_header(picture);

  memset(buffer, 0, BLOCK_SIZE);
  fwrite(buffer, 1, BLOCK_SIZE, picture);
  fwrite(buffer, 1, BLOCK_SIZE, picture);

  printf("  Getting header...\n");

  DUMP_START("header");

  send_photo_header_command(n_pic);
  simple_serial_read(buffer, 64);

  DUMP_DATA(buffer, 64);
  DUMP_END();

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

  memcpy(pic_size_str, buffer + IMG_SIZE_IDX, 3);

  DUMP_START("data");

  send_photo_data_command(n_pic, pic_size_str);
  pic_size_int = ((uint32)pic_size_str[0])<<16 | pic_size_str[1]<<8 | pic_size_str[2];

  printf("  Width %u, height %u, %lu bytes\n", 
         ntohs(width), ntohs(height), pic_size_int);

  for (i = 0; i < pic_size_int / BLOCK_SIZE; i++) {
    simple_serial_read(buffer, BLOCK_SIZE);

    fwrite(buffer, 1, BLOCK_SIZE, picture);
    DUMP_DATA(buffer, BLOCK_SIZE);

    send_ack();
  }
  simple_serial_read(buffer, pic_size_int % BLOCK_SIZE);
  fwrite(buffer, 1, pic_size_int % BLOCK_SIZE, picture);
  DUMP_DATA(buffer, pic_size_int % BLOCK_SIZE);

  DUMP_END();
  fclose(picture);

}

void qt_serial_connect(void) {
  int i;
  printf("Connecting to Quicktake...\n");
#ifdef __CC65__
  simple_serial_set_speed(SER_BAUD_9600);
#else
  simple_serial_set_speed(B9600);
#endif

  simple_serial_open();
  simple_serial_dtr_onoff(0);

  get_hello();
  send_hello();

  //tcflush(fileno(ttyfp), TCIOFLUSH);
#ifdef __CC65__
  simple_serial_set_parity(SER_PAR_EVEN);
#else
  simple_serial_set_parity(PARENB);
#endif

  sleep(1);
  printf("Initializing...\n");

  send_separator();
}

const char *qt_get_mode_str(uint8 mode) {
  switch(mode) {
    case 1:  return "standard quality";
    case 2:  return "high quality";
    default: return "Unknown";
  }
}

void qt_get_information(uint8 *num_pics, uint8 *left_pics, uint8 *mode, char **name) {
  #define NUM_PICS_IDX   0x04
  #define LEFT_PICS_IDX  0x06
  #define NAME_IDX       0x2F
  #define MODE_IDX       0x07

  printf("Getting information...\n");
  
  DUMP_START("summary");

  send_photo_summary_command();
  simple_serial_read(buffer, 128);

  DUMP_DATA(buffer, 128);
  DUMP_END();

  *num_pics = buffer[NUM_PICS_IDX];
  *left_pics = buffer[LEFT_PICS_IDX];
  *mode = buffer[MODE_IDX] - 1;
  *name = trim(buffer + NAME_IDX);
}
