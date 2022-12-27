#ifndef __CC65__
#include <stdlib.h>
#include <stdio.h>
#include <SDL.h>
#include "tgi_compat.h"

static SDL_Surface *screen = NULL;
#define BPP 8
#define X_RES 280
#define Y_RES 192

int tgi_init(void) {
  if (screen != NULL) {
    return -1;
  }

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("Couldn't initialize SDL: %s\n", SDL_GetError());
    return -1;
  }

  screen = SDL_SetVideoMode(X_RES * 2, Y_RES * 2, BPP, SDL_SWSURFACE);
  if (screen == NULL) {
    printf("Couldn't initialize screen: %s\n", SDL_GetError());
    return -1;
  }
  return 0;
}

int tgi_done() {
  SDL_Quit();
}

static int cur_color = 0;

void tgi_setcolor(int color) {
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
  Uint32 * const target_pixel = (Uint32 *) ((Uint8 *) screen->pixels
                                             + y * screen->pitch
                                             + x * screen->format->BytesPerPixel);
  *target_pixel = cur_color;
}

void tgi_setpixel(int x, int y) {
  int d_x = x * 2;
  int d_y = y * 2;

  SDL_LockSurface(screen);

  sdl_set_pixel(d_x, d_y);
  sdl_set_pixel(d_x + 1, d_y);
  sdl_set_pixel(d_x, d_y + 1);
  sdl_set_pixel(d_x + 1, d_y + 1);
  
  SDL_UnlockSurface(screen);

  SDL_UpdateRect(screen, 0, 0, X_RES * 2, Y_RES * 2);
}

void tgi_line(int start_x, int start_y, int end_x, int end_y) {
  
}

#endif
