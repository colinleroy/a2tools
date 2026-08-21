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
#include "dc50.h"
#include "dc50-read-response.h"
#include "../qt-serial.h"
#include "../../decoders/qt-conv.h"
#include "../../ui/ui.h"

#pragma code-name(push, "DC50")
#pragma rodata-name(push, "DC50")
#pragma data-name(push, "DC50")

/* Camera features */
#define dc50_features 0b0000000010000000
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
static uint8 dc50_get_information(camera_info *info);

/* Camera pictures functions */
static uint8 dc50_get_picture(uint8 n_pic, int fd, off_t avail);
static uint8 dc50_get_thumbnail(uint8 n_pic, int fd, thumb_info *info);
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

static const char *dc50_get_quality_str(uint8 mode);
static const char *dc50_get_flash_str(uint8 mode);

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

void init_packet(char command) {
  bzero(command_packet, 8);
  command_packet[0] = command;
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

extern int ttyfd;

#pragma warn(unused-param, push, off)
/* Wakeup and detect a DC50 camera
 * Returns 0 if successful, -1 otherwise
 */
static uint8 dc50_wakeup(CamSpeed speed) {
  uint8 tries = 2;
  cputs("Pinging Kodak DC50... ");

  simple_serial_set_speed(SER_BAUD_9600);
  simple_serial_set_parity(SER_PAR_NONE);

  // ACIA: CMD register bit 2/3, 1/1, transmit break
  simple_serial_send_break(250);

  platform_msleep(1500);

  if (dc50_set_speed(speed) == 0) {
    return QT_MODEL_DC50;
  }
  return QT_MODEL_UNKNOWN;
}
#pragma warn(unused-param, pop)

static CamSpeed my_speed = SER_BAUD_9600;
static uint8 using_ram_card;
#ifdef __CC65__
/* Use UI's info struct to spare memory */
extern camera_info cam_info;
#else
static camera_info cam_info;
#endif

static uint8 wait_command_completion(void) {
  char c;
  while (simple_serial_read_no_irq((char *)&c, 1) == 0) {
    PC_DEBUG("completion", &c, 1);
    if (c == REP_BUSY) {
      continue;
    }
    if (c == REP_COMPLETE) {
      return 0;
    }
    return -1;
  }
  return -1;
}

static uint8 dc50_send_and_read_response(uint16 response_len) {
  uint8 c;
  PC_DEBUG("CMD", command_packet, 8);
  simple_serial_write(command_packet, 8);
  if (simple_serial_read_no_irq((char *)&c, 1) != 0) {
    PC_DEBUG("No response", &c, 1);
    return -1;
  }
  PC_DEBUG("response", &c, 1);
  if (c != REP_ACK) {
    return -1;
  }

  if (response_len == 0) {
    return 0;
  }

  c = simple_serial_read_no_irq(buffer, response_len);

  PC_DEBUG("Data", buffer, response_len);
  /* Checksum */
  simple_serial_read_no_irq((char *)&c, 1);
  PC_DEBUG("checksum", &c, 1);
  
  simple_serial_putc(REP_CORRECT);

  return wait_command_completion();
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

  dc50_send_and_read_response(0);
  platform_msleep(300);

  /* Toggle speed */
  simple_serial_set_speed(speed);

  if (dc50_get_information(&cam_info) == 0) {
    my_speed = speed;
    return 0;
  }

  return -1;
}

#define BATTERY_STATUS_IDX    8
#define AC_STATUS_IDX         9
#define COMPRESSION_MODE_IDX  19
#define FLASH_MODE_IDX        20
#define NUM_INTERNAL_PIC_IDX  36 // big-endian, 16 bits
#define NUM_CARD_PIC_IDX      56 // big-endian, 16-bits
#define CAMERA_NAME_IDX       80

/* Get information from the camera */
static uint8 dc50_get_information(camera_info *info) {
  init_packet(CMD_GET_STATUS);
  if (dc50_send_and_read_response(256) != 0) {
    return -1;
  }

  switch(buffer[BATTERY_STATUS_IDX]) {
  case 0: /* full  */ buffer[BATTERY_STATUS_IDX] = 90; break;
  case 1: /* low   */ buffer[BATTERY_STATUS_IDX] = 50; break;
  case 2: /* empty */ buffer[BATTERY_STATUS_IDX] = 10; break;
  }
  info->battery_level = buffer[BATTERY_STATUS_IDX];

  info->flash_mode = buffer[FLASH_MODE_IDX];
  info->quality_mode = buffer[COMPRESSION_MODE_IDX];

  info->charging      = buffer[AC_STATUS_IDX];
  /* Counter is 16 bits but there won't be more than 256 pictures. */
  if ((info->num_pics = buffer[NUM_CARD_PIC_IDX+1]) == 0) {
    /* we'll access internal memory if there's no card or no pics on it. */
    info->num_pics    = buffer[NUM_INTERNAL_PIC_IDX+1];
    using_ram_card    = 0;
  } else {
    using_ram_card    = 1;
  }
  memcpy(info->name, buffer+CAMERA_NAME_IDX, 31);
  info->name[31] = '\0';
  return 0;
}

static void dc50_get_filename(uint8 n_pic, char *dirname, char *filename) {
  if (1) {
    sprintf(filename, "%s%sIMAGE%d.JPG",
          IS_NOT_NULL(dirname)?dirname:"",
          IS_NOT_NULL(dirname)?"/":"", n_pic);
  } else {
    sprintf(filename, "%s%s%s",
          IS_NOT_NULL(dirname)?dirname:"",
          IS_NOT_NULL(dirname)?"/":"",
          buffer);
  }
}

static uint8 dc50_get_image_data(uint8 n_pic, int fd, off_t picture_size, uint8 cmd) {
  progress_bar(-1, -1, scrw - 2, 1, 1);
  return -1;
}

static uint8 dc50_get_picture(uint8 n_pic, int fd, off_t avail) {
  ui_get_image_str(640, 480, 65536);

  return -1;
}

static uint8 dc50_get_thumbnail(uint8 n_pic, int fd, thumb_info *info) {
  return -1;
}

#pragma warn(unused-param, push, off)
static uint8 dc50_set_camera_name(const char *name) {
  return -1;
}

static uint8 dc50_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second) {
  return -1;
}

static uint8 dc50_set_quality(uint8 quality) {
  return -1;
}

static uint8 dc50_set_flash(uint8 mode) {
  return -1;
}

static uint8 dc50_take_picture(void) {
  return -1;
}

static uint8 dc50_delete_pictures(void) {
  return -1;
}


static const char *dc50_get_quality_str(uint8 mode) {
  switch (mode % 4) {
    /* we return %s quality, not compression, so invert */
    case 0: return "full";
    case 1: return "high";
    case 2: return "medium";
    case 3: return "low";
  }
  return NULL;
}

static const char *dc50_get_flash_str(uint8 mode) {
  switch(mode % 5) {
    case 2: return "off";
    case 1: return "fill";
    case 4: return "fill red-eye";
    case 0: return "auto";
    case 3: return "auto red-eye";
  }
  return NULL;
}

#pragma warn(unused-param, pop)
