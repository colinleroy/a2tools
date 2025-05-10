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
#define _DE_ISDIR(x) ((x) == DT_DIR)
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include "platform.h"
#include "clrzone.h"
#include "malloc0.h"

#define PATHNAME_MAX FILENAME_MAX
#define PRODOS_FILENAME_MAX 17
#define PRODOS_MAX_VOLUMES 37

#define VBAR "!"
#define LOWER_CORNER "!_"
#define N_UI_HELP_LINES 4

typedef struct _file_entry {
  char name[PRODOS_FILENAME_MAX];
  char is_dir;
} file_entry;

#ifdef __CC65__
extern file_entry *file_entries;
extern char last_dir[PATHNAME_MAX];

extern char scrw;

void __fastcall__ free_file_select_data (void);
unsigned char get_device_entries(void);

#else
file_entry *file_entries = NULL;
char last_dir[PATHNAME_MAX] = "";

static void __fastcall__ free_file_select_data (void) {
  free(file_entries);
  file_entries = NULL;
}
#endif

#if 0
unsigned char get_device_entries(void) {
  char n = 0, dev = getfirstdevice();

  file_entries = malloc0(sizeof(file_entry)*PRODOS_MAX_VOLUMES);
  do {
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

  return n;
}
#endif

#ifndef __CC65__
unsigned char get_dir_entries(char dir_only) {
  char n = 0;
  DIR *d = opendir(last_dir);
  struct dirent *ent;
  if (d) {
    file_entries = malloc0(sizeof(file_entry)*dir_entry_count(d));
    while ((ent = readdir(d))) {
      if (dir_only && !_DE_ISDIR(ent->d_type))
        continue;

      strcpy(file_entries[n].name, ent->d_name);
      file_entries[n].is_dir = _DE_ISDIR(ent->d_type);
      n++;
    }
    closedir(d);
  }

  return n;
}

static void start_line (void) {
  cputs(VBAR);
}

static void end_line (void) {
  clreol();
  cputs("\r\n");
}

static void empty_line(void) {
  start_line();
  end_line();
}

char enter_directory(char sel) {
  if (!file_entries[sel].is_dir) {
    return 0;
  }

  if (file_entries[sel].name[0] != '/')
    strcat(last_dir, "/");
  strcat(last_dir, file_entries[sel].name);
  return 1;
}

char exit_directory(void) {
  char *idx;

  if (!strlen(last_dir)) {
    return 0;
  }

  idx = strrchr(last_dir, '/');

  *idx = '\0';
  return 1;
}

char *build_filename(char sel, char dir) {
  char *filename;

  if (file_entries[sel].is_dir != dir) {
    return NULL;
  }

  filename = malloc0(PATHNAME_MAX+1);
  #ifndef __CC65__
  #pragma GCC diagnostic ignored "-Wformat-truncation"
  #endif
  if (last_dir[0] != '\0') {
    strcpy(filename, last_dir);
    strcat(filename, "/");
  }

  strcat(filename, file_entries[sel].name);
  return filename;
}

int c, sel, i, n, start, stop, loop_count, sx, sy, ey;
char *file_select(char dir, char *prompt) {
  static char *filename;

  filename = NULL;
  sx = wherex();
  sy = wherey();
  loop_count = 10;
  ey = sy + loop_count + N_UI_HELP_LINES;

  cputs("Please wait...");
  if (dir)
    last_dir[0] = '\0';

  n = 0;

list_again:
  free_file_select_data();

  n = sel = start = 0;

#ifdef __CC65__
  if (last_dir[0] == '\0') {
    n = get_device_entries();
  } else {
    n = get_dir_entries(dir);
  }
#else
  if (last_dir[0] == '\0') {
    last_dir[0] = '/';
  }
  get_dir_entries(dir);
#endif

  clrzone(sx, sy, 100, ey);
disp_again:
  gotoxy(sx, sy);

  /* Print header */
  start_line();
  cputs(prompt);
  end_line();

  if (n == 0) {
    /* print empty directory */
    gotox(sx); cputs(VBAR" *"); cputs(dir ? "No directory*":"Empty*"); end_line();
    gotox(sx); empty_line();
    gotox(sx); cputs(LOWER_CORNER" Any key to go up");
    cgetc();
    goto up;
  }

  /* Calculate bounds */
  if (sel - loop_count >= start) {
    start = sel - loop_count + 1;
  }
  if (sel < start) {
    start = sel;
  }
  stop = start + loop_count;
  if (stop > n) {
    stop = n;
  }

  /* Print entries */
  for (i = start; i < stop; i++) {
    revers(0);
    gotox(sx);
    cputs(VBAR" ");
    revers(i == sel);
    cputs(file_entries[i].name);
    if (file_entries[i].is_dir) {
      cputc('/');
    }
    end_line();
  }

  /* Print footer */
  revers(0);
  gotox(sx);empty_line();
  gotox(sx);cputs(VBAR"  U/J / Left/Right: navigate;\r\n");
  gotox(sx);cputs(VBAR"  R: refresh;\r\n");
  gotox(sx);cputs(LOWER_CORNER" Enter: choose; Esc: cancel");

  /* Get key */
  c = tolower(cgetc());
  switch (c) {
    case 'r':
      goto list_again;
    case CH_CURS_RIGHT:
      if (enter_directory(sel)) {
        goto list_again;
      }
err_bell:
#ifdef __CC65__
      beep();
#endif
      goto disp_again;

    case CH_CURS_LEFT:
up:
      if (exit_directory()) {
        goto list_again;
      }
      goto err_bell;

    case CH_ESC:
      goto out;

    case CH_ENTER:
      if ((filename = build_filename(sel, dir)) != NULL) {
        goto out;
      }
      goto err_bell;

    case CH_CURS_DOWN:
    case 'j':
      sel++;
      if (sel == n) {
        sel = 0;
      }
      goto disp_again;

    case CH_CURS_UP:
    case 'u':
      if (sel > 0) {
        sel--;
      } else {
        sel = n - 1;
      }
      goto disp_again;
  }
  goto disp_again;
out:
  free_file_select_data();
  clrzone(sx, sy, 100, ey);

  if (filename) {
    cputs(filename);
  }
  return filename;
}

#else
unsigned char get_dir_entries(char dir_only);
void start_line (void);
void end_line (void);
void empty_line (void);

char enter_directory(char sel);
char exit_directory(void);
char *build_filename(char sel, char dir);
char *file_select(char sx, char sy, char ey, char dir, char *prompt);
#endif
