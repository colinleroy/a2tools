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

int max_shift = 7;
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

SDL_Surface *image[7] = { NULL };

void open_animated_sprites(const char *sprite_name) {
  char filename[256];
  int i;

  for (i = 1; i < 7; i++) {
    snprintf(filename, sizeof(filename), "%s%d.png", sprite_name, i);
    image[i] = IMG_Load(filename);
    if (image[i] == NULL) {
      goto bail;
    }
  }
  printf("Sprite is animated.\n");
  return;

  bail:
  printf("Sprite is not animated.\n");
  for (i = 1; i < 7; i++) {
    if (image[i]) {
      SDL_FreeSurface(image[i]);
    }
    image[i] = image[0];
  }
}

int main(int argc, char *argv[]) {
  int x, y, dx, dy, shift;
  enum Pixel pixval;
  char *sprite_name;
  char filename[256];
  FILE *fp;

  if (argc < 4) {
    printf("Usage: %s [input.png] [Max right X coord] [pixel_perfect|unperfect] [segment]\n", argv[0]);
    exit(1);
  }

  image[0] = IMG_Load(argv[1]);
  if (image[0] == NULL) {
    printf("Can not open %s\n", argv[1]);
    exit(1);
  }

  sprite_name = argv[1];
  if (strchr(sprite_name, '.')) {
    *(strrchr(sprite_name, '.')) = '\0';
  }
  /* Open the possible other sprites before chopping
   * the directory off the name.
   */
  open_animated_sprites(sprite_name);

  if (strchr(sprite_name, '/')) {
    sprite_name = strrchr(sprite_name, '/') + 1;
  }

  if (!strcmp(argv[3], "unperfect")) {
    max_shift = 1;
  }

  if (argc > 4) {
    segment = argv[4];
  }

  if (image[0]->w % 7 != 0) {
    printf("Image width %d is not a multiple of 7\n", image[0]->w);
    exit(1);
  }

  /* Create reference sprite */
  Uint8 sprite_data[image[0]->h][(image[0]->w/7) + 1];
  Uint8 mask_data[image[0]->h][(image[0]->w/7) + 1];
  memset(sprite_data, 0, sizeof(sprite_data));
  memset(mask_data, 0, sizeof(mask_data));

  snprintf(filename, sizeof(filename) - 1, "%s.gen.inc", sprite_name);
  fp = fopen(filename, "wb");
  if (fp == NULL) {
    printf("Can not open %s\n", filename);
    exit(1);
  }
  fprintf(fp, "%s_WIDTH  = %d\n", sprite_name, image[0]->w);
  fprintf(fp, "%s_HEIGHT = %d\n", sprite_name, image[0]->h);
  fprintf(fp, "%s_BYTES  = %d\n", sprite_name, image[0]->h * ((image[0]->w/7)+(max_shift/7)));
  fprintf(fp, "%s_BPLINE = %d\n", sprite_name, (image[0]->w/7) + 1 - (max_shift == 1 ? 1 : 0));
  fprintf(fp, "%s_MIN_X  = 1\n", sprite_name);
  fprintf(fp, "%s_MAX_X  = %s-(%s_WIDTH)\n", sprite_name, argv[2], sprite_name);
  fprintf(fp, ".assert %s_MAX_X < 256, error\n", sprite_name);
  fprintf(fp, "%s_MIN_Y  = 0\n", sprite_name);
  fprintf(fp, "%s_MAX_Y  = 192-%s_HEIGHT\n", sprite_name, sprite_name);
  fclose(fp);

  snprintf(filename, sizeof(filename) - 1, "%s.%s.gen.s", sprite_name,
           max_shift == 1 ? "unperfect":"pixel_perfect");
  fp = fopen(filename, "wb");
  if (fp == NULL) {
    printf("Can not open %s\n", filename);
    exit(1);
  }

  fprintf(fp, "         .export _%s\n", sprite_name);
#ifdef ENABLE_QUICK_DRAW
  fprintf(fp, "         .export _quick_draw_%s\n", sprite_name);
#endif
  fprintf(fp,
          "         .import _draw_sprite_fast, fast_sprite_pointer\n"
          "         .import fast_n_bytes_per_line_draw, fast_sprite_x\n"
          "         .importzp n_bytes_draw, sprite_y\n");
  fprintf(fp, "         .include \"%s.gen.inc\"\n", sprite_name);
  fprintf(fp, "\n");
  fprintf(fp, "         .segment \"%s\"\n", segment);

  for (shift = 0; shift < max_shift; shift++) {
    memset(sprite_data, 0, sizeof(sprite_data));
    memset(mask_data, 0, sizeof(mask_data));

    for (y = image[shift]->h - 1, dy = 0; y >= 0; y--, dy++) {
      for (dx = 0; dx < shift; dx++) {
        /* shifted pixels are transparent */
        mask_data[dy][dx/7] |= (1 << (dx % 7));
      }
      for (x = 0; x < image[shift]->w; x++, dx++) {
        pixval = get_pixel(image[shift], x, y);

        if (pixval == WHITE) {
          sprite_data[dy][dx/7] |= (1 << (dx % 7));
        } else if (pixval == CLEAR) {
          mask_data[dy][dx/7] |= (1 << (dx % 7));
        } else {
          /* black pixel */
        }
      }
      /* Add clear mask at the end */
      for (; dx < image[shift]->w + 7; dx++) {
        mask_data[dy][dx/7] |= 1 << (dx % 7);
      }
    }

    fprintf(fp, "%s_x%d:\n", sprite_name, shift);
    for (y = 0; y < image[shift]->h; y++) {
      fprintf(fp, "         .byte ");
      for (x = 0; x < (image[shift]->w)/7; x++) {
        fprintf(fp, "$%02X", sprite_data[y][x]);
        if (x != ((image[shift]->w)/7)-1 || max_shift > 1) {
          fprintf(fp, ", ");
        }
      }
      if (max_shift > 1) {
        fprintf(fp, "$%02X\n", sprite_data[y][x]);
      } else {
        fprintf(fp, "\n");
      }
    }

    fprintf(fp, "%s_mask_x%d:\n", sprite_name, shift);
    for (y = 0; y < image[shift]->h; y++) {
      fprintf(fp, "         .byte ");
      for (x = 0; x < (image[shift]->w)/7; x++) {
        fprintf(fp, "$%02X", mask_data[y][x]);
        if (x != ((image[shift]->w)/7)-1 || max_shift > 1) {
          fprintf(fp, ", ");
        }
      }
      if (max_shift > 1) {
        fprintf(fp, "$%02X\n", mask_data[y][x]);
      } else {
        fprintf(fp, "\n");
      }
    }
  }

  fprintf(fp, "_%s:\n", sprite_name);
  for (shift = 0; shift < max_shift; shift++) {
    fprintf(fp, "         .addr %s_x%d\n", sprite_name, shift < max_shift ? shift : 0);
    fprintf(fp, "         .addr %s_mask_x%d\n", sprite_name, shift < max_shift ? shift : 0);
  }

  #ifdef ENABLE_QUICK_DRAW
  fprintf(fp, "\n         .code\n\n");
  fprintf(fp,
          "_quick_draw_%s:\n"
          "        stx     fast_sprite_x+1\n"
          "        sty     sprite_y\n"
          "\n"
          "        lda     #(%s_BYTES-1)\n"
          "        sta     n_bytes_draw\n"
          "\n"
          "        lda     #(%s_BPLINE-1)\n"
          "        sta     fast_n_bytes_per_line_draw+1\n"
          "\n"
          "        lda     #<%s_x0\n"
          "        sta     fast_sprite_pointer+1\n"
          "        lda     #>%s_x0\n"
          "        sta     fast_sprite_pointer+2\n"
          "\n"
          "        jmp     _draw_sprite_fast\n",
          sprite_name,
          sprite_name,
          sprite_name,
          sprite_name,
          sprite_name);
  #endif

  for (shift = 1; shift < 7; shift++) {
    if (image[shift] != image[0]) {
      SDL_FreeSurface(image[shift]);
    }
  }
  SDL_FreeSurface(image[0]);

  fclose(fp);
}
