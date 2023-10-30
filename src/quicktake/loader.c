#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <SDL.h>
#include "../lib/extended_conio.h"

static void sdl_set_pixel32(SDL_Surface *surface, int x, int y, Uint32 p) {
  if (x < surface->w && y < surface->h)
    *((Uint32*)(surface->pixels) + x + y * surface->w) = p;
}

static void sdl_set_pixel(SDL_Surface *surface, int x, int y, Uint8 r, Uint8 g, Uint8 b) {
  sdl_set_pixel32(surface, x, y, SDL_MapRGB(surface->format, r, g, b));
}

void main(void) {
  FILE *fp = fopen("thumbnail05.qtk","r");
  SDL_Surface *screen = NULL;

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("Couldn't initialize SDL: %s\n", SDL_GetError());
    return;
  }

  screen = SDL_SetVideoMode(80, 60, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
  if (screen == NULL) {
    printf("Couldn't initialize screen: %s\n", SDL_GetError());
    return;
  }
  
  SDL_LockSurface(screen);
  char c;
  int x, y;
  for (y = 0; y < 60; y++) {
    for (x = 0; x < 80; x+=2) {
      int a, b;
      fread(&c, 1, 1, fp);
      printf("0x%02x : %d, %d - ", c, a, b);
      a = (((c>>4) & 0b00001111) << 4) > 120 ? 255:0;
      b = (((c)    & 0b00001111) << 4) > 120 ? 255:0;

      sdl_set_pixel(screen, x, y, a, a, a);
      sdl_set_pixel(screen, x+1, y, b, b, b);
    }
  }
  fclose(fp);
  SDL_UnlockSurface(screen);
  SDL_UpdateRect(screen, 0, 0, 80, 60);
  cgetc();
}
