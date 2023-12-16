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

  if (w != 80) {
    for (y = 0; y < h; y++) {
      for (x = 0; x < w; x++) {
        fread(&c, 1, 1, fp);
    
        sdl_set_pixel(screen, x, y, c, c, c);
      }
    }
  } else {
    // for (y = 0; y < h*2; y++) {
    //   for (x = 0; x < w*2; x+=4) {
    //     int a, b;
    //     fread(&c, 1, 1, fp);
    //     a   = (((c>>4) & 0b00001111) << 4);
    //     b   = (((c)    & 0b00001111) << 4);
    // 
    //     sdl_set_pixel(screen, x, y, a, a, a);
    //     sdl_set_pixel(screen, x+1, y, a, a, a);
    //     sdl_set_pixel(screen, x, y+1, a, a, a);
    //     sdl_set_pixel(screen, x+1, y+1, a, a, a);
    // 
    //     sdl_set_pixel(screen, x+2, y, b, b, b);
    //     sdl_set_pixel(screen, x+3, y, b, b, b);
    //     sdl_set_pixel(screen, x+2, y+1, b, b, b);
    //     sdl_set_pixel(screen, x+3, y+1, b, b, b);
    //   }
    char line[80], *cur_in;
    char out[160], *cur_out;
    int i, a, b, c, x, y;
    for (y = 0; y < 60; y+=2) {
      fread(line, 1, 80, fp);
      cur_in = line;
      cur_out = out;
      for (i = 0; i < 80; i++) {
        c = *cur_in++;
        a   = (((c>>4) & 0b00001111) << 4);
        b   = (((c)    & 0b00001111) << 4);
        *cur_out++ = a;
        *cur_out++ = b;
      }
      i = 0;
      x = 0;
      cur_out = out;
      for (i = 0; i < 80 * 2; ) {
        if (i < 120) {
          a = *cur_out++;
          b = *cur_out++;
          c = *cur_out++;
          sdl_set_pixel(screen, x, (y), a, a, a);
          sdl_set_pixel(screen, x+1, (y), b, b, b);
          sdl_set_pixel(screen, x, (y)+1, c, c, c);
          x+=2;
          i+=3;
        } else {
          a = *cur_out++;
          sdl_set_pixel(screen, 1+((i - 120)*2), (y)+1, a, a, a);
          i++;
        }
      }
    }
  }
  fclose(fp);
  SDL_UnlockSurface(screen);
  SDL_UpdateRect(screen, 0, 0, w, h);
  cgetc();
}
