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
#include "malloc0.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "platform.h"

extern int bps;
extern unsigned char baudrate;
extern unsigned char data_baudrate;
extern unsigned char printer_baudrate;
extern unsigned char flow_control;

extern int ttyfd;

extern int flow_control_enabled;

#define DELAY_MS 3

void simple_serial_flush(void) {
  simple_serial_flush_fd(ttyfd);
}

void simple_serial_flush_fd(int fd) {
  struct termios tty;

  /* Disable flow control if needed */
  if (flow_control_enabled) {
    if(tcgetattr(fd, &tty) != 0) {
      printf("tcgetattr error: %s\n", strerror(errno));
    }

    tty.c_cflag &= ~CRTSCTS;
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
      printf("tcgetattr error: %s\n", strerror(errno));
    }
  }

  /* flush */
  tcflush(fd, TCIOFLUSH);

  /* Reenable flow control */
  if (flow_control_enabled) {
    tty.c_cflag |= CRTSCTS;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
      printf("tcgetattr error: %s\n", strerror(errno));
    }
  }
  if (fd == ttyfd)
    while(simple_serial_getc_with_timeout() != EOF);
}

extern char *opt_tty_path;

int __simple_serial_getc_with_tv_timeout(int timeout, int secs, int msecs) {
  fd_set fds;
  struct timeval tv_timeout;
  int n, r = 0;
  char c;

try_again:
  FD_ZERO(&fds);
  FD_SET(ttyfd, &fds);

  tv_timeout.tv_sec  = secs;
  tv_timeout.tv_usec = msecs*1000;

  n = select(ttyfd + 1, &fds, NULL, NULL, &tv_timeout);

  if (n > 0 && FD_ISSET(ttyfd, &fds)) {
    int flags = fcntl(ttyfd, F_GETFL);

    flags |= O_NONBLOCK;
    fcntl(ttyfd, F_SETFL, flags);

    r = read(ttyfd, &c, 1);

    flags &= ~O_NONBLOCK;
    fcntl(ttyfd, F_SETFL, flags);
  } else if (!timeout) {
    goto try_again;
  }
  return r == 1 ? c : EOF;
}

int simple_serial_getc_with_timeout(void) {
  return __simple_serial_getc_with_tv_timeout(1, 0, 500);
}

char simple_serial_getc(void) {
  return (char)__simple_serial_getc_with_tv_timeout(0, 0, 500);
}

int simple_serial_getc_immediate(void) {
  return __simple_serial_getc_with_tv_timeout(1, 0, DELAY_MS);
}

/* Output */
int serial_delay = 0;
unsigned char __fastcall__ simple_serial_putc(char c) {
  int r;
  fd_set fds;
  struct timeval tv_timeout;
  int n;
  

  FD_ZERO(&fds);
  FD_SET(ttyfd, &fds);

  tv_timeout.tv_sec  = 1;
  tv_timeout.tv_usec = 0;

  n = select(ttyfd + 1, NULL, &fds, NULL, &tv_timeout);

  if (n > 0 && FD_ISSET(ttyfd, &fds)) {
    int flags = fcntl(ttyfd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(ttyfd, F_SETFL, flags);

    r = write(ttyfd, &c, 1);

    flags &= ~O_NONBLOCK;
    fcntl(ttyfd, F_SETFL, flags);

    if (serial_delay) {
      /* Don't let the kernel buffer bytes */
      tcdrain(ttyfd);
    }

    /* Insert a delay if needed */
    usleep(serial_delay);
  } else {
    r = 0;
  }

  return r != 1 ? -1 : 0;
}

void __fastcall__ simple_serial_puts(const char *buf) {
  static const char *cur;

  cur = buf;

  while (*cur) {
    if (simple_serial_putc(*cur) == (unsigned char)-1)
      break;
    ++cur;
  }
}

void __fastcall__ simple_serial_puts_nl(const char *buf) {
  simple_serial_puts(buf);
  simple_serial_putc('\n');
}

void __fastcall__ simple_serial_read(char *ptr, size_t nmemb) {
  static char *cur;
  static char *end;

  cur = ptr;
  end = nmemb + cur;

  while (cur != end) {
    *cur = simple_serial_getc();
    ++cur;
  }
}

void __fastcall__ simple_serial_write(const char *ptr, size_t nmemb) {
  static const char *cur;
  static const char *end;

  cur = ptr;
  end = nmemb + cur;

  while (cur != end) {
    if (simple_serial_putc(*cur) == (unsigned char)-1)
      break;
    ++cur;
  }
}

#define MAX_WRITE_LEN 2048
void __fastcall__ simple_serial_write_fast(const char *ptr, size_t nmemb) {
  simple_serial_write_fast_fd(ttyfd, ptr, nmemb);
}

void __fastcall__ simple_serial_write_fast_fd(int fd, const char *ptr, size_t nmemb) {
  fd_set fds;
  struct timeval tv_timeout;
  int n, w, total = 0;

select_again:
  FD_ZERO(&fds);
  FD_SET(fd, &fds);

  tv_timeout.tv_sec  = 1;
  tv_timeout.tv_usec = 0;

  n = select(fd + 1, NULL, &fds, NULL, &tv_timeout);

  if (n > 0 && FD_ISSET(fd, &fds)) {
    int flags = fcntl(fd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);

write_again:
    w = write(fd, ptr, nmemb - total);
    if (w == -1) {
      if (errno == EAGAIN) {
        goto write_again;
      } else {
        flags &= ~O_NONBLOCK;
        fcntl(fd, F_SETFL, flags);
        goto select_again;
      }
    } else {
      total += w;
      ptr += w;
    }

    if (total < nmemb) {
      goto write_again;
    }

    flags &= ~O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);

    if (serial_delay) {
      /* Don't let the kernel buffer bytes */
      tcdrain(fd);
    }
  } else {
    if (errno == EAGAIN && total < nmemb) {
      goto select_again;
    } else {
      printf("Write error %d (%s)\n", errno, strerror(errno));
    }
  }
}
