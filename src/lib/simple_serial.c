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
#include <errno.h>
#include "extended_conio.h"
#include "extended_string.h"

#ifdef __CC65__
#pragma static-locals(push, on)

#ifdef SURL_TO_LANGCARD
#pragma code-name (push, "LC")
#else
#pragma code-name (push, "LOWCODE")
#endif
#endif

#ifdef __CC65__

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

int __fastcall__ simple_serial_open(int slot, int baudrate) {
  int err;
  
#ifdef __APPLE2__
  if ((err = ser_install(&a2e_ssc_ser)) != 0)
    return err;
#endif

#ifdef __APPLE2__
  if ((err = ser_apple2_slot(slot)) != 0)
    return err;
#endif

  default_params.baudrate = baudrate;
  /* HW flow control ignored as it's always on */

  err = ser_open (&default_params);

  last_slot = slot;
  last_baudrate = baudrate;

  return err;
}

int __fastcall__ simple_serial_close(void) {
  return ser_close();
}

void __fastcall__ simple_serial_flush(void) {
  while(simple_serial_getc_with_timeout() != EOF);
}

#pragma optimize(push, on)
int __fastcall__ simple_serial_getc_immediate(void) {
  static char c;
  if (ser_get(&c) != SER_ERR_NO_DATA) {
    return c;
  }
  return EOF;
}

static int timeout_cycles = -1;

/* Input */
int __fastcall__ simple_serial_getc_with_timeout(void) {
    static char c;

    timeout_cycles = 10000;

    while (ser_get(&c) == SER_ERR_NO_DATA) {
      if (--timeout_cycles == 0) {
        return EOF;
      }
    }
    return (int)c;
}

char __fastcall__ simple_serial_getc(void) {
    static char c;

    while (ser_get(&c) == SER_ERR_NO_DATA);
    return c;
}

#pragma optimize(pop)

#else
/* POSIX */
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define DELAY_MS 3

static FILE *ttyfp = NULL;
static int flow_control_enabled;

static char *readbuf = NULL;
static int readbuf_idx = 0;
static int readbuf_avail = 0;
static int bps = B9600;
static void setup_tty(int port, int baudrate, int hw_flow_control) {
  struct termios tty;

  if(tcgetattr(port, &tty) != 0) {
    printf("tcgetattr error: %s\n", strerror(errno));
    close(port);
    exit(1);
  }
  cfsetispeed(&tty, baudrate);
  cfsetospeed(&tty, baudrate);

  bps = baudrate;

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
    exit(1);
  }
}

#include "simple_serial_opts.c"

int simple_serial_open(void) {
  struct flock lock;

  lock.l_start = 0;
  lock.l_len = 0;
  lock.l_pid = getpid();
  lock.l_type = F_WRLCK;
  lock.l_whence = SEEK_SET;

  /* Get options */
  simple_serial_read_opts();
  
  /* Open file */
  ttyfp = fopen(opt_tty_path, "r+b");

  /* Try to lock file */
  if (ttyfp != NULL && fcntl(fileno(ttyfp), F_SETLK, &lock) < 0) {
    printf("%s is already opened by another process.\n", opt_tty_path);
    fclose(ttyfp);
    ttyfp = NULL;
  }
  if (ttyfp == NULL) {
    printf("Can't open %s\n", opt_tty_path);
    return -1;
  }

  simple_serial_flush();
  setup_tty(fileno(ttyfp), opt_tty_speed, opt_tty_hw_handshake);

  return 0;
}

int simple_serial_close(void) {
  if (ttyfp) {
    fclose(ttyfp);
  }
  ttyfp = NULL;
  if (readbuf) {
    free(readbuf);
    readbuf = NULL;
  }
  return 0;
}

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
  tcdrain(fileno(ttyfp));

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
    readbuf = malloc(16384);
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
unsigned char __fastcall__ simple_serial_putc(char c) {
  int r = fputc(c, ttyfp);
  fflush(ttyfp);

  if (!flow_control_enabled)
    usleep(500);

  return r == EOF ? -1 : 0;
}

#endif /* End of platform-dependant code */

#ifdef __CC65__
#pragma optimize(push, on)
#endif

void __fastcall__ simple_serial_puts(char *buf) {
  static char *cur;

  cur = buf;

  while (*cur) {
    simple_serial_putc(*cur);
    ++cur;
  }
}

char * __fastcall__ simple_serial_gets(char *out, size_t size) {
  static char c;
  static char *cur;
  static char *end;

  if (size == 0) {
    return NULL;
  }

  cur = out;
  end = cur + size - 1;
  while (cur < end) {
#ifdef __CC65__
    while (ser_get(&c) == SER_ERR_NO_DATA);
#else
    c = simple_serial_getc();
#endif
    
    if (c == '\r') {
      /* ignore \r */
      continue;
    }

    *cur = c;
    ++cur;

    if (c == '\n') {
      break;
    }
  }
  *cur = '\0';

  return out;
}

void __fastcall__ simple_serial_read(char *ptr, size_t nmemb) {
  static char *cur;
  static char *end;

  cur = ptr;
  end = ptr + nmemb;

#ifdef __CC65__
#ifdef __APPLE2ENH__
  __asm__("                  bra check_bound");           /* branch to check cur != end */
#else
  __asm__("                  ldx #$00");                  /* branch to check cur != end */
  __asm__("                  beq check_bound");           /* branch to check cur != end */
#endif

    __asm__("read_again:       lda %v",   cur);           /* low byte in A */
    __asm__("read_again_aok:   ldx %v+1", cur);           /* high byte in X */
    __asm__("read_again_axok:  jsr %v",   ser_get);       /* pass cur to ser_get */
    __asm__("                  cmp #%b", SER_ERR_NO_DATA);/* Did we get data? */
    __asm__("                  beq read_again");          /* No */

    __asm__("                  inc %v", cur);             /* Inc cur's low byte */
    __asm__("                  bne check_bound");         /* not wrapped? go check bound */
    __asm__("                  inc %v+1", cur);           /* Inc high byte */

  __asm__("check_bound:      lda %v", cur);               /* Compare cur/end low bytes */
  __asm__("                  cmp %v", end);
  __asm__("                  bne read_again_aok");        /* different, read again */
  __asm__("                  ldx %v+1", cur);             /* Compare high bytes */
  __asm__("                  cpx %v+1", end);
  __asm__("                  bne read_again_axok");       /* different, read again */
#else

  while (cur != end) {
    *cur = simple_serial_getc();
    ++cur;
  }
#endif
}
#ifdef __CC65__
#pragma optimize(pop)
#endif

void __fastcall__ simple_serial_write(char *ptr, size_t nmemb) {
  static char *cur;
  static char *end;

  cur = ptr;
  end = ptr + nmemb;

  while (cur != end) {
    simple_serial_putc(*cur);
    ++cur;
  }
}

#ifdef __CC65__
#pragma code-name (pop)
#pragma static-locals(pop)
#endif
