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
#include <accelerator.h>
#include "malloc0.h"
#include "platform.h"
#include "extended_conio.h"
#include "strtrim.h"
#include "path_helper.h"
#include "clrzone.h"

#pragma static-locals(push, on)

unsigned char baudrate = 0;
unsigned char flow_control = SER_HS_HW;

unsigned char open_slot = 0;

SimpleSerialParams ser_params = {
  SER_BAUD_115200,
  MODEM_SER_SLOT,
  SER_BAUD_9600,
  PRINTER_SER_SLOT
};

/* Setup */
static struct ser_params default_params = {
    SER_BAUD_115200,    /* Baudrate */
    SER_BITS_8,         /* Number of data bits */
    SER_STOP_1,         /* Number of stop bits */
    SER_PAR_NONE,       /* Parity setting */
    SER_HS_HW           /* Type of handshake to use */
};

#pragma code-name (push, "RT_ONCE")

char simple_serial_settings_io(const char *path, char *mode) {
  FILE *fp;

  _filetype = PRODOS_T_BIN;
  _auxtype  = PRODOS_AUX_T_TXT_SEQ;

  /* Find the cached settings */
  fp = fopen(path, mode);
  if (fp) {
    if (mode[0] == 'r') {
      fread(&ser_params, sizeof(SimpleSerialParams), 1, fp);
    } else {
      fwrite(&ser_params, sizeof(SimpleSerialParams), 1, fp);
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
#else
  if ((err = ser_install(&a2_ssc_ser)) != 0)
    return err;
#endif

  if ((err = ser_apple2_slot(open_slot)) != 0)
    return err;

  default_params.baudrate = baudrate;
  default_params.handshake = flow_control;

  err = ser_open (&default_params);

  return err;
}

char __fastcall__ simple_serial_open(void) {
  simple_serial_read_opts();
  if (baudrate == 0)
    baudrate = ser_params.data_baudrate;
  return simple_serial_open_slot(ser_params.data_slot);
}

char __fastcall__ simple_serial_open_printer(void) {
  simple_serial_read_opts();
  if (baudrate == 0)
    baudrate = ser_params.printer_baudrate;
  return simple_serial_open_slot(ser_params.printer_slot);
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
#pragma code-name (pop)
#pragma static-locals(pop)
