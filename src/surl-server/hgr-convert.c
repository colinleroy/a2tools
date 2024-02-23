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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hgr-convert.h"

static char *read_hgr(char *filename, size_t *len) {
  FILE *in = fopen(filename, "rb");
  char *buf;

  *len = 0;
  if (in == NULL) {
    return NULL;
  }

  /* Try to read a bit more to make sure the file
   * is actually an HGR-file size. Sometimes they
   * lack the last 8 bytes (which are invisible on
   * screen) so tolerate 8192 or 8184.
   */
  buf = malloc(10000);
  *len = fread(buf, 1, 10000, in);
  if (*len != 8192 && *len != 8184) {
    free(buf);
    buf = NULL;
    *len = 0;
  }
  if (*len == 8184) 
    *len = 8192;

  fclose(in);
  return buf;
}

static size_t write_file(char *filename, char *data, size_t len) {
  FILE *out = fopen(filename, "wb");
  size_t out_len = 0;

  if (out == NULL) {
    return 0;
  }
  if ((out_len = fwrite(data, 1, len, out)) < len) {
    out_len = 0;
  }

  fclose(out);
  return out_len;
}

int main(int argc, char **argv) {
  int i;
  size_t len;
  size_t out_len;
  char *out_buf;
  char out_file[255];
  char bayer = 0;

  if (argc < 2) {
    printf("Usage: %s [-bayer] [list of files]\n", argv[0]);
    exit(1);
  }

  i = 1;
  if (!strcmp(argv[1], "-bayer")) {
    bayer = 1;
    i++;
  }
  for (; i < argc; i++) {
    printf("Converting %s...\n", argv[i]);

    /* Try to convert an image to HGR. It will fail if
     * it's not an SDLimage-supported format.
     */
    if ((out_buf = (char *)sdl_to_hgr(argv[i], 1, 1, &len, bayer)) != NULL && len > 0) {
      snprintf(out_file, sizeof(out_file), "%s.hgr", argv[i]);
      write_file(out_file, out_buf, len);
      printf("Converted to hgr: %s\n", out_file);
    } else {
      /* If conversion from image to HGR failed, consider it's HGR,
       * and convert that to PNG. It will have funny results if the
       * file is actually something else.
       */
      char *hgr_buf = read_hgr(argv[i], &len);

      if (hgr_buf && len == 8192) {
        out_buf = hgr_to_png(hgr_buf, len, 1, &out_len);
        snprintf(out_file, sizeof(out_file), "%s.png", argv[i]);
        write_file(out_file, out_buf, out_len);
        printf("Converted to png: %s\n", out_file);
      } else {
        printf("%s does not look like an HGR file.\n", argv[i]);
      }
      free(hgr_buf);
    }
  }
}
