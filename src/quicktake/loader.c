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

void main(int argc, char *argv[]) {
  FILE *fp = NULL;
  FILE *fp2 = NULL;
  int w, h, qt150;
  SDL_Surface *screen = NULL;
  int video_inited = 0;
  int blink = 1;

display:
  if (fp) {
    fclose(fp);
    fp = NULL;
  }
  if (fp2) {
    fclose(fp2);
  }
  fp = fopen(argv[1],"r");
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
      w = 640;
      h = 480;
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

  if (!video_inited) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
      printf("Couldn't initialize SDL: %s\n", SDL_GetError());
      return;
    }
    printf("loading image %s (%dx%d)\n", argv[1],w,h);
    screen = SDL_SetVideoMode(w*2, h*2, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
    if (screen == NULL) {
      printf("Couldn't initialize screen: %s\n", SDL_GetError());
      return;
    }
    video_inited = 1;
  }

  rewind(fp);
  if (fp2) {
    rewind(fp2);
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
        if (c == c2) {
          sdl_set_pixel(screen, x*2, y*2, c, c, c);
          sdl_set_pixel(screen, x*2+1, y*2, c, c, c);
          sdl_set_pixel(screen, x*2, y*2+1, c, c, c);
          sdl_set_pixel(screen, x*2+1, y*2+1, c, c, c);
        } else {
          off_t offset = y*w + x;
          int color = c2;
          int highlight = blink ? abs(c2-c) : 0;
          printf("%d,%d: %d (%s) vs %d (%s)\n", x, y, c, argv[1], c2, argv[2]);
          sdl_set_pixel(screen, x*2, y*2, color, color/(1+highlight), color);
          sdl_set_pixel(screen, x*2+1, y*2, color, color/(1+highlight), color);
          sdl_set_pixel(screen, x*2, y*2+1, color, color/(1+highlight), color);
          sdl_set_pixel(screen, x*2+1, y*2+1, color, color/(1+highlight), color);
        }
      }
    }
  } else {
    char line[80], *cur_in;
    char out[160], *cur_out;
    int i, a, b, c, x, y;
    for (y = 0; y < 60*2; y+=2) {
      if (qt150) {
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
      } else {
        fread(line, 1, 40, fp);
        cur_in = line;
        cur_out = out;
        for (i = 0; i < 40; i++) {
          c = *cur_in++;
          a   = (((c>>4) & 0b00001111) << 4);
          b   = (((c)    & 0b00001111) << 4);
          printf("%02d %02d ", a, b);
          *cur_out++ = a;
          *cur_out++ = b;
        }
        i = 0;
        x = 0;
        cur_out = out;
        for (i = 0; i < 40; i++) {
          a = *cur_out++;
          b = *cur_out++;
          sdl_set_pixel(screen, x, y, a, a, a);
          sdl_set_pixel(screen, x+1, y, a, a, a);
          sdl_set_pixel(screen, x, y+1, a, a, a);
          sdl_set_pixel(screen, x+1, y+1, a, a, a);
          x+=2;
          sdl_set_pixel(screen, x, y, b, b, b);
          sdl_set_pixel(screen, x+1, y, b, b, b);
          sdl_set_pixel(screen, x, y+1, b, b, b);
          sdl_set_pixel(screen, x+1, y+1, b, b, b);
          x+=2;
        }
        printf("\n");
      }
    }
  }
  SDL_UnlockSurface(screen);
  SDL_UpdateRect(screen, 0, 0, w*2, h*2);
  while(1) {
    SDL_Event e;
    int timeout = 10;
    while (!SDL_PollEvent(&e)) {
      usleep(100*1000);
      if (timeout-- == 0) {
        goto display;
      }
    }
    if (e.type == SDL_KEYUP) {
      if (e.key.keysym.sym == SDLK_ESCAPE) {
        exit(0);
      }
      blink = !blink;
      goto display;
    }
    if (e.type == SDL_MOUSEMOTION) {
      off_t offset;
      SDL_GetMouseState(&x, &y);
      x /=2;
      y /=2;
      offset = (y*w + x);
      fseek(fp, offset, SEEK_SET);
      fread(&c, 1, 1, fp);
      if (fp2) {
        fseek(fp2, offset, SEEK_SET);
        fread(&c2, 1, 1, fp2);
        if (c != c2) {
          printf("0x%04lX: Pixel at %d,%d: %u vs %u |", offset, x, y, c, c2);
          for (int k = abs(c-c2); k; k--) {
            printf("*");
          }
          printf("\n");
        }
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
