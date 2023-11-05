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

#pragma code-name(push, "LC")

static uint8 get_hello(void) {
  int c;
  uint8 wait;

  wait = 20;
  while (wait--) {
    c = simple_serial_getc_with_timeout_rom();
    if (c != EOF) {
      goto read;
    }
  }
  printf("Cannot connect (timeout).\n");
  return -1;
read:
  buffer[0] = (unsigned char)c;
  simple_serial_read(buffer + 1, 6);

  DUMP_START("qt_hello");
  DUMP_DATA(buffer, 7);
  DUMP_END();

  return 0;
}

static uint8 send_hello(uint16 speed) {
  #define SPD_IDX 0x06
  #define CHK_IDX 0x0C
  char str_hello[] = {0x5A,0xA5,0x55,0x05,0x00,0x00,0x25,0x80,0x00,0x80,0x02,0x00,0x80};
  int r;
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
  r = simple_serial_getc_with_timeout_rom();
  if (r == EOF) {
    return -1;
  }
  buffer[0] = (char)r;
  simple_serial_read(buffer + 1, 9);

  DUMP_DATA(buffer, 10);
  DUMP_END();

  return 0;
}

uint8 qt_1x0_wakeup(uint16 speed) {
#ifdef __CC65__
  #ifndef IIGS
    printf("Toggling printer port on...\n");
    simple_serial_acia_onoff(1, 1);
    platform_sleep(1);
    printf("Toggling printer port off...\n");
    simple_serial_acia_onoff(1, 0);
  #else
    printf("Toggling DTR on...\n");
    simple_serial_dtr_onoff(1);
    printf("Toggling DTR off...\n");
    simple_serial_dtr_onoff(0);
  #endif
#else
  printf("Toggling DTR on...\n");
  simple_serial_dtr_onoff(1);
  printf("Toggling DTR off...\n");
  simple_serial_dtr_onoff(0);
#endif

  printf("Waiting for hello...\n");
  if (get_hello() != 0) {
    return -1;
  }
  printf("Sending hello...\n");
  send_hello(speed);

  printf("Connected to QuickTake 1x0.\n");
  return 0;
}

#pragma code-name(pop)
