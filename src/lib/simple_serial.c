#include "simple_serial.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef __CC65__
#include <apple2.h>

#define NCYCLES_PER_SEC 10000U

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

int simple_serial_open(int slot, int baudrate) {
  int err;
  
  if ((err = ser_install(&a2_ssc_ser)) != 0)
    return err;

  if ((err = ser_apple2_slot(slot)) != 0)
    return err;

  default_params.baudrate = baudrate;

  err = ser_open (&default_params);

  last_slot = slot;
  last_baudrate = baudrate;

  return err;
}

int simple_serial_close(void) {
  return ser_close();
}

static long timeout_cycles = -1;
static int timeout_secs = -1;

void simple_serial_set_timeout(int timeout) {
  timeout_secs = timeout;
}

static void serial_timeout_init() {
  if (timeout_secs < 0)
    return;

  timeout_cycles = NCYCLES_PER_SEC * timeout_secs;
}

static int serial_timeout_reached(void) {
  if (timeout_cycles < 0)
    return 0;

  timeout_cycles--;

  return timeout_cycles == 0;
}

static void serial_timeout_reset(void) {
  timeout_cycles = -1;
}

/* Input */
static int __simple_serial_getc_with_timeout(int with_timeout) {
    char c;

    serial_timeout_init();
    while (ser_get(&c) == SER_ERR_NO_DATA){
      if (with_timeout && serial_timeout_reached()) {
        serial_timeout_reset();
        return EOF;
      }
    }

    serial_timeout_reset();

    return (int)c;
}

static char *__simple_serial_gets_with_timeout(char *out, size_t size, int with_timeout) {
  int b;
  char c;
  size_t i = 0;

  if (size == 0) {
    return NULL;
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

  return out;
}

static size_t __simple_serial_read_with_timeout(char *ptr, size_t size, size_t nmemb, int with_timeout) {
  int b;
  size_t i = 0;
  size_t tries = 0;

  if (size != 1) {
    /* unsupported */
    return 0;
  }

  while (i < (nmemb - 1)) {
    b = __simple_serial_getc_with_timeout(with_timeout);
    if (b == EOF) {
      break;
    }
    ptr[i] = (char)b;
    i++;
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

  for (i = 0; i < len; i++) {
    if ((r = simple_serial_putc(buf[i])) == EOF)
      return EOF;
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
static void setup_tty(const char *ttypath, int baudrate) {
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
  tty.c_cflag &= ~CRTSCTS;
  tty.c_cflag |= CREAD | CLOCAL;

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

int simple_serial_open(const char *tty, int baudrate) {
  setup_tty(tty, baudrate);

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

void simple_serial_set_timeout(int timeout) {
  printf("Not implemented.\n");
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

  usleep(DELAY_MS*1000);
  return r;
}
int simple_serial_puts(char *buf) {
  int r = fputs(buf, ttyfp);
  fflush(ttyfp);

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
