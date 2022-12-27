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
      cur_color = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
      break;

    case TGI_COLOR_GREEN:
    case TGI_COLOR_VIOLET:
    case TGI_COLOR_WHITE:
    case TGI_COLOR_WHITE2:
      cur_color = SDL_MapRGB(screen->format, 0xff, 0xff, 0xff);
      break;

    case TGI_COLOR_BLACK2:
    case TGI_COLOR_ORANGE:
    case TGI_COLOR_BLUE:
    case TGI_COLOR_DARKGREEN:
    case TGI_COLOR_GRAY:
    case TGI_COLOR_CYAN:
    case TGI_COLOR_BROWN:
    case TGI_COLOR_GRAY2:
    case TGI_COLOR_PINK:
    case TGI_COLOR_YELLOW:
    case TGI_COLOR_AQUA:
      cur_color = SDL_MapRGB(screen->format, 0x00, 0xff, 0xff);
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
