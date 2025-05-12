/*
 * Compress an input file using LZ4, outputting the result to stdout
 * preceded by the *uncompressed* input file size. The 6502 loader
 * will use these two first bytes to know what size to decompress.
 */
/*
 * Copyright (C) 2025 Colin Leroy-Mira <colin@colino.net>
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


#include <lz4hc.h>
#include <stdio.h>
#include <stdlib.h>

static char in_buf[MAX_COMPRESSED_DATA_SIZE*16];
static char out_buf[MAX_COMPRESSED_DATA_SIZE*16];

int main(int argc, char *argv[]) {
  FILE *fp;
  size_t read_bytes, wrote_bytes;
  char header[2];

  if (argc < 2) {
    fprintf(stderr, "Usage: %s [input_file]", argv[0]);
    exit(1);
  }

  fp = fopen(argv[1], "rb");
  if (fp == NULL) {
    fprintf(stderr, "Can not open %s.\n", argv[1]);
    exit(1);
  }

  read_bytes = fread(in_buf, 1, sizeof(in_buf), fp);
  fclose(fp);

  if (read_bytes == sizeof(in_buf)) {
    fprintf(stderr, "Too much data too read (>= %zd bytes)\n", sizeof(in_buf));
    exit(1);
  }
  if (read_bytes == 0) {
    fprintf(stderr, "Nothing to read.\n");
    exit(1);
  }

  wrote_bytes = LZ4_compress_HC(in_buf, out_buf, read_bytes, sizeof(out_buf), 16);
  if (wrote_bytes == 0 || wrote_bytes > MAX_COMPRESSED_DATA_SIZE) {
    fprintf(stderr, "Compressed size %zd bytes, "
            "too big for decompression buffer (%d bytes).\n",
            wrote_bytes, MAX_COMPRESSED_DATA_SIZE);
    exit(1);
  }
  fprintf(stderr, "Compressed %zd bytes to %zd bytes "
          "(%zd%% compression).\n",
          read_bytes, wrote_bytes,
          ((read_bytes - wrote_bytes)*100)/read_bytes);

  /* Write uncompressed size as header in little-endian format
   * to help decompressor
   */
  header[0] = (read_bytes & 0xFF);
  header[1] = (read_bytes & 0xFFFF) >> 8;
  fwrite(header, 1, sizeof(header), stdout);
  fwrite(out_buf, 1, wrote_bytes, stdout);
}
