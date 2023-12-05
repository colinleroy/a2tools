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

#define STX 0x02 /* Start of data */
#define ETX 0x03 /* End of data */
#define EOT 0x04 /* End of session */
#define ENQ 0x05 /* Enquiry */
#define ACK 0x06
#define ESC 0x10
#define ETB 0x17 /* End of transmission block */
#define NAK 0x15

#define CMD_ACK 0x00
#define CMD_NAK 0x01

#define FUJI_CMD_PIC_GET_THUMB 0x00
#define FUJI_CMD_SPEED         0x07
#define FUJI_CMD_GET_INFO      0x09
#define FUJI_CMD_PIC_COUNT     0x0B
#define FUJI_CMD_PIC_SIZE      0x17


static uint16 response_len;
static uint8 response_continues;

/* Ent of session */
static void end_session(void) {
  simple_serial_putc(EOT);
}
/* Read a reply from the camera */
static uint8 read_response(unsigned char *buf, uint16 len, uint8 expect_header) {
  uint8 *cur_buf, *end_buf;
  uint8 eot_buf[3];
  uint16 i;
  if (expect_header) {
    /* Read the header */
    simple_serial_read((char *)buf, 6);

    if (buf[0] != ESC || buf[1] != STX) {
#ifdef DEBUG_PROTO
      printf("Unexpected header.\n");
      cgetc();
#endif
      return -1;
    }
#ifdef DEBUG_PROTO
    printf("header: %02x %02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
#endif
    response_len = (buf[5] << 8) | buf[4];
    if (response_len > len) {
      /* Buffer overflow awaiting */
#ifdef DEBUG_PROTO
      printf("data too long (%d bytes)\n", response_len);
      cgetc();
#endif
      return -1;
    }
  } else {
    response_len = BLOCK_SIZE;
  }
  cur_buf = buf;
  end_buf = cur_buf + response_len;
  i = 0;
  while (cur_buf != end_buf) {
    *cur_buf = simple_serial_getc();
    if (*cur_buf == ESC) {
      /* Skip escape */
      *cur_buf = simple_serial_getc();
    }
    cur_buf++;
    i++;
  }
  /* Read footer */
  simple_serial_read((char *)eot_buf, 3);

#ifdef DEBUG_PROTO
  printf("read %d bytes: ", i);
  for (i = 0; i < response_len; i++) {
    printf("%02x ", buf[i]);
  }
  printf("\n");
  printf("footer: %02x %02x %02x\n", eot_buf[0], eot_buf[1], eot_buf[2]);
#endif
  DUMP_DATA(buf, response_len);
  /* If cur_buf[1] == ETB, there will be more to read */
  response_continues = (eot_buf[1] == ETB);
  return 0;
}

/* Send a command to the camera */
static uint8 send_command(const char *cmd, uint8 len, uint8 get_ack, uint8 wait) {
  uint8 header[] = {ESC, STX};
  uint8 cmd_buffer[32];
  uint8 i, checksum = 0x00;

  memcpy(cmd_buffer, cmd, len);

  for (i = 0; i < len; i++) {
    checksum ^= cmd_buffer[i];
    if (cmd_buffer[i] == ESC) {
      memmove (cmd_buffer + i + 1, cmd_buffer + i, len - i);
      cmd_buffer[i] = ESC;
      i++;
      len++;
    }
  }
  cmd_buffer[len++] = ESC;
  cmd_buffer[len++] = ETX;
  checksum ^= ETX;
  cmd_buffer[len++] = checksum;
#ifdef DEBUG_PROTO
  printf("\nsending ");
  for (i = 0; i < len; i++) {
    printf("%02x ", cmd_buffer[i]);
  }
  printf("\n");
#endif
  simple_serial_write((char *)header, sizeof header);
  simple_serial_write((char *)cmd_buffer, len);

  if (get_ack && simple_serial_getc_with_timeout() != ACK)
    return -1;

  if (read_response(buffer, BLOCK_SIZE, get_ack) != 0) {
    return -1;
  }
  return 0;
}

static uint16 my_speed = 9600;

/* Ping the camera */
static uint8 qt200_send_ping(void) {
  int c;
  uint8 wait = 20;
  simple_serial_putc(ENQ);

  while (wait--) {
    c = simple_serial_getc_with_timeout();
    if (c != EOF)
      break;
  }
  if (c != ACK) {
#ifdef DEBUG_PROTO
    printf("Ping failed (%d)\n", c);
#endif
    return -1;
  }
  return 0;
}

/* Wakeup and detect a QuickTake 200
 * Returns 0 if successful, -1 otherwise
 */
uint8 qt200_wakeup(void) {
  end_session();
  printf("Pinging QuickTake 200... ");

  if (qt200_send_ping() == 0) {
    printf("Done.\n");
    return 0;
  } else {
    printf("Timeout.\n");
    return -1;
  }
}

/* Send the speed upgrade command */
uint8 qt200_set_speed(uint16 speed) {
#define SPD_CMD_IDX 0x04
  //                 {????,CMD ,          ????,????,SPD }
  char str_speed[] = {0x01,FUJI_CMD_SPEED,0x01,0x00,0x04};
  int spd_code;

  switch(speed) {
    case 9600:
#ifdef __CC65__
      spd_code = SER_BAUD_9600;
#else
      spd_code = B9600;
#endif
      str_speed[SPD_CMD_IDX] = 0x00;
      break;
    case 19200:
#ifdef __CC65__
      spd_code = SER_BAUD_19200;
#else
      spd_code = B19200;
#endif
      str_speed[SPD_CMD_IDX] = 0x04;
      break;

    case 57600U:
#ifdef __CC65__
      spd_code = SER_BAUD_57600;
#else
      spd_code = B57600;
#endif
      str_speed[SPD_CMD_IDX] = 0x07;
      break;
  }

#ifdef DEBUG_PROTO
  printf("Setting speed to %u...\n", speed);
#endif
  DUMP_START("set_speed");
  if (send_command(str_speed, sizeof str_speed, 1, 5) != 0) {
#ifdef DEBUG_PROTO
    printf("Speed set command failed.\n");
    cgetc();
#endif
    return -1;
  }
  DUMP_END();
  /* End session */
  end_session();

  platform_msleep(500);

  /* Toggle speed */
  simple_serial_set_speed(spd_code);


  /* ping again */
  if (qt200_send_ping() != 0) {
#ifdef DEBUG_PROTO
    printf("Speed set to %d: Communication check failed.\n", speed);
    cgetc();
#endif
    return -1;
  }

  if (speed != 9600) {
    my_speed = speed;
  }
  return 0;
}

static uint8 qt200_start(void) {
#ifdef DEBUG_PROTO
  printf("Session start, going to %d\n", my_speed);
#endif
  if (qt200_send_ping() == 0) {
    return qt200_set_speed(my_speed);
  }
  return -1;
}

static uint8 qt200_stop(void) {
#ifdef DEBUG_PROTO
  printf("Session stop\n");
#endif
  return qt200_set_speed(9600);
}

#pragma code-name(push, "LC")

/* Get information from the camera */
uint8 qt200_get_information(uint8 *num_pics, uint8 *left_pics, uint8 *quality_mode, uint8 *flash_mode, uint8 *battery_level, uint8 *charging, char **name, struct tm *time) {
  char num_pics_cmd[]  = {0x00,FUJI_CMD_PIC_COUNT,0x00,0x00};
  char info_cmd[]= {0x00,FUJI_CMD_GET_INFO,0x00,0x00};

  qt200_start();

  DUMP_START("num_pics");
  if (send_command(num_pics_cmd, sizeof num_pics_cmd, 1, 5) != 0) {
    DUMP_END();
    return -1;
  }
  DUMP_END();
  *num_pics = (buffer[1] << 8) + buffer[0];

  DUMP_START("info");
  if (send_command(info_cmd, sizeof info_cmd, 1, 5) != 0) {
    DUMP_END();
    return -1;
  }
  DUMP_END();
  
  buffer[response_len] = '\0';
  *name = malloc (response_len - 4);
  strncpy(*name, (char *)buffer + 6, response_len - 4);
  (*name)[response_len - 5] = '\0';

  *left_pics     = 0;
  *quality_mode  = QUALITY_STANDARD;
  *flash_mode    = FLASH_AUTO;
  *battery_level = 0;
  *charging = 0;
  time->tm_mday  = 1;
  time->tm_mon   = 1;
  time->tm_year  = 1970;
  time->tm_hour  = 0;
  time->tm_min   = 0;

  qt200_stop();
  return 0;
}

#pragma code-name(pop)

static uint8 get_data(uint8 n_pic, const char *name) {
  #define TYPE_IDX 1
  #define NUM_PIC_IDX 4
  char data_cmd[] = {0x00,0x02,0x02,0x00,0x00,0x00};
  char size_cmd[]= {0x00,FUJI_CMD_PIC_SIZE,0x02,0x00,0x00,0x00};

  FILE *picture;
  uint8 err = 0;
  unsigned long picture_size;
  uint16 blocks_read;
  uint16 num_blocks;
  uint8 y;

  if (qt200_start() != 0) {
#ifdef DEBUG_PROTO
    printf("Communication error.\n");
    cgetc();
#endif
    return -1;
  }

  picture = fopen(name, "wb");
  memset(buffer, 0, BLOCK_SIZE);

	data_cmd[NUM_PIC_IDX] = n_pic;

  printf("  Getting size...\n");
  size_cmd[NUM_PIC_IDX] = n_pic;

  DUMP_START("pic_size");
  if (send_command(size_cmd, sizeof size_cmd, 1, 5) != 0) {
    DUMP_END();
    return -1;
  }
  DUMP_END();
  picture_size = buffer[0] + (buffer[1] << 8) + ((uint32)buffer[2] << 16) + ((uint32)buffer[3] << 24);
#ifndef __CC65__
  picture_size = buffer[0] + (buffer[1] << 8) + (buffer[2] << 16) + (buffer[3] << 24);
#else
  ((unsigned char *)&picture_size)[0] = buffer[0];
  ((unsigned char *)&picture_size)[1] = buffer[1];
  ((unsigned char *)&picture_size)[2] = buffer[2];
  ((unsigned char *)&picture_size)[3] = buffer[3];
#endif

  printf("  Width 640, height 480, %lu bytes (jpg)\n",
         picture_size);

  DUMP_START("data");

  blocks_read = 0;
  num_blocks = (uint16)(picture_size / BLOCK_SIZE);

  y = wherey();
  progress_bar(2, y, scrw - 2, 0, num_blocks);

  if (send_command(data_cmd, sizeof data_cmd, 1, 5) != 0) {
#ifdef DEBUG_PROTO
    printf("Could not send get command\n");
    cgetc();
#endif
    return -1;
  }
  if (fwrite(buffer, 1, response_len, picture) < response_len) {
    err = -1;
  }
  while (response_continues) {
    blocks_read++;
    progress_bar(-1, -1, scrw - 2, blocks_read, num_blocks);

    simple_serial_putc(ACK);
    if (read_response(buffer, BLOCK_SIZE, 1) != 0) {
      err = -1;
      break;
    }
    if (fwrite(buffer, 1, response_len, picture) < response_len) {
      err = -1;
    }
  }
  DUMP_END();
  progress_bar(-1, -1, scrw - 2, num_blocks, num_blocks);

  qt200_stop();

  fclose(picture);

  return err;
}

#pragma code-name(push, "LOWCODE")

/* Get a picture from the camera to a file */
uint8 qt200_get_picture(uint8 n_pic, const char *filename) {
  return get_data(n_pic, filename);
}

#pragma code-name(pop)
