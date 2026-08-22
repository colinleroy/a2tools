/* Build with:
 * gcc -I/usr/include/SDL -o qt150-thumb-challenge qt150-thumb-challenge.c  -lSDL_image -lSDL
 *
 * Run with:
 *  ./qt150-thumb-challenge ../tests/thumbs/QT150.thumb
 */

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <SDL.h>

/* Put a pixel on the surface */
static void sdl_set_pixel(SDL_Surface *surface, int x, int y, Uint8 r, Uint8 g, Uint8 b) {
  if (x < surface->w && y < surface->h)
    *((Uint32*)(surface->pixels) + x + y * surface->w) = SDL_MapRGB(surface->format, r, g, b);
}

/* Helper to double resolution */
#define PIXEL_OUTPUT(x, y, r, g, b) do {             \
  sdl_set_pixel(screen, (x)*2, (y)*2,     r, g, b);  \
  sdl_set_pixel(screen, (x)*2+1, (y)*2,   r, g, b);  \
  sdl_set_pixel(screen, (x)*2, (y)*2+1,   r, g, b);  \
  sdl_set_pixel(screen, (x)*2+1, (y)*2+1, r, g, b);  \
} while (0)

/* The renderer. */
void render_thumbnail(FILE *fp, int w, int h, SDL_Surface *screen) {
  unsigned char i, j, x, y;
  char input_bytes[96];
  /* Skip two lines to center */
  fseek(fp, 96*2, SEEK_SET);

  for (y = 0; y < 60; y += 2) {
    fread(input_bytes, 1, 96, fp);   // 96 bytes = 64 pixels × 1.5 bytes

    /* start at 8 to center */
    for (int i = 8, x = 0; x < 80;) {
        unsigned char r0 = input_bytes[i] & 0xF0;
        PIXEL_OUTPUT(x,   y,   r0, r0, r0);
        PIXEL_OUTPUT(x,   y+1, r0, r0, r0);
        PIXEL_OUTPUT(x+1, y,   r0, r0, r0);
        PIXEL_OUTPUT(x+1, y+1, r0, r0, r0);
        // unsigned char g0 = input_bytes[i] << 4;
        // unsigned char b0 = input_bytes[i+1] & 0xF0;

        unsigned char r1 = input_bytes[i+1] << 4;
        // unsigned char g1 = input_bytes[i+2] & 0xF0;
        // unsigned char b1 = input_bytes[i+2] << 4;

        PIXEL_OUTPUT(x+2, y,   r1, r1, r1);
        PIXEL_OUTPUT(x+2, y+1, r1, r1, r1);
        PIXEL_OUTPUT(x+3, y,   r1, r1, r1);
        PIXEL_OUTPUT(x+3, y+1, r1, r1, r1);
        i+=3;
        x+=4;
        printf("i %d x %d\n", i, x);
    }
  }
  printf("read %zu bytes\n", ftell(fp));
}

void main(int argc, char *argv[]) {
  FILE *fp = NULL;
  int w = 80, h = 60;
  SDL_Surface *screen = NULL;

  fp = fopen(argv[1],"r");
  if (!fp) {
    printf("Can't open %s (%s)\n", argv[1], strerror(errno));
    exit(1);
  }

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("Can't initialize SDL: %s\n", SDL_GetError());
    return;
  }
  printf("loading image %s (%dx%d)\n", argv[1],w,h);
  screen = SDL_SetVideoMode(w*2, h*2, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
  if (screen == NULL) {
    printf("Can't initialize screen: %s\n", SDL_GetError());
    return;
  }

  SDL_LockSurface(screen);

  render_thumbnail(fp, w, h, screen);

  SDL_UnlockSurface(screen);
  SDL_UpdateRect(screen, 0, 0, w*2, h*2);

  /* Done - wait for input to quit */
  while(1) {
    SDL_Event e;
    int timeout = 10;
    while (!SDL_PollEvent(&e)) {
      usleep(100*1000);
    }
    if (e.type == SDL_KEYUP) {
      if (e.key.keysym.sym == SDLK_ESCAPE) {
        goto out;
      }
    }
    if (e.type == SDL_MOUSEBUTTONUP) {
      goto out;
    }
  }
  out:
  fclose(fp);
}
