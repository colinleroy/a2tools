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

extern unsigned char baudrate;
extern unsigned char data_baudrate;
extern unsigned char printer_baudrate;

extern unsigned char flow_control;
extern unsigned char data_slot;
extern unsigned char printer_slot;

extern unsigned char open_slot;

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
#pragma code-name (push, "RT_ONCE")

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

#pragma code-name (pop)
#pragma static-locals(pop)

#endif
