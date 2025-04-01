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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sound.h"

int sampling_hz = DEFAULT_SAMPLING_HZ;

static char *segment = "DATA";

int main(int argc, char *argv[]) {
  FILE *fp;
  char *filename;
  int c;

  if (argc < 3) {
    fprintf(stderr, "Usage: %s [sampling hz][input.raw]\n", argv[0]);
    exit(1);
  }

  if (argc == 4) {
    segment = argv[3];
  }

  sampling_hz = atoi(argv[1]);

  fp = fopen(argv[2], "rb");
  if (fp == NULL) {
    fprintf(stderr, "Can not open %s\n", argv[1]);
    exit(1);
  }
  filename = argv[2];
  if (strchr(filename, '.')) {
    *strrchr(filename, '.') = 0;
  }
  if (strchr(filename, '/')) {
    filename = strrchr(filename, '/')+1;
  }

  printf("; Settings for this sound sample:\n"
         "; CYCLES_PER_SEC = %d\n"
         "; CARRIER_HZ = %d\n"
         "; SAMPLING_HZ = %d\n"
         "; DUTY_CYCLE_LENGTH = %d\n"
         "; AVAIL_CYCLES = %d\n"
         "; NUM_LEVELS = %d\n"
         "; STEP = %d\n\n",
         CYCLES_PER_SEC,
         CARRIER_HZ,
         sampling_hz,
         DUTY_CYCLE_LENGTH,
         AVAIL_CYCLES,
         NUM_LEVELS,
         STEP);

  printf("         .export _%s_snd, _play_%s\n\n", filename, filename);
  printf("         .import _play_sample\n\n");

  printf("         .segment \"%s\"\n\n", segment);

  printf(".align $100\n"
         "_%s_snd:\n", filename);
  while ((c = fgetc(fp)) != EOF) {
    int r = (c*(NUM_LEVELS-1))/255, byte;
    r = (r/STEP);
    byte = (r*2);
    if (byte > 255) {
      fprintf(stderr, "Range error - too many levels\n");
      exit(1);
    }
    printf("         .byte $%02X    ; %d * 2\n", byte, r); /* *2 to avoid ASLing */
  }
  printf("         .byte (%d*2); play_done\n\n", NUM_LEVELS);

  printf("         .code\n\n"
         "_play_%s:\n"
         "         lda #<_%s_snd\n"
         "         ldx #>_%s_snd\n"
         "         jmp _play_sample\n",
       filename, filename, filename);
  fclose(fp);
}
