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

#ifdef __CC65__
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <conio.h>
#include <ctype.h>
#include <errno.h>
#include <device.h>
#include <dirent.h>
#include "clrzone.h"

static char last_dir[FILENAME_MAX] = "";

static void bell(void) {
  __asm__("bit     $C082");
  __asm__("jsr     $FBE4");
  __asm__("bit     $C080");
}

char *file_select(char sx, char sy, char ex, char ey, char dir, char *prompt) {
  char **list = NULL;
  char *is_dir = NULL;
  char dev, c, sel = 0, i, n = 0, start = 0;
  char *filename = NULL;

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
  } else {
    DIR *dir = opendir(last_dir);
    struct dirent *d;
    while (d = readdir(dir)) {
      n++;
      list = realloc(list, sizeof(char *) * n);
      is_dir = realloc(is_dir, sizeof(char) * n);

      list[n - 1] = strdup(d->d_name);
      is_dir[n - 1] = _DE_ISDIR(d->d_type);
    }
    closedir(dir);
  }
full_disp_again:
  clrzone(sx, sy, ex, ey);
disp_again:
  gotoxy(sx, sy);
  printf("-- %s\n", prompt);
  for (i = start; i < n && i - start < ey - sy - 3; i++) {
    revers(0);
    cputs("! ");
    revers(i == sel);
    printf("%s\n", list[i]);
  }
  revers(0);
  printf("! \n"
         "!  Up/Down: navigate; Right: enter dir;\n");
  printf("-- Esc: exit dir; Enter: select");
  c = tolower(cgetc());
  switch (c) {
    case CH_CURS_RIGHT:
      if (!is_dir[sel]) {
        bell();
        goto disp_again;
      }
      if (list[sel][0] != '/')
        strcat(last_dir, "/");
      strcat(last_dir, list[sel]);
      goto list_again;
    case CH_ESC:
      if (last_dir[0] == '\0')
        return NULL;
      *strrchr(last_dir, '/') = '\0';
      goto list_again;
    case CH_ENTER:
      filename = malloc(FILENAME_MAX);
      snprintf(filename, FILENAME_MAX, "%s/%s", last_dir, list[sel]);
      if (is_dir[sel] != dir) {
        bell();
        free(filename);
        goto disp_again;
      } else {
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
  printf("%s", filename);
  return filename;
}

#else
char *file_select(char sx, char sy, char w, char h, char dir_only) {
  return 0;
}
#endif
