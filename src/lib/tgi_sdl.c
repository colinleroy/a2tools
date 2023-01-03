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

#ifndef __CC65__
#include <stdlib.h>
#include <stdio.h>
#include <SDL.h>
#include "tgi_compat.h"
#include "math.h"

static SDL_Surface *screen = NULL;
#define BPP 8
#define X_RES 280
#define Y_RES 192

static int tgi_enabled = 1;

int tgi_init(void) {
  if (getenv("DISABLE_TGI") != NULL) {
    tgi_enabled = 0;
    return 0;
  }
  if (screen != NULL) {
    return -1;
  }

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("Couldn't initialize SDL: %s\n", SDL_GetError());
    return -1;
  }

  screen = SDL_SetVideoMode(X_RES * 2, Y_RES * 2, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
  if (screen == NULL) {
    printf("Couldn't initialize screen: %s\n", SDL_GetError());
    return -1;
  }
  return 0;
}

int tgi_done() {
  if (!tgi_enabled)
    return 0;

  SDL_Quit();

  return 0;
}

static int cur_color = 0;

void tgi_setcolor(int color) {
  if (!tgi_enabled)
    return;

  switch (color) {
    case TGI_COLOR_BLACK:
    case TGI_COLOR_BLACK2:
      cur_color = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
      break;

    case TGI_COLOR_GREEN: //11c407
      cur_color = SDL_MapRGB(screen->format, 0x11, 0xc4, 0x07);
      break;

    case TGI_COLOR_VIOLET: //94035c
      cur_color = SDL_MapRGB(screen->format, 0x94, 0x03, 0x5c);
      break;

    case TGI_COLOR_WHITE:
    case TGI_COLOR_WHITE2:
      cur_color = SDL_MapRGB(screen->format, 0xff, 0xff, 0xff);
      break;

    case TGI_COLOR_ORANGE: //e85900
      cur_color = SDL_MapRGB(screen->format, 0x85, 0x90, 0x00);
      break;

    case TGI_COLOR_BLUE: //2392f7
      cur_color = SDL_MapRGB(screen->format, 0x23, 0x92, 0xf7);
      break;

    case TGI_COLOR_DARKGREEN: //006f0f
      cur_color = SDL_MapRGB(screen->format, 0x00, 0x6f, 0x0f);
      break;

    case TGI_COLOR_GRAY: //7b7b7b
      cur_color = SDL_MapRGB(screen->format, 0x7b, 0x7b, 0x7b);
      break;

    case TGI_COLOR_CYAN: //c412f6
      cur_color = SDL_MapRGB(screen->format, 0xc4, 0x12, 0xf6);
      break;

    case TGI_COLOR_BROWN: //4d4e01
      cur_color = SDL_MapRGB(screen->format, 0x4d, 0x4e, 0x01);
      break;

    case TGI_COLOR_GRAY2: //b8b8b8
      cur_color = SDL_MapRGB(screen->format, 0xb8, 0xb8, 0xb8);
      break;

    case TGI_COLOR_PINK: //f781da
      cur_color = SDL_MapRGB(screen->format, 0xf7, 0x81, 0xda);
      break;

    case TGI_COLOR_YELLOW: //c8cd12
      cur_color = SDL_MapRGB(screen->format, 0xc8, 0xcd, 0x12);
      break;

    case TGI_COLOR_AQUA: //4eed90
      cur_color = SDL_MapRGB(screen->format, 0x4e, 0xed, 0x90);
      break;
  }
}

static void sdl_set_pixel(int x, int y) {
  *((Uint32*)(screen->pixels) + x + y * screen->w) = cur_color;
}

void tgi_setpixel(int x, int y) {
  int d_x = x * 2;
  int d_y = y * 2;

  if (!tgi_enabled)
    return;

  SDL_LockSurface(screen);

  sdl_set_pixel(d_x, d_y);
  sdl_set_pixel(d_x + 1, d_y);
  sdl_set_pixel(d_x, d_y + 1);
  sdl_set_pixel(d_x + 1, d_y + 1);
  
  SDL_UnlockSurface(screen);

  SDL_UpdateRect(screen, 0, 0, X_RES * 2, Y_RES * 2);
}

void tgi_line(int start_x, int start_y, int end_x, int end_y) {
  int width, height, steps;
  float dx, dy;
  float x, y;

  if (!tgi_enabled)
    return;

  width = abs(end_x - start_x);
  height = abs(end_y - start_y);
  steps = max(width, height);
  
  if (steps == 0) {
    tgi_setpixel(start_x, start_y);
    return;
  }

  dx = (float)(end_x - start_x) / (float)steps;
  dy = (float)(end_y - start_y) / (float)steps;
  
  for (x = start_x, y = start_y;
       (int)x != end_x || (int)y != end_y;) {
    if ((int)x != end_x) {
      x += dx;
    }
    if ((int)y != end_y) {
      y += dy;
    }
    tgi_setpixel((int)x, (int)y);
  }
}

void tgi_outtextxy (int x, int y, const char* s) {
  
}

#endif
