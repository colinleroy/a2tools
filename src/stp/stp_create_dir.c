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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include "platform.h"
#include "file_select.h"
#include "stp_list.h"
#include "stp_cli.h"
#include "stp_send_file.h"
#include "simple_serial.h"
#include "clrzone.h"
#include "extended_conio.h"
#include "progress_bar.h"
#include "get_buf_size.h"
#include "math.h"

#define BUFSIZE 255

#ifndef __CC65__
#define _DE_ISDIR(x) ((x) == DT_DIR)
#endif

static char *stp_mkdir_dialog(void) {
  char *filename = malloc(16);
  clrzone(0, 2, NUMCOLS - 1, 2 + PAGE_HEIGHT);
  gotoxy(0, 3);
  cprintf("New directory name: ");

  filename[0] = '\0';
  dget_text_single(filename, 16, NULL);
  if (filename[0] == '\0') {
    free(filename);
    return NULL;
  }
  return filename;
}

void stp_create_dir(char *remote_dir) {
  static char *dir, *path;

  dir = stp_mkdir_dialog();
  if (IS_NULL(dir)) {
    return;
  }

  path = malloc(strlen(remote_dir) + 1 + strlen(dir) + 1 + 1);
  if (IS_NULL(path)) {
    free(dir);
    return;
  }

  sprintf(path, "%s/%s/", remote_dir, dir);
  free(dir);

  surl_start_request(NULL, 0, path, SURL_METHOD_MKDIR);
  free(path);
  stp_print_result();

  clrzone(0, 5, NUMCOLS - 1, PAGE_HEIGHT - 1);
  gotoxy(0, 6);
  cprintf("Hit a key to continue.");
  cgetc();
}
