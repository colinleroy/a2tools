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
#include "dputc.h"
#include "dputs.h"
#include "clrzone.h"

static char last_dir[FILENAME_MAX] = "";

char *file_select(char sx, char sy, char ex, char ey, char dir, char *prompt) {
  char **list = NULL;
  char *is_dir = NULL;
#ifdef __CC65__
  char dev, c, sel = 0, i, n = 0, start = 0;
#else
  int c, sel = 0, i, n = 0, start = 0;
#endif
  char *filename = NULL;

  gotoxy(sx, sy);
  dputs("Please wait...");
  if (dir)
    last_dir[0] = '\0';

list_again:
  for (i = 0; i < n; i++) {
    free(list[i]);
  }
  free(list);
  free(is_dir);
  list = NULL;
  is_dir = NULL;

  n = 0;
  sel = 0;
  start = 0;

  if (last_dir[0] == '\0') {
#ifdef __CC65__
    dev = getfirstdevice();
    do {
      n++;
      list = realloc(list, sizeof(char *) * n);
      is_dir = realloc(is_dir, sizeof(char) * n);

      list[n - 1] = malloc(FILENAME_MAX);
      if (getdevicedir(dev, list[n - 1], FILENAME_MAX) == NULL) {
        n--;
        free(list[n]);
        continue;
      }
      is_dir[n - 1] = 1;
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

        n++;
        list = realloc(list, sizeof(char *) * n);
        is_dir = realloc(is_dir, sizeof(char) * n);

        list[n - 1] = strdup(ent->d_name);
        is_dir[n - 1] = _DE_ISDIR(ent->d_type);
      }
      closedir(d);
    }
  }
full_disp_again:
  clrzone(sx, sy, ex, ey);
disp_again:
  gotoxy(sx, sy);
  cprintf("-- %s\r\n", prompt);
  if (n == 0) {
    gotox(sx); cprintf("! *%s*\r\n", dir ? "No directory":"Empty");
    gotox(sx); dputs("!\r\n");
    gotox(sx); dputs("-- Any key to go up");
    cgetc();
    goto up;
  }
  for (i = start; i < n && i - start < ey - sy - 3; i++) {
    revers(0);
    gotox(sx);
    cputs("! ");
    revers(i == sel);
    cprintf("%s\r\n", list[i]);
  }
  revers(0);

  gotox(sx);dputs("! \r\n");
  gotox(sx);dputs("!  Up/Down/Left/Right: navigate;\r\n");
  gotox(sx);dputs("-- Enter: select; Esc: cancel");

  c = tolower(cgetc());
  switch (c) {
    case CH_CURS_RIGHT:
      if (!is_dir[sel]) {
        dputc('\07');
        goto disp_again;
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
        dputc('\07');
        goto disp_again;
      } else {
        filename = malloc(FILENAME_MAX);
        snprintf(filename, FILENAME_MAX, "%s%s%s", last_dir, (last_dir[0] != '\0' ? "/":""), list[sel]);
        goto out;
      }
    case CH_CURS_DOWN:
      if (sel < n - 1)
        sel++;
      if (sel == start + ey - sy - 3) {
        start++;
        goto full_disp_again;
      }
      break;
    case CH_CURS_UP:
      if (sel > 0)
        sel--;
      if (sel < start) {
        start--;
        goto full_disp_again;
      }
      break;
  }
  goto disp_again;
out:
  for (i = 0; i < n; i++) {
    free(list[i]);
  }
  free(list);
  free(is_dir);
  clrzone(sx, sy, ex, ey);
  gotoxy(sx, sy);
  if (filename) {
    dputs(filename);
  }
  return filename;
}
