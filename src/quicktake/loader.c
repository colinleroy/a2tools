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

void main(int argc, char *argv[]) {
  FILE *fp = fopen(argv[1],"r");
  int w = atoi(argv[2]);
  int h = atoi(argv[3]);
  SDL_Surface *screen = NULL;

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("Couldn't initialize SDL: %s\n", SDL_GetError());
    return;
  }
  printf("loading image %s (%dx%d)\n", argv[1],w,h);
  screen = SDL_SetVideoMode(w, h, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
  if (screen == NULL) {
    printf("Couldn't initialize screen: %s\n", SDL_GetError());
    return;
  }
  
  SDL_LockSurface(screen);
  unsigned char c;
  int x, y;
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      fread(&c, 1, 1, fp);
      //printf("%d,%d = %d\n", x,y,c);
      sdl_set_pixel(screen, x, y, c, c, c);
    }
  }
  fclose(fp);
  SDL_UnlockSurface(screen);
  SDL_UpdateRect(screen, 0, 0, w, h);
  cgetc();
}
