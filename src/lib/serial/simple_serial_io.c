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
#include "platform.h"

char *readbuf = NULL;

extern int bps;
extern unsigned char baudrate;
extern unsigned char data_baudrate;
extern unsigned char printer_baudrate;
extern unsigned char flow_control;
extern FILE *ttyfp;
extern int flow_control_enabled;

static int readbuf_idx = 0;
static int readbuf_avail = 0;

#define DELAY_MS 3

void simple_serial_flush(void) {
  struct termios tty;

  /* Disable flow control if needed */
  if (flow_control_enabled) {
    if(tcgetattr(fileno(ttyfp), &tty) != 0) {
      printf("tcgetattr error: %s\n", strerror(errno));
    }

    tty.c_cflag &= ~CRTSCTS;
    if (tcsetattr(fileno(ttyfp), TCSANOW, &tty) != 0) {
      printf("tcgetattr error: %s\n", strerror(errno));
    }
  }

  /* flush */
  tcflush(fileno(ttyfp), TCIOFLUSH);

  /* Reenable flow control */
  if (flow_control_enabled) {
    tty.c_cflag |= CRTSCTS;

    if (tcsetattr(fileno(ttyfp), TCSANOW, &tty) != 0) {
      printf("tcgetattr error: %s\n", strerror(errno));
    }
  }
  while(simple_serial_getc_with_timeout() != EOF);
}
/* Input
 * Very complicated because select() won't mark fd as readable
 * if there was more than one byte available last time and we only
 * read one. So we're doing our own buffer.
 */
int __simple_serial_getc_with_tv_timeout(int timeout, int secs, int msecs) {
  fd_set fds;
  struct timeval tv_timeout;
  int n;

  if (readbuf == NULL) {
    readbuf = malloc0(16384);
  }

send_from_buf:
  if (readbuf_avail > 0) {
    int r = (unsigned char)readbuf[readbuf_idx];
    readbuf_idx++;
    readbuf_avail--;

    return r;
  }

try_again:
  FD_ZERO(&fds);
  FD_SET(fileno(ttyfp), &fds);

  tv_timeout.tv_sec  = secs;
  tv_timeout.tv_usec = msecs*1000;

  n = select(fileno(ttyfp) + 1, &fds, NULL, NULL, &tv_timeout);

  if (n > 0 && FD_ISSET(fileno(ttyfp), &fds)) {
    int flags = fcntl(fileno(ttyfp), F_GETFL);
    int r;
    flags |= O_NONBLOCK;
    fcntl(fileno(ttyfp), F_SETFL, flags);

    r = fread(readbuf, sizeof(char), 16383, ttyfp);
    if (r > 0) {
      readbuf_avail = r;
    }
    readbuf_idx = 0;

    flags &= ~O_NONBLOCK;
    fcntl(fileno(ttyfp), F_SETFL, flags);

    goto send_from_buf;
  } else if (!timeout) {
    goto try_again;
  }
  return EOF;
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
  FD_SET(fileno(ttyfp), &fds);

  tv_timeout.tv_sec  = 1;
  tv_timeout.tv_usec = 0;

  n = select(fileno(ttyfp) + 1, NULL, &fds, NULL, &tv_timeout);

  if (n > 0 && FD_ISSET(fileno(ttyfp), &fds)) {
    int flags = fcntl(fileno(ttyfp), F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(fileno(ttyfp), F_SETFL, flags);

    r = fputc(c, ttyfp);
    fflush(ttyfp);

    flags &= ~O_NONBLOCK;
    fcntl(fileno(ttyfp), F_SETFL, flags);

    if (serial_delay) {
      /* Don't let the kernel buffer bytes */
      tcdrain(fileno(ttyfp));
    }

    /* Insert a delay if needed */
    usleep(serial_delay);
  } else {
    r = EOF;
  }

  return r == EOF ? -1 : 0;
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
  fd_set fds;
  struct timeval tv_timeout;
  int n, w, total = 0;

  if (nmemb > 256 && nmemb % 256) {
    size_t full_pages = MIN (MAX_WRITE_LEN, nmemb - (nmemb % 256));
    while (full_pages > 0) {
      simple_serial_write_fast(ptr, full_pages);
      ptr += full_pages;
      nmemb -= full_pages;
      fflush(ttyfp);
      full_pages = MIN (MAX_WRITE_LEN, nmemb - (nmemb % 256));
    }
    simple_serial_write_fast(ptr, nmemb);
    fflush(ttyfp);
    return;
  }

again:
  FD_ZERO(&fds);
  FD_SET(fileno(ttyfp), &fds);

  tv_timeout.tv_sec  = 1;
  tv_timeout.tv_usec = 0;

  n = select(fileno(ttyfp) + 1, NULL, &fds, NULL, &tv_timeout);

  if (n > 0 && FD_ISSET(fileno(ttyfp), &fds)) {
    int flags = fcntl(fileno(ttyfp), F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(fileno(ttyfp), F_SETFL, flags);

    w = fwrite(ptr, 1, nmemb - total, ttyfp);
    total += w;

    if (total < nmemb) {
      if (errno == EAGAIN) {
        ptr += w;
        fflush(ttyfp);
        goto again;
      }
    }
    fflush(ttyfp);

    flags &= ~O_NONBLOCK;
    fcntl(fileno(ttyfp), F_SETFL, flags);

    if (serial_delay) {
      /* Don't let the kernel buffer bytes */
      tcdrain(fileno(ttyfp));
    }
  } else {
    if (errno == EAGAIN && total < nmemb) {
      goto again;
    } else {
      printf("write error %d\n", errno);
    }
  }
}
