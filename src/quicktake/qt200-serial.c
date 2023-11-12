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


/* Get the ack from the camera */
static uint8 get_ack(void) {
  uint8 wait = 20;

  /* about 10seconds wait */
  while (wait--) {
    if (simple_serial_getc_with_timeout() == 0x06) {
      return 0;
    }
  }
  return -1;
}

#pragma code-name(push, "LC")

/* Send an ack to the camera */
static void send_ack() {
  simple_serial_putc(0x06);
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
static uint8 qt200_send_ping(void) {
  char str[] = {0x05};

  return send_command(str, sizeof str, 0);
}

#define PNUM_IDX       0x06
#define PSIZE_IDX      0x07
#define THUMBNAIL_SIZE 0x0960

/* Wakeup and detect a QuickTake 200
 * Returns 0 if successful, -1 otherwise
 */
uint8 qt200_wakeup(void) {
  printf("Pinging QuickTake 200... ");
  qt200_send_ping();

  if (get_ack() == 0) {
    printf("Done.\n");
    return 0;
  } else {
    printf("Timeout.\n");
    return -1;
  }
}

/* Send the speed upgrade command */
uint8 qt200_set_speed(uint16 speed, int first_sleep, int second_sleep) {
#define SPD_CMD_IDX 0x06
#define SPD_CHK_IDX 0x09
  //                 {????,????,????,????,????,????,SPD ,????,????,CHKS,????}
  char str_speed[] = {0x10,0x02,0x01,0x07,0x01,0x00,0x04,0x10,0x03,0x00,0x04};
  int spd_code;

  switch(speed) {
    case 9600:
      return qt200_send_ping();

    case 19200:
#ifdef __CC65__
      spd_code = SER_BAUD_19200;
#else
      spd_code = B19200;
#endif
      str_speed[SPD_CMD_IDX] = 0x04;
      str_speed[SPD_CHK_IDX] = 0x00;
      break;

    case 57600U:
#ifdef __CC65__
      spd_code = SER_BAUD_57600;
#else
      spd_code = B57600;
#endif
      str_speed[SPD_CMD_IDX] = 0x07;
      str_speed[SPD_CHK_IDX] = 0x03;
      break;
  }

  printf("Setting speed to %u...\n", speed);
  simple_serial_write(str_speed, sizeof str_speed);

  platform_msleep(300);

  /* Toggle speed */
  simple_serial_set_speed(spd_code);

  /* get ack */
  qt200_send_ping();
  if (get_ack() != 0) {
    printf("Speed set command failed.\n");
    return -1;
  }

  return 0;
}

/* Take a picture */
uint8 qt200_take_picture(void) {
  return -1;
}

/* Set the camera name */
uint8 qt200_set_camera_name(const char *name) {
  return -1;
}

/* Set the camera time */
uint8 qt200_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second) {
  return -1;
}

/* Get a picture from the camera to a file */
uint8 qt200_get_picture(uint8 n_pic, const char *filename) {
  return -1;
}

#pragma code-name(pop)
#pragma code-name(push, "LOWCODE")

/* Get a thumnail from the camera to /RAM/THUMBNAIL */
uint8 qt200_get_thumbnail(uint8 n_pic) {
  return -1;
}

/* Delete all pictures from the camera */
uint8 qt200_delete_pictures(void) {
  return -1;
}

/* Set quality */
uint8 qt200_set_quality(uint8 quality) {
  return -1;
}

/* Set flash mode */
uint8 qt200_set_flash(uint8 mode) {
  return -1;
}

/* Get information from the camera */
uint8 qt200_get_information(uint8 *num_pics, uint8 *left_pics, uint8 *quality_mode, uint8 *flash_mode, uint8 *battery_level, char **name, struct tm *time) {
  return -1;
}
#pragma code-name(pop)
