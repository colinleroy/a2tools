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
#include <string.h>
#include <SDL_image.h>
#include "png.h"

enum Pixel {
  CLEAR,
  WHITE,
  BLACK
};

char *segment = "RODATA";

static enum Pixel get_pixel(SDL_Surface *surface, int x, int y) {
  int bpp = surface->format->BytesPerPixel;
  /* Here p is the address to the pixel we want to retrieve */
  Uint32 *p;
  Uint32 val;

  if (bpp != 4) {
    printf("Expected 32-bit png\n");
    exit(1);
  }

  if (x >= surface->w || y >= surface->h) {
    printf("Wrong coordinates %d, %d\n", x, y);
    exit(1);
  }

  p = (Uint32 *)((Uint8 *)surface->pixels + y * surface->pitch + x * bpp);

  val = *p;

  if (val == 0x00000000) {
    return CLEAR;
  } else if (val == 0xff000000) {
    return BLACK;
  } else if (val == 0xffffffff) {
    return WHITE;
  }

  printf("Unexpected value 0x%08X at %d, %d\n", val, x, y);
  exit(1);
}

int main(int argc, char *argv[]) {
  SDL_Surface *image;
  int x, y, c;
  enum Pixel pixval;
  char *sprite_name;
  char filename[256];
  FILE *fp;

  if (argc < 3) {
    printf("Usage: %s [input.png] [Max right X coord] [segment]\n", argv[0]);
    exit(1);
  }

  image = IMG_Load(argv[1]);
  if (image == NULL) {
    printf("Can not open %s\n", argv[1]);
    exit(1);
  }

  sprite_name = argv[1];
  if (strchr(sprite_name, '.')) {
    *(strrchr(sprite_name, '.')) = '\0';
  }
  if (strchr(sprite_name, '/')) {
    sprite_name = strrchr(sprite_name, '/') + 1;
  }

  if (argc > 3) {
    segment = argv[3];
  }
  if (image->w % 7 != 0) {
    printf("Image width %d is not a multiple of 7\n", image->w);
    exit(1);
  }

  /* Create reference sprite */
  Uint8 char_data[image->h];
  Uint8 char_mask[image->h];

  snprintf(filename, sizeof(filename) - 1, "%s.gen.inc", sprite_name);
  fp = fopen(filename, "wb");
  if (fp == NULL) {
    printf("Can not open %s\n", filename);
    exit(1);
  }
  fprintf(fp, "%s_WIDTH  = %d\n", sprite_name, image->w);
  fprintf(fp, "%s_HEIGHT = %d\n", sprite_name, image->h);
  fprintf(fp, "%s_BYTES  = %d\n", sprite_name, image->h * ((image->w/7)+1));
  fclose(fp);

  snprintf(filename, sizeof(filename) - 1, "%s.gen.s", sprite_name);
  fp = fopen(filename, "wb");
  if (fp == NULL) {
    printf("Can not open %s\n", filename);
    exit(1);
  }

  fprintf(fp, "         .export _%s\n", sprite_name);
#ifdef ENABLE_MASK
  fprintf(fp, "         .export _%s_mask\n", sprite_name);
#endif
  fprintf(fp, "         .include \"%s.gen.inc\"\n", sprite_name);
  fprintf(fp, "\n");
  fprintf(fp, "         .segment \"%s\"\n", segment);

  for (c = 0; c < image->w / 7; c++) {
    memset(char_data, 0, sizeof(Uint8) * image->h);
    memset(char_mask, 0, sizeof(Uint8) * image->h);

    for (y = image->h - 1; y >= 0; y--) {
      for (x = 7*c; x < 7*(c+1); x++) {
        pixval = get_pixel(image, x, y);

        if (pixval == WHITE) {
          char_data[y] |= (1 << (x % 7));
        } else if (pixval == CLEAR) {
          char_mask[y] |= (1 << (x % 7));
        } else {
          /* black pixel */
        }
      }
    }

    fprintf(fp, "%s_c%d:\n", sprite_name, c);
    for (y = 0; y < image->h; y++) {
      fprintf(fp, "         .byte %d\n", char_data[y]);
    }

#ifdef ENABLE_MASK
    fprintf(fp, "%s_mask_c%d:\n", sprite_name, c);
    for (y = 0; y < image->h; y++) {
      fprintf(fp, "         .byte %d\n", char_mask[y]);
    }
#endif
  }
  fprintf(fp, "_%s: \n", sprite_name);
  for (c = 0; c < image->w / 7; c++) {
    fprintf(fp, "         .addr %s_c%d\n", sprite_name, c);
  }

#ifdef ENABLE_MASK
  fprintf(fp, "_%s_mask: \n", sprite_name);
  for (c = 0; c < image->w / 7; c++) {
    fprintf(fp, "         .addr %s_mask_c%d\n", sprite_name, c);
  }
#endif
  fclose(fp);
}
