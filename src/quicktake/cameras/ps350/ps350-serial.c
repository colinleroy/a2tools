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
#include "ps350.h"
#include "../qt-serial.h"
#include "../../decoders/qt-conv.h"
#include "../../ui/ui.h"

#pragma code-name(push, "PS350")
#pragma rodata-name(push, "PS350")
#pragma data-name(push, "PS350")

uint8 command_packet[300];

/* Camera features */
#define ps350_features 0b0000000010000000
//                               ||||||||_ SET_CAMERA_NAME
//                               |||||||__ SET_CAMERA_TIME
//                               ||||||___ SET_QUALITY,
//                               |||||____ SET_FLASH,
//                               ||||_____ TAKE_PICTURE,
//                               |||______ GET_THUMBNAIL,
//                               ||_______ DELETE_PICTURES,
//                               |________ RESERVED,

/* Camera callbacks definitions */
static uint8 ps350_wakeup(CamSpeed speed);
static uint8 ps350_set_speed(CamSpeed speed);

/* Camera settings functions */
static uint8 ps350_get_information(void);

/* Camera pictures functions */
static uint8 ps350_get_picture(uint8 n_pic, int fd, off_t avail);
static uint8 ps350_get_thumbnail(uint8 n_pic, int fd);
static void ps350_get_filename(uint8 n_pic, char *dirname, char *filename);

/* Other functions, that this driver doesn't implement
 * but must exist and return -1
 */
static uint8 ps350_set_camera_name(const char *name);
static uint8 ps350_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second);
static uint8 ps350_set_quality(uint8 quality);
static uint8 ps350_set_flash(uint8 mode);
static uint8 ps350_take_picture(void);
static uint8 ps350_delete_pictures(void);

/* Camera thumbnail functions */
void ps350_thumb_histogram(void);
void ps350_load_thumb_data(uint8 line);

/* Modes strings */
static const char *ps350_get_quality_str(uint8 is_pic, uint8 mode);
static const char *ps350_get_flash_str(uint8 is_pic, uint8 mode);

/* Camera callbacks */
void *ps350_callbacks[] = {
  /* FEATURES */        (void *)ps350_features,
  /* WAKEUP */          ps350_wakeup,
  /* SET_SPEED */       ps350_set_speed,
  /* SET_CAMERA_NAME */ ps350_set_camera_name,
  /* SET_CAMERA_TIME */ ps350_set_camera_time,
  /* GET_INFORMATION */ ps350_get_information,
  /* SET_QUALITY */     ps350_set_quality,
  /* SET_FLASH */       ps350_set_flash,
  /* TAKE_PICTURE */    ps350_take_picture,
  /* GET_PICTURE */     ps350_get_picture,
  /* GET_THUMBNAIL */   ps350_get_thumbnail,
  /* DELETE_PICTURES */ ps350_delete_pictures,
  /* GET_FILENAME */    ps350_get_filename,
  /* THUMB_HISTOGRAM */ ps350_thumb_histogram,
  /* THUMB_LOAD_DATA */ ps350_load_thumb_data,
  /* GET_QUALITY_STR */ ps350_get_quality_str,
  /* GET_FLASH_STR */   ps350_get_flash_str,
};

#ifdef __CC65__
#define PC_DEBUG_BUFFER(op, str, len)
#define PC_DEBUG_PRINTF(...)
#else
#define PC_DEBUG_PRINTF(...) do { if (do_debug) printf(__VA_ARGS__); } while (0)

static void PC_DEBUG_BUFFER(char *op, const char *str, int len) {
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

uint8 count = 0;

static void ps350_prepare_packet(uint16 len) {
  bzero(command_packet, sizeof command_packet);
  command_packet[0]   = 0xC0;
  command_packet[1]   = count++;
  command_packet[3]   = len & 0xFF;
  command_packet[4]   = len >> 8;
  command_packet[299] = 0xC1;
}

static void ps350_send_packet(void) {
  unsigned short chksum = 0;
  unsigned short i;
  unsigned char j;
  unsigned char inputbit, lastbit, xorbit;

  /* Compute checksum, excluding header, checksum and trailer */
  for (i = 1; i < PS350_PKT_LEN-3; i++) {
    unsigned char cur = command_packet[i];
    for (j = 0; j <= 7; j++) {
      lastbit = chksum & 0x0001;             /* remember if last bit was 1 */
      chksum >>= 1;                          /* shift 1 place */

      inputbit = cur & 0x01;                 /* the In bit */
      cur >>= 1;
      xorbit = inputbit ^ lastbit;           /* output of upper XOR gate */
      if (xorbit) {                          /* XOR if needed */
        chksum ^= 0x8408;
      }
    }
  }
  command_packet[PS350_CHK_IDX]   = chksum & 0xFF;
  command_packet[PS350_CHK_IDX+1] = chksum >> 8;

  simple_serial_write(command_packet, PS350_PKT_LEN);
  PC_DEBUG_BUFFER("Sent: ", command_packet, PS350_PKT_LEN);
}

static void ps350_send_ack(void) {
  ps350_prepare_packet(4);
  command_packet[PS350_CMD_IDX] = CMD_ACK;
  ps350_send_packet();
}

static void ps350_send_ping(void) {
  ps350_prepare_packet(12);
  command_packet[PS350_CNT_IDX] = 0;
  command_packet[PS350_CMD_IDX] = CMD_PING;
  command_packet[9]             = 0x31;
  command_packet[11]            = 0x01;
  command_packet[12]            = 0x60;
  command_packet[13]            = 12;       /* total length */
  ps350_send_packet();
}


static uint8 ps350_get_ping_reply(void) {
#ifndef __CC65__
  bzero(buffer, sizeof buffer);
#endif
  if (simple_serial_read_no_irq((char *)buffer, PS350_PKT_LEN) == EOF) {
    PC_DEBUG_BUFFER("Ping reply short read: ", buffer, PS350_PKT_LEN);
    return -1;
  } else {
  }
  if (buffer[PS350_CMD_IDX] != CMD_PING) {
    PC_DEBUG_BUFFER("Not Ping reply: ", buffer, PS350_PKT_LEN);
    return -1;
  }
  PC_DEBUG_BUFFER("Got Ping reply: ", buffer, PS350_PKT_LEN);
  // count = buffer[PS350_CNT_IDX];

  return 0;
}

static void ps350_flush(void) {
  uint8 c;
  while (simple_serial_read_no_irq((char *)&c, 1) != EOF);
}

static uint8 ps350_get_eot(void) {
  if (simple_serial_read_no_irq((char *)buffer, PS350_PKT_LEN) == EOF) {
    PC_DEBUG_BUFFER("EOT short read: ", buffer, PS350_PKT_LEN);
    return -1;
  } else {
  }
  if (buffer[PS350_CMD_IDX] != CMD_EOT) {
    PC_DEBUG_BUFFER("Not EOT: ", buffer, PS350_PKT_LEN);
    return -1;
  }
  PC_DEBUG_BUFFER("Got EOT: ", buffer, PS350_PKT_LEN);
  // count = buffer[PS350_CNT_IDX];

  return 0;
}

#pragma warn(unused-param, push, off)
/* Wakeup and detect a Canon Powershot 350 camera
 * Returns 0 if successful, -1 otherwise
 */
static uint8 ps350_wakeup(CamSpeed speed) {
  uint8 tries = 2;
  cputs("Pinging Canon Powershot 350... ");

  simple_serial_set_speed(SER_BAUD_9600);
  simple_serial_set_parity(SER_PAR_NONE);

  /* Flush shit */
  simple_serial_send_break(200);
  ps350_flush();

  memset(command_packet, 0x55, 6);
  command_packet[0] = 0x00;
  command_packet[1] = 0x53;

  simple_serial_write(command_packet, 6);
  PC_DEBUG_BUFFER("Sending ping: ", command_packet, 6);

  if (simple_serial_read_no_irq((char *)buffer, PS350_PKT_LEN) != EOF) {
    PC_DEBUG_BUFFER("Initial reply: ", buffer, PS350_PKT_LEN);
  } else {
    PC_DEBUG_BUFFER("Initial reply shorter than expected: ", buffer, PS350_PKT_LEN);
  }

  if (memcmp(buffer+INIT_REPLY_NAME_IDX, "ModelName=PowerShot 350", 23/* strlen("ModelName=PowerShot 350") */)) {
    goto no_cam;
  }

  if (ps350_get_eot() != 0) {
    goto no_cam;
  }
  ps350_send_ack();
  return QT_MODEL_PS350;

no_cam:
  return QT_MODEL_UNKNOWN;
}

#pragma warn(unused-param, pop)

static CamSpeed my_speed = SER_BAUD_9600;

/* Send the speed upgrade command */
static uint8 ps350_set_speed(CamSpeed speed) {
  uint8 tries = 3, ps350_speed, c;

  if (speed == my_speed) {
    return 0;
  }

  switch (speed) {
  case SER_BAUD_9600:   ps350_speed = PS350_SPEED_9600;   break;
  case SER_BAUD_19200:  ps350_speed = PS350_SPEED_19200;  break;
  case SER_BAUD_57600:  ps350_speed = PS350_SPEED_57600;  break;
  case SER_BAUD_115200: ps350_speed = PS350_SPEED_115200; break;
  }

again:
  simple_serial_send_break(200);
  simple_serial_set_speed(SER_BAUD_9600);
  PC_DEBUG_PRINTF("Waiting for break\n");

  PC_DEBUG_PRINTF("Sending %02X %02X\n", 0x42, ps350_speed);
  simple_serial_putc(0x42);
  platform_msleep(500);
  simple_serial_putc(ps350_speed);
  if (simple_serial_read_no_irq(&c, 1) == EOF) {
    if (tries--) {
      goto again;
    }
    cputs("No reply.\r\n");
    return -1;
  }

  PC_DEBUG_PRINTF("Upgrading speed\n");
  simple_serial_set_speed(speed);

  PC_DEBUG_PRINTF("Sending %02X %02X\n", 0x00, 0x53);
  simple_serial_putc(0x00);
  simple_serial_putc(0x53);
  platform_msleep(500);

  ps350_send_ping();
  return ps350_get_ping_reply();
}

/* Get information from the camera */
static uint8 ps350_get_information(void) {
  return -1;
}

static void ps350_get_filename(uint8 n_pic, char *dirname, char *filename) {
  sprintf(filename, "%s%sIMAGE%d.JPG",
        IS_NOT_NULL(dirname)?dirname:"",
        IS_NOT_NULL(dirname)?"/":"", n_pic);
}

static uint8 ps350_get_picture(uint8 n_pic, int fd, off_t avail) {
  ui_get_image_header_str();
  ui_get_image_str(640, 480, 0UL);

  return -1;
}

static uint8 ps350_get_thumbnail(uint8 n_pic, int fd) {
  ui_get_thumbnail_str(n_pic);
  th_info.flash_mode   = 0xFF;
  th_info.quality_mode = 0xFF;
  return -1;
}

#pragma warn(unused-param, push, off)
static uint8 ps350_set_camera_name(const char *name) {
  return -1;
}

static uint8 ps350_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second) {
  return -1;
}

static uint8 ps350_set_quality(uint8 quality) {
  return -1;
}

static uint8 ps350_set_flash(uint8 mode) {
  return -1;
}

static uint8 ps350_take_picture(void) {
  return -1;
}

static uint8 ps350_delete_pictures(void) {
  return -1;
}

static const char *ps350_get_quality_str(uint8 is_pic, uint8 mode) {
  return "unknown";
}

static const char *ps350_get_flash_str(uint8 is_pic, uint8 mode) {
  return "unknown";
}

#pragma warn(unused-param, pop)
