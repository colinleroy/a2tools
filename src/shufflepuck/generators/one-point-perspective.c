#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <SDL.h>
#include <SDL_image.h>

#include "one-point-perspective.h"

static void sdl_set_pixel32(SDL_Surface *surface, int x, int y, Uint32 p) {
  if (x < surface->w && y < surface->h)
    *((Uint32*)(surface->pixels) + x + y * surface->w) = p;
}

static void sdl_set_pixel(SDL_Surface *surface, int x, int y, Uint8 r, Uint8 g, Uint8 b) {
  sdl_set_pixel32(surface, x, y, SDL_MapRGB(surface->format, r, g, b));
}

void main(int argc, char *argv[]) {
  SDL_Surface *screen, *image = IMG_Load("table.png");
  int w = 280, h = 192;

  screen = SDL_SetVideoMode(w, h, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
  if (screen == NULL) {
    printf("Couldn't initialize screen: %s\n", SDL_GetError());
    return;
  }

  SDL_LockSurface(screen);
  SDL_BlitSurface(image, NULL, screen, NULL);
  
  unsigned char c, c2;
  int x, y;

  for (y = 0; y < h; y+=10) {
    for (x = 0; x < w; x+=10) {
      int gx = (x*x_factor(y))/256 + x_shift(y);
      int gy = y_factor(y);
      sdl_set_pixel(screen, gx, gy, 255, 0, 0);
    }
  }

  SDL_UnlockSurface(screen);
  SDL_UpdateRect(screen, 0, 0, w*2, h*2);
  while(1) {
    SDL_Event e;
    SDL_WaitEvent(&e);
    if (e.type == SDL_MOUSEBUTTONUP) {
      goto out;
    }
  }
  out:
  return;
}
