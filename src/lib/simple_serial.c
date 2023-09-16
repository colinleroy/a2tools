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
#include <ctype.h>
#include "extended_conio.h"
#include "extended_string.h"
#include "path_helper.h"

#ifdef __CC65__

#pragma static-locals(push, on)

static unsigned char baudrate = SER_BAUD_9600;

static char *baud_strs[] = {
  " 2400",
  " 4800",
  " 9600",
  "19200",
#ifdef IIGS
  "57600",
#endif
  NULL
};

static char baud_rates[] = {
  SER_BAUD_2400,
  SER_BAUD_4800,
  SER_BAUD_9600,
  SER_BAUD_19200,
  SER_BAUD_57600
};
#ifdef IIGS
static unsigned char slot = 0;
static char *slots_strs[] = {
  "  CHANNEL B (MODEM)",
  "CHANNEL A (PRINTER)",
  NULL
};
#define MAX_SLOT_IDX 1
#define MAX_SPEED_IDX 5
#else
static unsigned char slot = 2;
static char *slots_strs[] = {
  "1",
  "2",
  "3",
  "4",
  "5",
  "6",
  "7",
  NULL
};
#define MAX_SLOT_IDX 6
#define MAX_SPEED_IDX 4
#endif
/* Setup */
static struct ser_params default_params = {
    SER_BAUD_19200,     /* Baudrate */
    SER_BITS_8,         /* Number of data bits */
    SER_STOP_1,         /* Number of stop bits */
    SER_PAR_NONE,       /* Parity setting */
    SER_HS_HW           /* Type of handshake to use */
};

static char simple_serial_settings(const char *path, char *mode) {
  FILE *fp;

#ifdef __APPLE2ENH__
  _filetype = PRODOS_T_BIN;
  _auxtype  = PRODOS_AUX_T_TXT_SEQ;
#endif

  /* Find the cached settings */
  fp = fopen(path, mode);
  if (fp) {
    if (mode[0] == 'r') {
      fread(&baudrate, sizeof(char), 1, fp);
      fread(&slot, sizeof(char), 1, fp);
    } else {
      fwrite(&baudrate, sizeof(char), 1, fp);
      fwrite(&slot, sizeof(char), 1, fp);
    }
    fclose(fp);
    return 0;
  }

  return -1;
}

static char simple_serial_load_settings(const char *path) {
  return simple_serial_settings(path, "r");
}

static char simple_serial_save_settings(const char *path) {
  return simple_serial_settings(path, "w");
}

static void simple_serial_read_opts(void) {
  register_start_device();

  /* Find the cached settings */
  if (simple_serial_load_settings("/RAM/serialcfg") == 0) {
    reopen_start_device();
    return;
  }

  reopen_start_device();
  simple_serial_load_settings("serialcfg");
}

#ifdef SURL_TO_LANGCARD
#pragma code-name (push, "LC")
#else
#pragma code-name (push, "LOWCODE")
#endif

void simple_serial_configure(void) {
  static char cur_setting = 0;
  char c, done = 0, modified = 0;
#ifdef IIGS
  signed char slot_idx = 0;
#else
  signed char slot_idx = 1;
#endif
  signed char speed_idx = 3;
  signed char setting_offset = 0;

  #ifdef IIGS
    slot_idx = slot;
  #else
    slot_idx = slot - 1;
  #endif

  for (c = 0; c < MAX_SPEED_IDX; c++) {
    if (baud_rates[c] == baudrate) {
      speed_idx = c;
    }
  }

  clrscr();
  gotoxy(0, 0);
  cputs("Serial connection settings\r\n\r\n");
  cputs("Slot:      \r\n"
        "Baud rate: \r\n"
        "\r\n"
        "Enter to validate.");


  do {
    gotoxy(12, 2);
    revers(cur_setting == 0);
    cputs(slots_strs[slot_idx]);
    revers(0);

    gotoxy(12, 3);
    revers(cur_setting == 1);
    cputs(baud_strs[speed_idx]);
    revers(0);

    setting_offset = 0;
    c = cgetc();
    switch(c) {
      case CH_CURS_UP:
      case CH_CURS_DOWN:
        cur_setting = !cur_setting;
        break;
      case CH_CURS_LEFT:
        setting_offset = -1;
        modified = 1;
        break;
      case CH_CURS_RIGHT:
        setting_offset = +1;
        modified = 1;
        break;
      case CH_ENTER:
        done = 1;
    }

    /* update current setting */
    if (setting_offset != 0) {
      if (cur_setting == 0) {
        /* Slot */
        slot_idx += setting_offset;
        if (slot_idx > MAX_SLOT_IDX)
          slot_idx = 0;
        if (slot_idx < 0)
          slot_idx = MAX_SLOT_IDX;
      } else {
        /* Speed */
        speed_idx += setting_offset;
        if (speed_idx > MAX_SPEED_IDX)
          speed_idx = 0;
        if (speed_idx < 0)
          speed_idx = MAX_SPEED_IDX;
      }
    }
  } while (!done);
#ifdef IIGS
  slot = slot_idx;
#else
  slot = slot_idx + 1;
#endif
  baudrate = baud_rates[speed_idx];

  if (modified) {
    simple_serial_save_settings("serialcfg");
  }

  /* Cache in RAM */
  simple_serial_save_settings("/RAM/serialcfg");
  reopen_start_device();
}

int __fastcall__ simple_serial_open(void) {
  int err;

  simple_serial_read_opts();

#ifdef __APPLE2ENH__
  #ifdef IIGS
  if ((err = ser_install(&a2gs_ssc_ser)) != 0)
    return err;
  #else
  if ((err = ser_install(&a2e_ssc_ser)) != 0)
    return err;
  #endif
  if ((err = ser_apple2_slot(slot)) != 0)
    return err;
#endif

  default_params.baudrate = baudrate;

  err = ser_open (&default_params);

  if (err == 0) {
    unsigned char s;
    /* We expect the serial card to be ready to send bytes */
#ifndef IIGS
    err = ser_status(&s);
    if (err != 0 || (s & 0x10) == 0) {
      return SER_ERR_NO_DATA;
    }
#else
    err = ser_status(&s);
    if (err != 0 || (s & 0x20) == 0) {
      return SER_ERR_NO_DATA;
    }
#endif
    simple_serial_flush();
  }

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
static int bps = B19200;

static char *opt_tty_path = NULL;
static int opt_tty_speed = B19200;
static int opt_tty_hw_handshake = 1;

static const char *get_cfg_path(void) {
  return CONF_FILE_PATH;
}

static int tty_speed_from_str(char *tmp) {
  if (!strcmp(tmp, "300"))
    return B300;
  if (!strcmp(tmp, "600"))
    return B600;
  if (!strcmp(tmp, "1200"))
    return B1200;
  if (!strcmp(tmp, "2400"))
    return B2400;
  if (!strcmp(tmp, "4800"))
    return B4800;
  if (!strcmp(tmp, "9600"))
    return B9600;
  if (!strcmp(tmp, "19200"))
    return B19200;
  if (!strcmp(tmp, "38400"))
    return B38400;
  if (!strcmp(tmp, "57600"))
    return B57600;
  if (!strcmp(tmp, "115200"))
    return B115200;
  printf("Unhandled speed %s.\n", tmp);
  exit(1);
}

static char *tty_speed_to_str(int speed) {
  if (speed == B300)
    return "300";
  if (speed == B600)
    return "600";
  if (speed == B1200)
    return "1200";
  if (speed == B2400)
    return "2400";
  if (speed == B4800)
    return "4800";
  if (speed == B9600)
    return "9600";
  if (speed == B19200)
    return "19200";
  if (speed == B38400)
    return "38400";
  if (speed == B57600)
    return "57600";
  if (speed == B115200)
    return "115200";
  return "???";
}

static int get_bool(char *tmp) {
  return !strcmp(tmp, "1") 
    || !strcasecmp(tmp, "yes")
    || !strcasecmp(tmp, "true")
    || !strcasecmp(tmp, "on");
}

static void simple_serial_write_defaults(void) {
  FILE *fp = NULL;
  fp = fopen(get_cfg_path(), "w");
  if (fp == NULL) {
    printf("Cannot open %s for writing: %s\n", get_cfg_path(),
           strerror(errno));
    printf("Please create this configuration in the following format:\n\n");
    fp = stdout;
  }
  fprintf(fp, "tty: /dev/ttyUSB0\n"
              "baudrate: 19200\n"
              "hw_handshake: off\n"
              "\n"
              "#Alternatively, you can export environment vars:\n"
              "#A2_TTY (unset by default)\n"
              "#A2_TTY_SPEED (default %s)\n"
              "#A2_TTY_HW_HANDSHAKE (default %s)\n",
              tty_speed_to_str(opt_tty_speed),
              opt_tty_hw_handshake ? "true":"false");
  if (fp == stdout) {
    exit(1);
  }
  fclose(fp);
  printf("A default configuration file has been generated to %s.\n"
         "Please review it and try again.\n", get_cfg_path());
  exit(1);
}

static int simple_serial_read_opts(void) {
  FILE *fp = NULL;
  char buf[255];
  fp = fopen(get_cfg_path(), "r");
  if (fp == NULL && !getenv("A2_TTY")) {
    simple_serial_write_defaults();
    /* We'll be exit()ed there */
  }
  while (fp != NULL && fgets(buf, 255, fp)) {
    if(!strncmp(buf, "tty:", 4)) {
      free(opt_tty_path);
      opt_tty_path = trim(buf + 4);
    }

    if (!strncmp(buf,"baudrate:", 9)) {
      char *tmp = trim(buf + 9);
      opt_tty_speed = tty_speed_from_str(tmp);
      free(tmp);
    }

    if (!strncmp(buf,"hw_handshake:", 13)) {
      char *tmp = trim(buf + 13);
      opt_tty_hw_handshake = get_bool(tmp);
      free(tmp);
    }
  }
  if (fp != NULL) {
    fclose(fp);
  }
  
  /* Env vars take precedence */
  if (getenv("A2_TTY")) {
    free(opt_tty_path);
    opt_tty_path = strdup(getenv("A2_TTY"));
  }

  if (getenv("A2_TTY_SPEED")) {
    opt_tty_speed = tty_speed_from_str(getenv("A2_TTY_SPEED"));
  }

  if (getenv("A2_TTY_HW_HANDSHAKE")) {
    opt_tty_hw_handshake = get_bool(getenv("A2_TTY_HW_HANDSHAKE"));
  }
  return 0;
}

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

  if (!flow_control_enabled) {
    switch (bps) {
      case B57600:
        usleep(60);
        break;
      default:
        usleep(600);
    }
  }

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

void __fastcall__ simple_serial_read(char *ptr, size_t nmemb) {
  static char *cur;
  static char *end;

  cur = ptr;
  end = ptr + nmemb;

#ifdef __APPLE2ENH__
  __asm__("                  lda %v", cur);               /* Copy cur to ZP */
  __asm__("                  sta tmp1");
  __asm__("                  lda %v+1", cur);
  __asm__("                  sta tmp2");

  __asm__("                  lda %v", end);              /* Copy end to ZP */
  __asm__("                  sta tmp3");
  __asm__("                  lda %v+1", end);
  __asm__("                  sta tmp4");

  __asm__("                  bra check_bound");           /* branch to check cur != end */

    __asm__("read_again:       lda tmp1");                /* low byte in A */
    __asm__("read_again_aok:   ldx tmp2");                /* high byte in X */
    __asm__("read_again_axok:  jsr %v", ser_get);         /* pass cur to ser_get */
    __asm__("                  cmp #%b", SER_ERR_NO_DATA);/* Did we get data? */
    __asm__("                  beq read_again");          /* No */

    __asm__("                  inc tmp1");                /* Inc cur's low byte */
    __asm__("                  bne check_bound");         /* not wrapped? go check bound */
    __asm__("                  inc tmp2");                /* Inc high byte */

  __asm__("check_bound:      lda tmp1");                  /* Compare cur/end low bytes */
  __asm__("                  cmp tmp3");
  __asm__("                  bne read_again_aok");        /* different, read again */
  __asm__("                  ldx tmp2");                  /* Compare high bytes */
  __asm__("                  cpx tmp4");
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
