/*
 * Copyright (C) 2022 Colin Leroy-Mira <colin@colino.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "simple_serial.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "extended_conio.h"

static char serial_activity_indicator_enabled = 0;
static int serial_activity_indicator_x = -1;
static int serial_activity_indicator_y = -1;

#ifdef __CC65__
#include <apple2enh.h>

/* Setup */
static int last_slot = 2;
static int last_baudrate = SER_BAUD_9600;

static struct ser_params default_params = {
    SER_BAUD_9600,     /* Baudrate */
    SER_BITS_8,         /* Number of data bits */
    SER_STOP_1,         /* Number of stop bits */
    SER_PAR_NONE,       /* Parity setting */
    SER_HS_HW           /* Type of handshake to use */
};

static char *activity_char = " *";
static char activity_count = 0;
static void activity_cb(int on) {
  gotoxy(serial_activity_indicator_x, serial_activity_indicator_y);
  cputc(on ? activity_char[activity_count++ % 2] : ' ');
}

int simple_serial_open(int slot, int baudrate, int hw_flow_control) {
  int err;
  
  if ((err = ser_install(&a2e_ssc_ser)) != 0)
    return err;

  if ((err = ser_apple2_slot(slot)) != 0)
    return err;

  default_params.baudrate = baudrate;
  /* HW flow control ignored as it's always on */

  err = ser_open (&default_params);

  last_slot = slot;
  last_baudrate = baudrate;

  return err;
}

int simple_serial_close(void) {
  return ser_close();
}

static int timeout_cycles = -1;

/* Input */
static int __simple_serial_getc_with_timeout(int with_timeout) {
    char c;

    if (with_timeout)
      timeout_cycles = 10000;

    while (ser_get(&c) == SER_ERR_NO_DATA) {
      if (with_timeout && timeout_cycles-- == 0) {
        return EOF;
      }
    }

    return (int)c;
}

static char *__simple_serial_gets_with_timeout(char *out, size_t size, int with_timeout) {
  int b;
  char c;
  size_t i = 0;

  if (serial_activity_indicator_x == -1) {
    serial_activity_indicator_x = wherex();
    serial_activity_indicator_y = wherey();
  }
  if (size == 0) {
    return NULL;
  }

  if (serial_activity_indicator_enabled) {
    activity_cb(1);
  }
  while (i < size - 1) {
    b = __simple_serial_getc_with_timeout(with_timeout);
    if (b == EOF) {
      break;
    }
    c = (char)b;
    if (c == '\r') {
      /* ignore \r */
      continue;
    }

    out[i] = c;
    i++;

    if (c == '\n') {
      break;
    }
  }
  out[i] = '\0';
  if (serial_activity_indicator_enabled) {
    activity_cb(0);
  }

  return out;
}

static size_t __simple_serial_read_with_timeout(char *ptr, size_t size, size_t nmemb, int with_timeout) {
  int b;
  size_t i = 0;
  size_t tries = 0;

  if (serial_activity_indicator_x == -1) {
    serial_activity_indicator_x = wherex();
    serial_activity_indicator_y = wherey();
  }

  if (size != 1) {
    /* unsupported */
    return 0;
  }

  if (serial_activity_indicator_enabled) {
    activity_cb(1);
  }
  while (i < (nmemb - 1)) {
    b = __simple_serial_getc_with_timeout(with_timeout);
    if (b == EOF) {
      break;
    }
    ptr[i] = (char)b;
    i++;
  }

  if (serial_activity_indicator_enabled) {
    activity_cb(0);
  }

  return i;
}

/* Output */
static int send_delay;
int simple_serial_putc(char c) {
  if ((ser_put(c)) == SER_ERR_OVERFLOW) {
    return EOF;
  }
  for (send_delay = 0; send_delay < 200; send_delay++) {
    /* Why do we need that.
     * Thanks platoterm for the hint */
  }

  return c;
}

int simple_serial_puts(char *buf) {
  int i, r, len = strlen(buf);

  if (serial_activity_indicator_x == -1) {
    serial_activity_indicator_x = wherex();
    serial_activity_indicator_y = wherey();
  }

  if (serial_activity_indicator_enabled) {
    activity_cb(1);
  }
  for (i = 0; i < len; i++) {
    if ((r = simple_serial_putc(buf[i])) == EOF)
      return EOF;
  }
  if (serial_activity_indicator_enabled) {
    activity_cb(0);
  }

  return len;
}

#else
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#define DELAY_MS 3
#define LONG_DELAY_MS 50

static FILE *ttyfp = NULL;
static int flow_control_enabled;

static void setup_tty(const char *ttypath, int baudrate, int hw_flow_control) {
  struct termios tty;
  int port = open(ttypath, O_RDWR);

  if (port < 0) {
    printf("Cannot open %s: %s\n", ttypath, strerror(errno));
    exit(1);
  }

  if(tcgetattr(port, &tty) != 0) {
    printf("tcgetattr error: %s\n", strerror(errno));
    close(port);
    exit(1);
  }
  cfsetispeed(&tty, baudrate);
  cfsetospeed(&tty, baudrate);

  tty.c_cflag &= ~PARENB;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag |= CS8;
  tty.c_cflag |= CREAD | CLOCAL;

  if (hw_flow_control)
    tty.c_cflag |= CRTSCTS;
  else
    tty.c_cflag &= ~CRTSCTS;

  flow_control_enabled = hw_flow_control;

  tty.c_lflag &= ~ICANON;
  tty.c_lflag &= ~ECHO;
  tty.c_lflag &= ~ISIG;
  tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

  tty.c_iflag &= ~(IXON | IXOFF | IXANY);
  tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);

  tty.c_oflag &= ~OPOST;
  tty.c_oflag &= ~ONLCR;

  if (tcsetattr(port, TCSANOW, &tty) != 0) {
    printf("tcgetattr error: %s\n", strerror(errno));
    close(port);
    exit(1);
  }
  close(port);
}

int simple_serial_open(const char *tty, int baudrate, int hw_flow_control) {
  setup_tty(tty, baudrate, hw_flow_control);

  ttyfp = fopen(tty, "r+b");
  if (ttyfp == NULL) {
    printf("Can't open %s\n", tty);
    return -1;
  }

  return 0;
}

int simple_serial_close(void) {
  if (ttyfp) {
    fclose(ttyfp);
  }
  ttyfp = NULL;
  return 0;
}

/* Input */
int __simple_serial_getc_with_timeout(int timeout) {
  return fgetc(ttyfp);
}

char *__simple_serial_gets_with_timeout(char *out, size_t size, int timeout) {
  return fgets(out, size, ttyfp);
}


size_t __simple_serial_read_with_timeout(char *ptr, size_t size, size_t nmemb, int timeout) {
  return fread(ptr, size, nmemb, ttyfp);
}

/* Output */
int simple_serial_putc(char c) {
  int r = fputc(c, ttyfp);
  fflush(ttyfp);

  if (!flow_control_enabled)
    usleep(DELAY_MS*1000);

  return r;
}
int simple_serial_puts(char *buf) {
  int r = fputs(buf, ttyfp);
  fflush(ttyfp);

  if (!flow_control_enabled)
    usleep(LONG_DELAY_MS*1000);

  return r;
}

#endif

int simple_serial_getc_with_timeout(void) {
  return __simple_serial_getc_with_timeout(1);
}

char simple_serial_getc(void) {
  return (char)__simple_serial_getc_with_timeout(0);
}

char *simple_serial_gets_with_timeout(char *out, size_t size) {
  return __simple_serial_gets_with_timeout(out, size, 1);
}

char *simple_serial_gets(char *out, size_t size) {
  return __simple_serial_gets_with_timeout(out, size, 0);
}

size_t simple_serial_read(char *ptr, size_t size, size_t nmemb) {
  return __simple_serial_read_with_timeout(ptr, size, nmemb, 0);
}

size_t simple_serial_read_with_timeout(char *ptr, size_t size, size_t nmemb) {
  return __simple_serial_read_with_timeout(ptr, size, nmemb, 1);
}

static char simple_serial_buf[255];
int simple_serial_printf(const char* format, ...) {
  va_list args;

  va_start(args, format);
  vsnprintf(simple_serial_buf, 255, format, args);
  va_end(args);

  return simple_serial_puts(simple_serial_buf);
}

int simple_serial_write(char *ptr, size_t size, size_t nmemb) {
  int i;
  if (size != 1) {
    printf("Can only simple_serial_write chars.\n");
    return -1;
  }
  for (i = 0; i < nmemb; i++) {
    if (simple_serial_putc(ptr[i]) < 0) {
      printf("Error sending at %d\n", i);
      return i;
    }
  }
  return i;
}

void simple_serial_set_activity_indicator(char enabled, int x, int y) {
  serial_activity_indicator_enabled = enabled;
  serial_activity_indicator_x = x;
  serial_activity_indicator_y = y;
}
