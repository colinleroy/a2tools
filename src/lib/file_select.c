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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#ifdef __CC65__
#include <dio.h>
#include <device.h>
#include <apple2.h>
#else
#define _DE_ISDIR(x) ((x) == DT_DIR)
#endif
#include "extended_conio.h"
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include "clrzone.h"
#include "malloc0.h"

static char last_dir[FILENAME_MAX] = "";

typedef struct _file_entry {
  char name[FILENAME_MAX+1];
  char is_dir;
} file_entry;

file_entry *file_entries = NULL;

static void __fastcall__ free_data (void) {
  free(file_entries);
  file_entries = NULL;
}

char *file_select(char sx, char sy, char ex, char ey, char dir, char *prompt) {
  static char l_sx;
#ifdef __CC65__
  static char dev, c, sel, i, n, start, stop, loop_stop;
#else
  int c, sel, i, n, start, stop, loop_stop;
#endif
  static char *filename;

  filename = NULL;
  l_sx = sx;

  gotoxy(l_sx, sy);

  cputs("Please wait...");
  if (dir)
    last_dir[0] = '\0';

  n = 0;

list_again:
  free_data();

  n = 0;
  sel = 0;
  start = 0;

  if (last_dir[0] == '\0') {
#ifdef __CC65__
    dev = getfirstdevice();
    do {
      file_entries = realloc_safe(file_entries, sizeof(file_entry)*(n+1));
      if (getdevicedir(dev, file_entries[n].name, 17) == NULL) {
#ifdef FILESEL_ALLOW_NONPRODOS_VOLUMES
        int blocks;
        dhandle_t dev_handle = dio_open(dev);
        if (!dev_handle) {
          continue;
        }
        blocks = dio_query_sectcount(dev_handle);
        dio_close(dev_handle);
        if (blocks != 280U) {
          continue;
        }
        /* Dev: 0000DSSS (as ProDOS, but shifted >>4)*/
        sprintf(file_entries[n].name, "S%dD%d",
          (dev & 0x07), (dev & 0x08) ? 2:1);
#else
        continue;
#endif
      }
      file_entries[n].is_dir = 1;
      n++;
    } while ((dev = getnextdevice(dev)) != INVALID_DEVICE);
#else
  last_dir[0] = '/';
  goto posix_use_dir;
#endif
  } else {
#ifndef __CC65__
posix_use_dir:
#endif
    DIR *d = opendir(last_dir);
    struct dirent *ent;
    if (d) {
      while ((ent = readdir(d))) {
        if (dir && !_DE_ISDIR(ent->d_type))
          continue;

        file_entries = realloc_safe(file_entries, sizeof(file_entry)*(n+1));

        strcpy(file_entries[n].name, ent->d_name);
        file_entries[n].is_dir = _DE_ISDIR(ent->d_type);
        n++;
      }
      closedir(d);
    }
  }
#ifdef __APPLE2ENH__
#define VBAR "\337"
#define LOWER_CORNER "\324\137"
#else
#define VBAR "!"
#define LOWER_CORNER "!_"
#endif
full_disp_again:
  clrzone(l_sx, sy, ex, ey);
disp_again:
  gotoxy(l_sx, sy);
  cprintf(VBAR"%s\r\n", prompt);
  if (n == 0) {
    gotox(l_sx); cprintf(VBAR" *%s*\r\n", dir ? "No directory":"Empty");
    gotox(l_sx); cputs(VBAR"\r\n");
    gotox(l_sx); cputs(LOWER_CORNER" Any key to go up");
    cgetc();
    goto up;
  }

  loop_stop = stop = ey - sy - 3 + start;
  if (n < stop) {
    loop_stop = n;
  }

  for (i = start; i < loop_stop; i++) {
    revers(0);
    gotox(l_sx);
    cputs(VBAR" ");
    revers(i == sel);
    cprintf("%s\r\n", file_entries[i].name);
  }
  revers(0);

  gotox(l_sx);cputs(VBAR" \r\n");
#ifdef __APPLE2ENH__
  gotox(l_sx);cputs(VBAR"  Up/Down / Left/Right: navigate;\r\n");
#else
  gotox(l_sx);cputs(VBAR"  U/J / Left/Right: navigate;\r\n");
#endif
  gotox(l_sx);cputs(LOWER_CORNER" Enter: select; Esc: cancel");

  c = tolower(cgetc());
  switch (c) {
    case CH_CURS_RIGHT:
      if (!file_entries[sel].is_dir) {
err_bell:
#ifdef __CC65__
        beep();
#endif
        break;
      }
      if (file_entries[sel].name[0] != '/')
        strcat(last_dir, "/");
      strcat(last_dir, file_entries[sel].name);
      goto list_again;
    case CH_CURS_LEFT:
up:
      if (strrchr(last_dir, '/'))
        *strrchr(last_dir, '/') = '\0';
      goto list_again;
    case CH_ESC:
      goto out;
    case CH_ENTER:
      if (file_entries[sel].is_dir != dir) {
        goto err_bell;
      } else {
        filename = malloc0(FILENAME_MAX+1);
        #ifndef __CC65__
        #pragma GCC diagnostic ignored "-Wformat-truncation"
        #endif
        snprintf(filename, FILENAME_MAX, "%s%s%s", last_dir, (last_dir[0] != '\0' ? "/":""),
                 file_entries[sel].name);
        goto out;
      }
#ifdef __APPLE2ENH__
    case CH_CURS_DOWN:
#else
    case 'j':
#endif
      if (sel < n - 1) {
        sel++;
      }
      if (sel == stop) {
        start++;
        goto full_disp_again;
      }
      break;
#ifdef __APPLE2ENH__
    case CH_CURS_UP:
#else
    case 'u':
#endif
      if (sel > 0) {
        sel--;
      }
      if (sel < start) {
        start--;
        goto full_disp_again;
      }
      break;
  }
  goto disp_again;
out:
  free_data();
  clrzone(l_sx, sy, ex, ey);

  if (filename) {
    cputs(filename);
  }
  return filename;
}
