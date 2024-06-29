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
#include <errno.h>
#include <conio.h>

static char start_dir[FILENAME_MAX*2];

void register_start_device(void) {
#ifdef __APPLE2__
  static char argv0_len;
  char *start_dir_end = start_dir;

  /* If current program pathname is absolute,
   * skip prefix */
  __asm__("lda $281");
  __asm__("cmp #%b", '/');
  __asm__("beq %g", skip_prefix);

  /* Get current PREFIX */
  getcwd(start_dir, FILENAME_MAX);
  start_dir_end = start_dir+strlen(start_dir);
  /* Add a slash at end of directory, to concatenate
   * a possibly-relative filename to it */
  if (*start_dir_end != '/') {
    *start_dir_end = '/';
    start_dir_end++;
  }

skip_prefix:
  /* Get the path name to the program */
  __asm__("lda $280"); /* Get its length */
  __asm__("sta %v", argv0_len);

  /* Copy it */
  memcpy(start_dir_end, (char *)0x281, argv0_len);
  start_dir_end[argv0_len] = '\0';

  /* Cut the filename off */
  *strrchr(start_dir, '/') = '\0';
#else
  getcwd(start_dir, FILENAME_MAX);
#endif
}

int reopen_start_device(void) {
  return chdir(start_dir);
}

#else
void register_start_device(void) {}
int reopen_start_device(void) { return 0; }
#endif
