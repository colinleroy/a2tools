#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "platform.h"
#include "extended_conio.h"
#include "progress_bar.h"
#include "simple_serial.h"
#include "qt-serial.h"
#include "qt-conv.h"

#pragma code-name(push, "QT200")
#pragma rodata-name(push, "QT200")
#pragma data-name(push, "QT200")

/* Camera features */
#define qt200_features 0b0000000010000000
//                               ||||||||_ SET_CAMERA_NAME
//                               |||||||__ SET_CAMERA_TIME
//                               ||||||___ SET_QUALITY,
//                               |||||____ SET_FLASH,
//                               ||||_____ TAKE_PICTURE,
//                               |||______ GET_THUMBNAIL,
//                               ||_______ DELETE_PICTURES,
//                               |________ RESERVED,

/* Camera callbacks definitions */
static uint8 qt200_wakeup(uint8 speed);
static uint8 qt200_set_speed(uint8 speed);

/* Camera settings functions */
static uint8 qt200_get_information(camera_info *info);

/* Camera pictures functions */
static uint8 qt200_get_picture(uint8 n_pic, int fd, off_t avail);

/* Camera callbacks */
void *qt200_callbacks[] = {
  /* FEATURES */        (void *)qt200_features,
  /* WAKEUP */          qt200_wakeup,
  /* SET_SPEED */       qt200_set_speed,
  /* SET_CAMERA_NAME */ NULL,
  /* SET_CAMERA_TIME */ NULL,
  /* GET_INFORMATION */ qt200_get_information,
  /* SET_QUALITY */     NULL,
  /* SET_FLASH */       NULL,
  /* TAKE_PICTURE */    NULL,
  /* GET_PICTURE */     qt200_get_picture,
  /* GET_THUMBNAIL */   NULL,
  /* DELETE_PICTURES */ NULL,
};

extern uint8 scrw, scrh;
extern uint8 do_debug;

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

#define STD_WAIT 20
#define SHORT_WAIT 5

static uint16 response_len;
static uint8 response_continues;

static uint8 qt200_send_ping(uint8 wait);
static void end_session(void);

/* Wakeup and detect a QuickTake 200
 * Returns 0 if successful, -1 otherwise
 */
static uint8 qt200_wakeup(uint8 speed) {
  uint8 tries = 2;
  cputs("Pinging QuickTake 200... ");

  simple_serial_set_parity(SER_PAR_EVEN);

again:
  end_session();

  if (qt200_send_ping(SHORT_WAIT) == 0) {
    cputs("Done.");
    return QT_MODEL_200;
  } else {
    if (--tries) {
      goto again;
    }
    cputs("Timeout.");
    return QT_MODEL_UNKNOWN;
  }
}

/* End of session */
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
    if (simple_serial_read_no_irq((char *)buf, 6) == EOF) {
      if (do_debug) {
        cputs("Timeout reading 6 chars.\r\n");
      }
      return -1;
    }

    if (buf[0] != ESC || buf[1] != STX) {
      if (do_debug) {
        for (i = 0; i < 6; i++) {
          cprintf("%02X ", buf[i]);
        }
        cputs(": Unexpected header.\r\n");
        cgetc();
      }
      return -1;
    }
    if (do_debug) {
      cprintf("header: %02x %02x %02x %02x %02x %02x\r\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
    }

    response_len = (buf[5] << 8) | buf[4];
    if (response_len > len) {
      /* Buffer overflow awaiting */
      if (do_debug) {
        cprintf("data too long (%d bytes)\r\n", response_len);
        cgetc();
      }
      return -1;
    }
  } else {
    response_len = BLOCK_SIZE;
  }
  cur_buf = buf;
  end_buf = cur_buf + response_len;
  i = 0;
  while (cur_buf != end_buf) {
    *cur_buf = serial_read_byte_no_irq();
    if (*cur_buf == ESC) {
      /* Skip escape */
      *cur_buf = serial_read_byte_no_irq();
    }
    cur_buf++;
    i++;
  }
  /* Read footer */
  simple_serial_read_no_irq((char *)eot_buf, 3);


  if (do_debug) {
    cprintf("read %d bytes: ", i);
    for (i = 0; i < response_len; i++) {
      cprintf("%02x ", buf[i]);
    }
    cprintf(", footer: %02x %02x %02x\r\n", eot_buf[0], eot_buf[1], eot_buf[2]);
  }

  DUMP_DATA(buf, response_len);
  /* If cur_buf[1] == ETB, there will be more to read */
  response_continues = (eot_buf[1] == ETB);
  return 0;
}


/* Send a command to the camera */
static uint8 send_command(const char *cmd, uint8 len, uint8 get_ack) {
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

  if (do_debug) {
    cprintf("\r\nSending ");
    for (i = 0; i < len; i++) {
      cprintf("%02x ", cmd_buffer[i]);
    }
    cprintf("\r\n");
  }

  simple_serial_write((char *)header, sizeof header);
  simple_serial_write((char *)cmd_buffer, len);

  if (get_ack && (simple_serial_read_no_irq((char *)&i, 1) != 0 || i != ACK))
    return -1;

  if (read_response(buffer, BLOCK_SIZE, get_ack) != 0) {
    return -1;
  }
  return 0;
}

static uint8 my_speed = SER_BAUD_9600;

/* Ping the camera */
static uint8 qt200_send_ping(uint8 wait) {
  int c;
  simple_serial_putc(ENQ);

  while (wait--) {
    if (simple_serial_read_no_irq((char *)&c, 1) == 0)
      break;
    if (kbhit() && cgetc() == CH_ESC) {
      break;
    }
  }
  if (c != ACK) {
    if (do_debug) {
      cprintf("Ping failed (%02X)\r\n", c);
    }
    return -1;
  }
  if (do_debug) {
    cprintf("Ping success (%02X)\r\n", c);
  }
  return 0;
}

/* Send the speed upgrade command */
static uint8 qt200_set_speed(uint8 speed) {
#define SPD_CMD_IDX 0x04
  //                 {????,CMD ,          ????,????,SPD }
  char str_speed[] = {0x01,FUJI_CMD_SPEED,0x01,0x00,0x00};

  switch(speed) {
    case SER_BAUD_9600:
      str_speed[SPD_CMD_IDX] = 0x00;
      break;

    case SER_BAUD_19200:
      str_speed[SPD_CMD_IDX] = 0x04;
      break;

    case SER_BAUD_57600:
      str_speed[SPD_CMD_IDX] = 0x07;
      break;
  }

  if (do_debug) {
    cprintf("Negociating speed...\r\n");
  }
  DUMP_START("set_speed");
  if (send_command(str_speed, sizeof str_speed, 1) != 0) {
    cprintf("Speed set command failed (%d).\r\n", speed);
    if (do_debug) {
      cgetc();
    }
    return -1;
  }
  DUMP_END();
  /* End session */
  end_session();

  platform_msleep(500);

  /* Toggle speed */
  simple_serial_set_speed(speed);

  /* ping again */
  if (qt200_send_ping(STD_WAIT) != 0) {
    if (do_debug) {
      cprintf("Communication check failed.\r\n");
      cgetc();
    }
    return -1;
  }

  if (do_debug) {
    cprintf("Success setting speed to %d\r\n", speed);
  }
  if (speed != SER_BAUD_9600) {
    my_speed = speed;
  }
  return 0;
}

static uint8 qt200_start(void) {
  if (do_debug) {
    cprintf("Session start, going to %d\r\n", my_speed);
  }
  if (qt200_send_ping(STD_WAIT) == 0) {
    return qt200_set_speed(my_speed);
  }
  return -1;
}

static uint8 qt200_stop(void) {
  if (do_debug) {
    cprintf("Session stop\r\n");
  }
  return qt200_set_speed(SER_BAUD_9600);
}

/* Get information from the camera */
static uint8 qt200_get_information(camera_info *info) {
  char num_pics_cmd[]  = {0x00,FUJI_CMD_PIC_COUNT,0x00,0x00};
  char info_cmd[]= {0x00,FUJI_CMD_GET_INFO,0x00,0x00};

  qt200_start();

  DUMP_START("num_pics");
  if (send_command(num_pics_cmd, sizeof num_pics_cmd, 1) != 0) {
    DUMP_END();
    return -1;
  }
  DUMP_END();
  info->num_pics = (buffer[1] << 8) + buffer[0];

  DUMP_START("info");
  if (send_command(info_cmd, sizeof info_cmd, 1) != 0) {
    DUMP_END();
    return -1;
  }
  DUMP_END();
  
  buffer[response_len] = '\0';
  info->name = malloc (response_len - 4);

  strncpy(info->name, (char *)buffer + 6, response_len - 4);
  info->name[response_len - 5] = '\0';

  info->left_pics     = 0;
  info->quality_mode  = QUALITY_STANDARD;
  info->flash_mode    = FLASH_AUTO;
  info->battery_level = 0;
  info->charging      = 0;

  info->date.day      = 1;
  info->date.month    = 1;
  info->date.year     = 1970;
  info->date.hour     = 0;
  info->date.minute   = 0;

  qt200_stop();
  return 0;
}

static uint8 qt200_get_picture(uint8 n_pic, int fd, off_t avail) {
  #define TYPE_IDX 1
  #define NUM_PIC_IDX 4
  char data_cmd[] = {0x00,0x02,0x02,0x00,0x00,0x00};
  char size_cmd[]= {0x00,FUJI_CMD_PIC_SIZE,0x02,0x00,0x00,0x00};

  uint8 err = 0;
  unsigned long picture_size;
  uint16 blocks_read;
  uint16 num_blocks;
  uint8 y;

  if (qt200_start() != 0) {
    if (do_debug) {
      cprintf("Communication error.\r\n");
      cgetc();
    }
    errno = EIO;
    return -1;
  }

  bzero(buffer, BLOCK_SIZE);

  data_cmd[NUM_PIC_IDX] = n_pic;

  cputs("  Getting size...\r\n");
  size_cmd[NUM_PIC_IDX] = n_pic;

  DUMP_START("pic_size");
  if (send_command(size_cmd, sizeof size_cmd, 1) != 0) {
    DUMP_END();
    errno = EIO;
    return -1;
  }
  DUMP_END();

#ifndef __CC65__
  picture_size = buffer[0] + (buffer[1] << 8) + (buffer[2] << 16) + (buffer[3] << 24);
#else
  ((unsigned char *)&picture_size)[0] = buffer[0];
  ((unsigned char *)&picture_size)[1] = buffer[1];
  ((unsigned char *)&picture_size)[2] = buffer[2];
  ((unsigned char *)&picture_size)[3] = buffer[3];
#endif

  if (picture_size > avail) {
    errno = ENOSPC;
    return -1;
  }


  cprintf("  Width 640, height 480, %lu bytes (jpg)\r\n",
         picture_size);

  DUMP_START("data");

  blocks_read = 0;
  num_blocks = (uint16)(picture_size / BLOCK_SIZE);

  y = wherey();
  progress_bar(2, y, scrw - 2, 0, num_blocks);

  if (send_command(data_cmd, sizeof data_cmd, 1) != 0) {
    if (do_debug) {
      cputs("Could not send get command\r\n");
      cgetc();
    }
    errno = EIO;
    return -1;
  }
  if (write(fd, buffer,response_len) < response_len) {
    errno = EIO;
    err = -1;
  }
  while (response_continues) {
    blocks_read++;
    progress_bar(-1, -1, scrw - 2, blocks_read, num_blocks);

    simple_serial_putc(ACK);
    if (read_response(buffer, BLOCK_SIZE, 1) != 0) {
      errno = EIO;
      err = -1;
      break;
    }
    if (write(fd, buffer, response_len) < response_len) {
      errno = EIO;
      err = -1;
    }
  }
  DUMP_END();
  progress_bar(-1, -1, scrw - 2, num_blocks, num_blocks);

  qt200_stop();

  return err;
}
