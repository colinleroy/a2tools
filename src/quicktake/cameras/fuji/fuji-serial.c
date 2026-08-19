#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "a2_features.h"
#include "platform.h"
#include "extended_conio.h"
#include "progress_bar.h"
#include "simple_serial.h"
#include "../qt-serial.h"
#include "../../decoders/qt-conv.h"
#include "../../ui/ui.h"

#pragma code-name(push, "FUJI")
#pragma rodata-name(push, "FUJI")
#pragma data-name(push, "FUJI")

/* Camera features */
#define fuji_features 0b0000000010100000
//                              ||||||||_ SET_CAMERA_NAME
//                              |||||||__ SET_CAMERA_TIME
//                              ||||||___ SET_QUALITY,
//                              |||||____ SET_FLASH,
//                              ||||_____ TAKE_PICTURE,
//                              |||______ GET_THUMBNAIL,
//                              ||_______ DELETE_PICTURES,
//                              |________ RESERVED,

/* Camera callbacks definitions */
static uint8 fuji_wakeup(CamSpeed speed);
static uint8 fuji_set_speed(CamSpeed speed);

/* Camera settings functions */
static uint8 fuji_get_information(camera_info *info);

/* Camera pictures functions */
static uint8 fuji_get_picture(uint8 n_pic, int fd, off_t avail);
static uint8 fuji_get_thumbnail(uint8 n_pic, int fd, thumb_info *info);
static void fuji_get_filename(uint8 n_pic, char *dirname, char *filename);

/* Other functions, that this driver doesn't implement
 * but must exist and return -1
 */
static uint8 fuji_set_camera_name(const char *name);
static uint8 fuji_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second);
static uint8 fuji_set_quality(uint8 quality);
static uint8 fuji_set_flash(uint8 mode);
static uint8 fuji_take_picture(void);
static uint8 fuji_delete_pictures(void);

/* Camera thumbnail functions */
void fuji_thumb_histogram(void);
void fuji_load_thumb_data(uint8 line);

/* Camera callbacks */
void *fuji_callbacks[] = {
  /* FEATURES */        (void *)fuji_features,
  /* WAKEUP */          fuji_wakeup,
  /* SET_SPEED */       fuji_set_speed,
  /* SET_CAMERA_NAME */ fuji_set_camera_name,
  /* SET_CAMERA_TIME */ fuji_set_camera_time,
  /* GET_INFORMATION */ fuji_get_information,
  /* SET_QUALITY */     fuji_set_quality,
  /* SET_FLASH */       fuji_set_flash,
  /* TAKE_PICTURE */    fuji_take_picture,
  /* GET_PICTURE */     fuji_get_picture,
  /* GET_THUMBNAIL */   fuji_get_thumbnail,
  /* DELETE_PICTURES */ fuji_delete_pictures,
  /* GET_FILENAME */    fuji_get_filename,
  /* THUMB_HISTOGRAM */ fuji_thumb_histogram,
  /* THUMB_LOAD_DATA */ fuji_load_thumb_data,
};

extern uint8 scrw, scrh;
extern uint8 do_debug;

#define FUJI_QT200    0x00
#define FUJI_UNKNOWN  0xFF

uint8 fuji_model;

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
#define FUJI_CMD_PIC_GET_DATA  0x02
#define FUJI_CMD_SPEED         0x07
#define FUJI_CMD_GET_INFO      0x09
#define FUJI_CMD_PIC_NAME      0x0A
#define FUJI_CMD_PIC_COUNT     0x0B
#define FUJI_CMD_PIC_SIZE      0x17

#define NUM_PIC_IDX 4

#define STD_WAIT 20
#define SHORT_WAIT 5


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

static uint16 response_len;
static uint8 response_continues;

static uint8 fuji_send_ping(uint8 wait);
static void end_session(void);

#ifdef __CC65__
/* Use UI's info struct to spare memory */
extern camera_info cam_info;
#else
camera_info cam_info;
#endif

#pragma warn(unused-param, push, off)
/* Wakeup and detect a Fuji camera
 * Returns 0 if successful, -1 otherwise
 */
static uint8 fuji_wakeup(CamSpeed speed) {
  uint8 tries = 2;
  cputs("Pinging Fuji camera... ");

  simple_serial_set_speed(SER_BAUD_9600);
  simple_serial_set_parity(SER_PAR_EVEN);

  /* Speed unused there */

again:
  end_session();

  fuji_model = FUJI_UNKNOWN;
  if (fuji_send_ping(SHORT_WAIT) == 0) {
    cputs("Done.");
    fuji_get_information(&cam_info);
    if (!strcmp(cam_info.name, "QT-200")) {
      fuji_model = FUJI_QT200;
    }
    return QT_MODEL_FUJI;
  } else {
    if (--tries) {
      goto again;
    }
    cputs("Timeout.");
    return QT_MODEL_UNKNOWN;
  }
}
#pragma warn(unused-param, pop)

/* End of session */
static void end_session(void) {
  simple_serial_putc(EOT);
}

/* Read a reply from the camera */
static uint8 read_response(void) {
  uint8 eot_buf[3];
  uint16 i, j;

  /* Read the header */
  if (simple_serial_read_no_irq((char *)buffer, 6) == EOF) {
    if (do_debug) {
      cputs("Timeout reading response.\r\n");
    }
    return -1;
  }

  if (buffer[0] != ESC || buffer[1] != STX) {
    return -1;
  }

  response_len = (buffer[5] << 8) | buffer[4];

  i = 0;
  j = response_len; /* Don't change respnse_le, callers use it */
  while (j) {
    buffer[i] = serial_read_byte_no_irq();
    if (buffer[i] == ESC) {
      /* Skip escape */
      buffer[i] = serial_read_byte_no_irq();
    }
    i++;
    j--;
  }
  /* Read footer */
  simple_serial_read_no_irq((char *)eot_buf, 3);

  /* If cur_buf[1] == ETB, there will be more to read */
  response_continues = (eot_buf[1] == ETB);
  return 0;
}


/* Send a command to the camera */
static uint8 send_command(const char *cmd, uint8 len) {
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

  simple_serial_write((char *)header, sizeof header);
  simple_serial_write((char *)cmd_buffer, len);

  response_len = 0;
  if (simple_serial_read_no_irq((char *)&i, 1) != 0 || i != ACK) {
    return -1;
  }

  if (read_response() != 0) {
    return -1;
  }
  return 0;
}

static CamSpeed my_speed = SER_BAUD_9600;

/* Ping the camera */
static uint8 fuji_send_ping(uint8 wait) {
  char c = 0xFF;
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
      cputs("Ping failed\r\n");
    }
    return -1;
  }
  return 0;
}

/* Send the speed upgrade command */
static uint8 fuji_set_speed(CamSpeed speed) {
#define SPD_CMD_IDX 0x04
  //                 {????,CMD ,          ????,????,SPD }
  char str_speed[] = {0x01,FUJI_CMD_SPEED,0x01,0x00,0x00};

  if (speed == SER_BAUD_115200) {
    /* Use max possible speed */
    speed = is_iigs ? SER_BAUD_57600:SER_BAUD_19200;
  }

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

  if (send_command(str_speed, sizeof str_speed) != 0) {
    cputs("Speed set command failed.\r\n");
    if (do_debug) {
      cgetc();
    }
    return -1;
  }
  /* End session */
  end_session();

  platform_msleep(500);

  /* Toggle speed */
  simple_serial_set_speed(speed);

  /* ping again */
  if (fuji_send_ping(STD_WAIT) != 0) {
    if (do_debug) {
      cputs("Communication check failed.\r\n");
      cgetc();
    }
    return -1;
  }

  if (do_debug) {
    cputs("Success.\r\n");
  }
  if (speed != SER_BAUD_9600) {
    my_speed = speed;
  }
  return 0;
}

static uint8 fuji_start(void) {
  if (fuji_send_ping(STD_WAIT) == 0) {
    return fuji_set_speed(my_speed);
  }
  return -1;
}

static uint8 fuji_stop(void) {
  return fuji_set_speed(SER_BAUD_9600);
}

/* Get information from the camera */
static uint8 fuji_get_information(camera_info *info) {
  char cmd[]  = {0x00,FUJI_CMD_PIC_COUNT,0x00,0x00};

  fuji_start();

  if (send_command(cmd, sizeof cmd) != 0) {
    return -1;
  }
  info->num_pics = (buffer[1] << 8) + buffer[0];

  cmd[1] = FUJI_CMD_GET_INFO;
  if (send_command(cmd, sizeof cmd) != 0) {
    return -1;
  }
  PC_DEBUG("cmd", buffer, response_len);
  buffer[response_len] = '\0';

  strncpy(info->name, (char *)buffer + 6, response_len - 4);
  info->name[response_len - 5] = '\0';

  info->left_pics     = 0;
  info->quality_mode  = QUALITY_UNKNOWN;
  info->flash_mode    = FLASH_UNKNOWN;
  info->battery_level = 0;
  info->charging      = 0;

  info->date.day      = 1;
  info->date.month    = 1;
  info->date.year     = 1970;
  info->date.hour     = 0;
  info->date.minute   = 0;

  fuji_stop();
  return 0;
}

static void fuji_get_filename(uint8 n_pic, char *dirname, char *filename) {
  char cmd[]  = {0x00,FUJI_CMD_PIC_NAME,0x02,0x00,0x00,0x00};

  cmd[NUM_PIC_IDX] = n_pic;

  if (fuji_start() != 0 || send_command(cmd, sizeof cmd) != 0) {
    sprintf(filename, "%s%sIMAGE%d.JPG",
          IS_NOT_NULL(dirname)?dirname:"",
          IS_NOT_NULL(dirname)?"/":"", n_pic);
  } else {
    buffer[response_len] = '\0';
    sprintf(filename, "%s%s%s",
          IS_NOT_NULL(dirname)?dirname:"",
          IS_NOT_NULL(dirname)?"/":"",
          buffer);
  }
  fuji_stop();
}

static uint8 fuji_get_image_data(uint8 n_pic, int fd, off_t picture_size, uint8 cmd) {
  char data_cmd[] = {0x00,FUJI_CMD_PIC_GET_DATA,0x02,0x00,0x00,0x00};
  uint16 blocks_read;
  uint16 num_blocks;
  uint8 err = 0;

  data_cmd[1] = cmd;
  data_cmd[NUM_PIC_IDX] = n_pic;

  blocks_read = 0;
  num_blocks = (uint16)(picture_size / BLOCK_SIZE);

  progress_bar(2, wherey(), scrw - 2, 0, num_blocks);

  PC_DEBUG("cmd", data_cmd, sizeof data_cmd);
  if (send_command(data_cmd, sizeof data_cmd) != 0) {
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
    if (read_response() != 0) {
      errno = EIO;
      err = -1;
      break;
    }
    if (write(fd, buffer, response_len) < response_len) {
      errno = EIO;
      err = -1;
    }
  }

  progress_bar(-1, -1, scrw - 2, num_blocks, num_blocks);

  fuji_stop();

  return err;
}

static uint8 fuji_get_picture(uint8 n_pic, int fd, off_t avail) {
  char size_cmd[]= {0x00,FUJI_CMD_PIC_SIZE,0x02,0x00,0x00,0x00};
  unsigned long picture_size;

  if (fuji_start() != 0) {
    errno = EIO;
    return -1;
  }

  bzero(buffer, BLOCK_SIZE);

  ui_get_image_header_str();
  size_cmd[NUM_PIC_IDX] = n_pic;

  if (send_command(size_cmd, sizeof size_cmd) != 0) {
    errno = EIO;
    return -1;
  }

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

  ui_get_image_str(640, 480, picture_size);

  return fuji_get_image_data(n_pic, fd, picture_size, FUJI_CMD_PIC_GET_DATA);
}

static uint8 fuji_get_thumbnail(uint8 n_pic, int fd, thumb_info *info) {
  ui_get_thumbnail_str(n_pic);

  if (fuji_start() != 0) {
    errno = EIO;
    return -1;
  }

  /* FIXME get info */
  return fuji_get_image_data(n_pic, fd, 60*175, FUJI_CMD_PIC_GET_THUMB);
}

#pragma warn(unused-param, push, off)
static uint8 fuji_set_camera_name(const char *name) {
  return -1;
}

static uint8 fuji_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second) {
  return -1;
}

static uint8 fuji_set_quality(uint8 quality) {
  return -1;
}

static uint8 fuji_set_flash(uint8 mode) {
  return -1;
}

static uint8 fuji_take_picture(void) {
  return -1;
}

static uint8 fuji_delete_pictures(void) {
  return -1;
}
#pragma warn(unused-param, pop)
