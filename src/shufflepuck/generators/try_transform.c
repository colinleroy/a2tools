#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <SDL.h>
#include "../lib/extended_conio.h"

static void sdl_set_pixel32(SDL_Surface *surface, int x, int y, Uint32 p) {
  if (x < surface->w && y < surface->h)
    *((Uint32*)(surface->pixels) + x + y * surface->w) = p;
}

static void sdl_set_pixel(SDL_Surface *surface, int x, int y, Uint8 r, Uint8 g, Uint8 b) {
  sdl_set_pixel32(surface, x, y, SDL_MapRGB(surface->format, r, g, b));
}

static Uint32 sdl_get_pixel32(SDL_Surface *surface, int x, int y) {
  return *((Uint32*)(surface->pixels) + x + y * surface->w);
}

static void sdl_get_pixel(SDL_Surface *surface, int x, int y, Uint8 *r, Uint8 *g, Uint8 *b) {
  Uint32 v = sdl_get_pixel32(surface, x, y);
  SDL_GetRGB(v, surface->format, r, g, b);
}

#define MIDX 138
#define MAXY 191
#define FOVY 78.0
#define LEFT_BORDER 3

int x_divisor_at_y(int y) {
  return FOVY + MAXY - y;
}

float y_divisor_at_y(int y) {
  return (FOVY + MAXY - y)/2.05f;
}

float x_shift_at_y(int y) {
  return (-MIDX*FOVY)/(FOVY + MAXY - y) + MIDX + LEFT_BORDER;
}

int x_mult_table[192];
int x_shift_table[192];
int y_trans_table[192];
void transform(int *x, int *y) {
    int oy = MAXY-*y;
    int nx, ny;
    nx = ((*x)*x_mult_table[*y])/256 + x_shift_table[*y];
    ny = y_trans_table[*y];

    *x = nx;
    *y = ny;
}

void build_x_mult_table(void) {
  int y;
  printf("x_mult: .byte ");
  for (y = 0; y < 192; y++) {
    x_mult_table[y] = (256*FOVY)/x_divisor_at_y(y);
    printf("%d%s", x_mult_table[y], y < 191 ? ", ":"");
  }
  printf("\n");
}

void build_x_shift_table(void) {
  int y;
  printf("x_shift: .byte ");
  for (y = 0; y < 192; y++) {
    x_shift_table[y] = x_shift_at_y(y);
    printf("%d%s", x_shift_table[y], y < 191 ? ", ":"");
  }
  printf("\n");
}

void build_y_table(void) {
  int y;
  printf("y_transform: .byte ");
  for (y = 0; y < 192; y++) {
    y_trans_table[y] = MAXY-(((MAXY-y) * FOVY) / y_divisor_at_y(y));
    printf("%d%s", y_trans_table[y], y < 191 ? ", ":"");
  }
  printf("\n");
}

void project(SDL_Surface *in) {
  int x, y;

  build_x_mult_table();
  build_x_shift_table();
  build_y_table();

  x = 0; y = 0;
  transform(&x, &y);
  printf("=> %d, %d\n\n\n", x, y);

  SDL_LockSurface(in);
  for (x = 0; x < 256; x++) {
    for (y = 0; y < 192; y++) {
      Uint8 r, g, b;
      int nx, ny;
      sdl_get_pixel(in, x, y, &r, &g, &b);
      nx = x;
      ny = y;
      transform(&nx, &ny);

      sdl_set_pixel(in, nx+256, ny, r, g, b);
    }
  }
  SDL_UnlockSurface(in);
  SDL_UpdateRect(in, 0, 0, 512, 192);
}

void main(int argc, char *argv[]) {
  FILE *fp = fopen(argv[1],"r");
  FILE *fp2 = NULL;
  int w, h, qt150;
  SDL_Surface *screen = NULL;

  if (argc < 4) {
    fseek(fp, 0, SEEK_END);
    if (ftell(fp) == 256*192) {
      w = 256;
      h = 192;
    } else if (ftell(fp) == 320*240) {
      w = 320;
      h = 240;
    } else if (ftell(fp) == 640*480) {
      w = 640;
      h = 480;
    } else {
      printf("Can't guess size.\n");
      exit(1);
    }
    rewind(fp);
    if (argc == 3) {
      fp2 = fopen(argv[2], "r");
    }
  } else {
    w = atoi(argv[2]);
    h = atoi(argv[3]);
    qt150 = atoi(argv[4]);
  }

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("Couldn't initialize SDL: %s\n", SDL_GetError());
    return;
  }
  printf("loading image %s (%dx%d)\n", argv[1],w,h);
  screen = SDL_SetVideoMode(w*2, h, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
  if (screen == NULL) {
    printf("Couldn't initialize screen: %s\n", SDL_GetError());
    return;
  }

  SDL_LockSurface(screen);
  unsigned char c, c2;
  int x, y;

  if (w != 80) {
    for (y = 0; y < h; y++) {
      for (x = 0; x < w; x++) {
        fread(&c, 1, 1, fp);
        if (fp2) {
          fread(&c2, 1, 1, fp2);
        } else {
          c2 = c;
        }
        sdl_set_pixel(screen, x, y, c, c, c);
      }
    }
  }
  SDL_UnlockSurface(screen);
  SDL_UpdateRect(screen, 0, 0, w*2, h);

  project(screen);

  while(1) {
    SDL_Event e;
    SDL_WaitEvent(&e);
    if (e.type == SDL_MOUSEMOTION) {
      off_t offset;
      SDL_GetMouseState(&x, &y);
      x -= 256;
      //y /=2;
      offset = (y*w + x);
      fseek(fp, offset, SEEK_SET);
      fread(&c, 1, 1, fp);
      if (fp2) {
        fseek(fp2, offset, SEEK_SET);
        fread(&c2, 1, 1, fp2);
        if (c != c2)
          printf("0x%04lX: Pixel at %d,%d: %u vs %u\n", offset, x, y, c, c2);
        else
          printf("0x%04lX: Pixel at %d,%d: %u\n", offset, x, y, c);
      } else {
        printf("0x%04lX: Pixel at %d,%d: %u\n", offset, x, y, c);
      }
    }
    if (e.type == SDL_MOUSEBUTTONUP) {
      goto out;
    }
  }
  out:
  fclose(fp);
  if (fp2)
    fclose(fp2);
}
