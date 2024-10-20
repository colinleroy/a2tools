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
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "malloc0.h"
#include "platform.h"
#include "extended_conio.h"
#include "strtrim.h"
#include "path_helper.h"
#include "clrzone.h"

int ttyfd = -1;
int aux_ttyfd = -1;

int flow_control_enabled;

int bps = B19200;

char *opt_tty_path = NULL;
char *opt_aux_tty_path = NULL;

static int opt_tty_speed = B115200;
static int opt_aux_tty_speed = B9600;
static int opt_tty_hw_handshake = 1;

SimpleSerialParams ser_params = {
  B115200,
  0,
  B9600,
  0
};

unsigned char data_slot = 0;
unsigned char printer_slot = 0;
unsigned int data_baudrate = B115200;
unsigned int printer_baudrate = B9600;

static const char *get_cfg_path(void) {
  return CONF_FILE_PATH;
}

const char *get_cfg_dir(void) {
  static char *cfg_dir = NULL;
  if (cfg_dir == NULL) {
    cfg_dir = strdup(get_cfg_path());
    if (strrchr(cfg_dir, '/'))
      *(strrchr(cfg_dir, '/')) = '\0';
  }
  return cfg_dir;
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
              "baudrate: %s\n"
              "hw_handshake: %s\n"
              "aux_tty: /dev/ttyUSB1\n"
              "aux_baudrate: %s\n"
              "\n"
              "#Alternatively, you can export environment vars:\n"
              "#A2_TTY (unset by default)\n"
              "#A2_TTY_SPEED (default %s)\n"
              "#A2_TTY_HW_HANDSHAKE (default %s)\n"
              "#A2_AUX_TTY (unset by default)\n"
              "#A2_AUX_TTY_SPEED (default %s)\n",
              tty_speed_to_str(opt_tty_speed),
              opt_tty_hw_handshake ? "true":"false",
              tty_speed_to_str(opt_aux_tty_speed),
              tty_speed_to_str(opt_tty_speed),
              opt_tty_hw_handshake ? "true":"false",
              tty_speed_to_str(opt_aux_tty_speed));
  if (fp == stdout) {
    exit(1);
  }
  fclose(fp);
  printf("A default serial configuration file has been generated to %s.\n"
         "Please review it and try again.\n", get_cfg_path());
  exit(1);
}

int simple_serial_read_opts(void) {
  static int opts_read_done = 0;
  FILE *fp = NULL;
  char buf[255];

  if (opts_read_done)
    return 0;
  opts_read_done = 1;

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

    if(!strncmp(buf, "aux_tty:", 8)) {
      free(opt_aux_tty_path);
      opt_aux_tty_path = trim(buf + 8);
    }

    if (!strncmp(buf,"baudrate:", 9)) {
      char *tmp = trim(buf + 9);
      opt_tty_speed = tty_speed_from_str(tmp);
      free(tmp);
    }

    if (!strncmp(buf,"aux_baudrate:", 13)) {
      char *tmp = trim(buf + 13);
      opt_aux_tty_speed = tty_speed_from_str(tmp);
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

  if (getenv("A2_AUX_TTY")) {
    free(opt_aux_tty_path);
    opt_aux_tty_path = strdup(getenv("A2_AUX_TTY"));
  }

  if (getenv("A2_TTY_SPEED")) {
    opt_tty_speed = tty_speed_from_str(getenv("A2_TTY_SPEED"));
  }

  if (getenv("A2_AUX_TTY_SPEED")) {
    opt_aux_tty_speed = tty_speed_from_str(getenv("A2_AUX_TTY_SPEED"));
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

int simple_serial_open_file(char *tty_path, int tty_speed) {
  struct flock lock;
  static char cannot_open = 0;
  int fd;

  lock.l_start = 0;
  lock.l_len = 0;
  lock.l_pid = getpid();
  lock.l_type = F_WRLCK;
  lock.l_whence = SEEK_SET;

  /* Open file */
  fd = open(tty_path, O_RDWR|O_SYNC|O_NOCTTY);

  /* Try to lock file */
  if (fd > 0 && fcntl(fd, F_SETLK, &lock) < 0) {
    printf("%s is already opened by another process.\n", tty_path);
    close(fd);
    fd = -1;
  }
  if (fd < 0) {
    if (cannot_open == 0) {
      printf("Can't open %s...\n", tty_path);
    }
    cannot_open = 1;
    return -1;
  }
  cannot_open = 0;

  simple_serial_flush_fd(fd);
  setup_tty(fd, tty_speed, opt_tty_hw_handshake);
  printf("opened %s at %sbps\n", tty_path, tty_speed_to_str(tty_speed));
  return fd;
}

int simple_serial_open(void) {
  ttyfd = simple_serial_open_file(opt_tty_path, opt_tty_speed);
  return ttyfd > 0  ? 0 : -1;
}

int simple_serial_open_printer(void) {
  aux_ttyfd = simple_serial_open_file(opt_aux_tty_path, opt_aux_tty_speed);
  return aux_ttyfd > 0  ? 0 : -1;
}

int simple_serial_close(void) {
  if (ttyfd > 0) {
    close(ttyfd);
  }
  ttyfd = -1;
  return 0;
}

int simple_serial_close_printer(void) {
  if (aux_ttyfd > 0) {
    close(aux_ttyfd);
  }
  aux_ttyfd = -1;
  return 0;
}
