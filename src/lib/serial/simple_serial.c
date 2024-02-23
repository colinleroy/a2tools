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
#include "malloc0.h"
#include "platform.h"
#include "extended_conio.h"
#include "strtrim.h"
#include "path_helper.h"
#include "clrzone.h"

#ifdef __CC65__
#include <accelerator.h>

#pragma static-locals(push, on)

unsigned char baudrate = 0;
unsigned char data_baudrate = SER_BAUD_115200;
unsigned char printer_baudrate = SER_BAUD_9600;
unsigned char flow_control = SER_HS_HW;

  unsigned char open_slot = 0;
#ifdef IIGS
  static char *baud_strs[] = {
    "  2400",
    "  4800",
    "  9600",
    " 19200",
    " 57600",
    "115200",
    NULL
  };

  static char baud_rates[] = {
    SER_BAUD_2400,
    SER_BAUD_4800,
    SER_BAUD_9600,
    SER_BAUD_19200,
    SER_BAUD_57600,
    SER_BAUD_115200
  };

  unsigned char data_slot = 0;
  unsigned char printer_slot = 1;
  static char *slots_strs[] = {
    "MODEM  ",
    "PRINTER",
    NULL
  };
  #define MAX_SLOT_IDX 1
  #define MAX_SPEED_IDX 5
#else
  static char *baud_strs[] = {
    " 2400",
    " 4800",
    " 9600",
    "19200",
    "115200",
    NULL
  };

  static char baud_rates[] = {
    SER_BAUD_2400,
    SER_BAUD_4800,
    SER_BAUD_9600,
    SER_BAUD_19200,
    SER_BAUD_115200
  };

  unsigned char data_slot = 2;
  unsigned char printer_slot = 1;
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
    SER_BAUD_115200,    /* Baudrate */
    SER_BITS_8,         /* Number of data bits */
    SER_STOP_1,         /* Number of stop bits */
    SER_PAR_NONE,       /* Parity setting */
    SER_HS_HW           /* Type of handshake to use */
};

#pragma code-name (push, "RT_ONCE")

static char simple_serial_settings_io(const char *path, char *mode) {
  FILE *fp;

#ifdef __APPLE2__
  _filetype = PRODOS_T_BIN;
  _auxtype  = PRODOS_AUX_T_TXT_SEQ;
#endif

  /* Find the cached settings */
  fp = fopen(path, mode);
  if (fp) {
    if (mode[0] == 'r') {
      fread(&data_baudrate, sizeof(char), 1, fp);
      fread(&data_slot, sizeof(char), 1, fp);
      fread(&printer_baudrate, sizeof(char), 1, fp);
      fread(&printer_slot, sizeof(char), 1, fp);
    } else {
      fwrite(&data_baudrate, sizeof(char), 1, fp);
      fwrite(&data_slot, sizeof(char), 1, fp);
      fwrite(&printer_baudrate, sizeof(char), 1, fp);
      fwrite(&printer_slot, sizeof(char), 1, fp);
    }
    fclose(fp);
    return 0;
  }

  return -1;
}

static void simple_serial_read_opts(void) {
  register_start_device();

  /* Find the cached settings */
  if (simple_serial_settings_io("/RAM/serialcfg", "r") == 0) {
    reopen_start_device();
    return;
  }

  reopen_start_device();
  simple_serial_settings_io("serialcfg", "r");
}

signed char __fastcall__ bind_val(signed char val, signed char min, signed char max) {
  if (val > max)
    return min;
  if (val < min)
    return max;
  return val;
}

void simple_serial_configure(void) {
  static signed char cur_setting = 0;
  char c, done = 0, modified = 0;
#ifdef IIGS
  signed char slot_idx = 0;
  signed char printer_slot_idx = 1;
  signed char speed_idx = 5;
#else
  signed char slot_idx = 1;
  signed char printer_slot_idx = 0;
  signed char speed_idx = 4;
#endif
  signed char printer_speed_idx = 2;
  signed char setting_offset = 0;

  #ifdef IIGS
    slot_idx = data_slot;
    printer_slot_idx = printer_slot;
  #else
    slot_idx = data_slot - 1;
    printer_slot_idx = printer_slot - 1;
  #endif

  for (c = 0; c < MAX_SPEED_IDX; c++) {
    if (baud_rates[c] == data_baudrate) {
      speed_idx = c;
    }
    if (baud_rates[c] == printer_baudrate) {
      printer_speed_idx = c;
    }
  }

  /* Make sure we don't have an opened serial
   * port lingering around.
   */
  simple_serial_close();

  clrscr();
  gotoxy(0, 0);
  cputs("Serial connection\r\n\r\n");
  cputs("Data slot:      \r\n"
        "Baud rate:      \r\n"
        "\r\n"
        "Printer slot:   \r\n"
        "Baud rate:      \r\n"
        "\r\n"
#ifdef __APPLE2ENH__
        "Up/down/left/right to configure,\r\n"
#else
        "Space/left/right to configure,\r\n"
#endif
        "Enter to validate.");


  do {
    gotoxy(15, 2);
    revers(cur_setting == 0);
    cputs(slots_strs[slot_idx]);

    gotoxy(15, 3);
    revers(cur_setting == 1);
    cputs(baud_strs[speed_idx]);

    gotoxy(15, 5);
    revers(cur_setting == 2);
    cputs(slots_strs[printer_slot_idx]);

    gotoxy(15, 6);
    revers(cur_setting == 3);
    cputs(baud_strs[printer_speed_idx]);

    revers(0);

    setting_offset = 0;
    c = cgetc();
    switch(c) {
#ifdef __APPLE2ENH__
      case CH_CURS_UP:
        cur_setting = bind_val(cur_setting - 1, 0, 3);
        break;
      case CH_CURS_DOWN:
#else
      case ' ':
#endif
        cur_setting = bind_val(cur_setting + 1, 0, 3);
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
        slot_idx = bind_val(slot_idx + setting_offset, 0, MAX_SLOT_IDX);
      } else if (cur_setting == 1) {
        /* Speed */
        speed_idx = bind_val(speed_idx + setting_offset, 0, MAX_SPEED_IDX);
      } else if (cur_setting == 2) {
        /* Printer Slot */
        printer_slot_idx = bind_val(printer_slot_idx + setting_offset, 0, MAX_SLOT_IDX);
      } else if (cur_setting == 3) {
        /* Printer Speed */
        printer_speed_idx = bind_val(printer_speed_idx + setting_offset, 0, MAX_SPEED_IDX);
      }
    }
  } while (!done);
#ifdef IIGS
  data_slot = slot_idx;
  printer_slot = printer_slot_idx;
#else
  data_slot = slot_idx + 1;
  printer_slot = printer_slot_idx + 1;
#endif
  data_baudrate = baud_rates[speed_idx];
  printer_baudrate = baud_rates[printer_speed_idx];

  if (modified) {
    simple_serial_settings_io("serialcfg", "w");
  }

  /* Cache in RAM */
  simple_serial_settings_io("/RAM/serialcfg", "w");
  reopen_start_device();
}

static char __fastcall__ simple_serial_open_slot(unsigned char my_slot) {
  char err;

  open_slot = my_slot;

#ifdef __APPLE2ENH__
  #ifdef IIGS
  if ((err = ser_install(&a2e_gs_ser)) != 0)
    return err;
  #else
  if ((err = ser_install(&a2e_ssc_ser)) != 0)
    return err;
  #endif

  if ((err = ser_apple2_slot(open_slot)) != 0)
    return err;
#else
  #ifdef __APPLE2__
    #ifdef IIGS
    if ((err = ser_install(&a2_gs_ser)) != 0)
      return err;
    #else
    if ((err = ser_install(&a2_ssc_ser)) != 0)
      return err;
    #endif

    if ((err = ser_apple2_slot(open_slot)) != 0)
      return err;
  #endif
#endif
  default_params.baudrate = baudrate;
  default_params.handshake = flow_control;

  err = ser_open (&default_params);

  if (err == 0) {
    simple_serial_set_irq(1);
    simple_serial_flush();
  }

  return err;
}

char __fastcall__ simple_serial_open(void) {
  simple_serial_read_opts();
  if (baudrate == 0)
    baudrate = data_baudrate;
  return simple_serial_open_slot(data_slot);
}

char __fastcall__ simple_serial_open_printer(void) {
  simple_serial_read_opts();
  if (baudrate == 0)
    baudrate = printer_baudrate;
  return simple_serial_open_slot(printer_slot);
}

char __fastcall__ simple_serial_close(void) {
  ser_close();
  baudrate = 0;
  return ser_uninstall();
}

#pragma code-name (pop)

#ifdef SURL_TO_LANGCARD
#pragma code-name (push, "LC")
#else
#pragma code-name (push, "LOWCODE")
#endif

#pragma optimize(push, on)

static uint16 timeout_cycles = 0;
#ifdef IIGS
uint8 orig_speed_reg; /* For IIgs */
#endif

/* Input */
int __fastcall__ simple_serial_getc_with_timeout(void) {
  static char c;
  static char prev_spd;
  timeout_cycles = 10000U;
  prev_spd = get_iigs_speed();
  set_iigs_speed(SPEED_SLOW);
  while (ser_get(&c) == SER_ERR_NO_DATA) {
    if (--timeout_cycles == 0) {
      set_iigs_speed(prev_spd);
      return EOF;
    }
  }
  set_iigs_speed(prev_spd);
  return (int)c;
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

FILE *ttyfp = NULL;
int flow_control_enabled;

extern char *readbuf;
int bps = B19200;

static char *opt_tty_path = NULL;
static int opt_tty_speed = B115200;
static int opt_tty_hw_handshake = 1;

unsigned char data_slot = 0;
unsigned char printer_slot = 0;
unsigned int data_baudrate = B115200;
unsigned int printer_baudrate = B9600;

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

char *tty_speed_to_str(int speed) {
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
              "baudrate: 115200\n"
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

static int simple_serial_open_slot(int slot) {
  struct flock lock;

  lock.l_start = 0;
  lock.l_len = 0;
  lock.l_pid = getpid();
  lock.l_type = F_WRLCK;
  lock.l_whence = SEEK_SET;

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

int simple_serial_open(void) {
  /* Get options */
  simple_serial_read_opts();
  return simple_serial_open_slot(0);
}

int simple_serial_open_printer(void) {
  /* Get options */
  simple_serial_read_opts();
  return simple_serial_open_slot(0);
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

#endif /* End of platform-dependant code */

#ifdef __CC65__
#pragma code-name (pop)
#pragma static-locals(pop)
#endif