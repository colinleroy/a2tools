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
#include <device.h>
#else
#define _DE_ISDIR(x) ((x) == DT_DIR)
#endif
#include "extended_conio.h"
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include "clrzone.h"
#include "dputc.h"
#include "malloc0.h"

static char last_dir[FILENAME_MAX] = "";

static char **list = NULL;
static char *is_dir = NULL;

static void __fastcall__ free_data (char n) {
  static char i;
  for (i = 0; i < n; i++) {
    free(list[i]);
  }
  free(list);
  free(is_dir);
  list = NULL;
  is_dir = NULL;
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
  free_data(n);

  n = 0;
  sel = 0;
  start = 0;

  if (last_dir[0] == '\0') {
#ifdef __CC65__
    char **cur_list;
    char *cur_is_dir;
    list = malloc0(sizeof(char *) * 58);
    is_dir = malloc0(sizeof(char) * 58);
    dev = getfirstdevice();
    cur_list = list;
    cur_is_dir = is_dir;
    do {

      *cur_list = malloc0(17);
      if (getdevicedir(dev, *cur_list, 17) == NULL) {
        free(*cur_list);
        continue;
      }
      *cur_is_dir = 1;
      n++;
      cur_list++;
      cur_is_dir++;
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

        list = realloc_safe(list, sizeof(char *) * (n + 1));
        is_dir = realloc_safe(is_dir, sizeof(char) * (n + 1));

        list[n] = strdup(ent->d_name);
        is_dir[n] = _DE_ISDIR(ent->d_type);
        n++;
      }
      closedir(d);
    }
  }
full_disp_again:
  clrzone(l_sx, sy, ex, ey);
disp_again:
  gotoxy(l_sx, sy);
  cprintf("-- %s\r\n", prompt);
  if (n == 0) {
    gotox(l_sx); cprintf("! *%s*\r\n", dir ? "No directory":"Empty");
    gotox(l_sx); cputs("!\r\n");
    gotox(l_sx); cputs("-- Any key to go up");
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
    cputs("! ");
    revers(i == sel);
    cprintf("%s\r\n", list[i]);
  }
  revers(0);

  gotox(l_sx);cputs("! \r\n");
#ifdef __APPLE2ENH__
  gotox(l_sx);cputs("!  Up/Down / Left/Right: navigate;\r\n");
#else
  gotox(l_sx);cputs("!  U/J / Left/Right: navigate;\r\n");
#endif
  gotox(l_sx);cputs("-- Enter: select; Esc: cancel");

  c = tolower(cgetc());
  switch (c) {
    case CH_CURS_RIGHT:
      if (!is_dir[sel]) {
err_bell:
        dputc(0x07);
        break;
      }
      if (list[sel][0] != '/')
        strcat(last_dir, "/");
      strcat(last_dir, list[sel]);
      goto list_again;
    case CH_CURS_LEFT:
up:
      if (strrchr(last_dir, '/'))
        *strrchr(last_dir, '/') = '\0';
      goto list_again;
    case CH_ESC:
      goto out;
    case CH_ENTER:
      if (is_dir[sel] != dir) {
        goto err_bell;
      } else {
        filename = malloc0(FILENAME_MAX);
        snprintf(filename, FILENAME_MAX, "%s%s%s", last_dir, (last_dir[0] != '\0' ? "/":""), list[sel]);
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
  free_data(n);
  clrzone(l_sx, sy, ex, ey);

  if (filename) {
    cputs(filename);
  }
  return filename;
}
