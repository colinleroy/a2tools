#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <SDL.h>
#include "platform.h"
#include "../lib/extended_conio.h"
#include "../cameras/qt-serial.h"
#include "../decoders/qt-conv.h"

static void sdl_set_pixel32(SDL_Surface *surface, int x, int y, Uint32 p) {
  if (x < surface->w && y < surface->h)
    *((Uint32*)(surface->pixels) + x + y * surface->w) = p;
}

static void sdl_set_pixel(SDL_Surface *surface, int x, int y, Uint8 r, Uint8 g, Uint8 b) {
  sdl_set_pixel32(surface, x, y, SDL_MapRGB(surface->format, r, g, b));
}

#define PIXEL_OUTPUT(x, y, v, h) do {                     \
  sdl_set_pixel(screen, (x)*2, (y)*2, v, v/(1+h), v);     \
  sdl_set_pixel(screen, (x)*2+1, (y)*2, v, v/(1+h), v);   \
  sdl_set_pixel(screen, (x)*2, (y)*2+1, v, v/(1+h), v);   \
  sdl_set_pixel(screen, (x)*2+1, (y)*2+1, v, v/(1+h), v); \
} while (0)

void main(int argc, char *argv[]) {
  FILE *fp = NULL;
  FILE *fp2 = NULL;
  int w, h, qtmodel;
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
  if (!fp) {
    printf("Can't open %s (%s)\n", argv[1], strerror(errno));
    exit(1);
  }
  if (argc < 4) {
    size_t data_len;
    fseek(fp, 0, SEEK_END);
    printf("file size: %zu\n", ftell(fp));
    if (ftell(fp) == 2400) {
      /* Raw thumb */
      w = 80;
      h = 60;
    } else {
      data_len = ftell(fp) - PNM_HEADER_SIZE - 512; // 512=sizeof(histogram)
      if (data_len == 256*192) {
        w = 256;
        h = 192;
      } else if (data_len == 320*240 || data_len == 76288) {
        w = 320;
        h = 240;
      } else if (data_len == 640*480 || data_len == 306688) {
        w = 640;
        h = 480;
      } else if (data_len == 91648) {
        w = 384;
        h = 240;
      } else {
        printf("Can't guess size from %zu.\n", data_len);
        w = 640;
        h = 480;
      }
    }

    if (argc == 3) {
      fp2 = fopen(argv[2], "r");
    }
  } else {
    w = atoi(argv[2]);
    h = atoi(argv[3]);
    qtmodel = atoi(argv[4]);
    printf("qtmodel %d\n", qtmodel);
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

  fseek(fp, PNM_HEADER_SIZE, SEEK_SET);
  if (fp2) {
    fseek(fp2, PNM_HEADER_SIZE, SEEK_SET);
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
          PIXEL_OUTPUT(x, y, c, 0);
        } else {
          off_t offset = y*w + x;
          int color = c2;
          int highlight = blink ? abs(c2-c) : 0;
          PIXEL_OUTPUT(x, y, color, highlight);
        }
      }
    }
  } else {
    char line[80], *cur_in;
    unsigned char out[160], *cur_out;
    int i, a, b, c, d, x, y;

    rewind(fp);
    if (qtmodel == QT_MODEL_200) {
      unsigned int data_offset;
      fseek(fp, 0, SEEK_END);
      data_offset = ftell(fp);
      data_offset -= 160*60;
      printf("data_offset = %04X\n", data_offset);
      fseek(fp, data_offset, SEEK_SET);
    }
    for (y = 0; y < 60; y++) {
      if (qtmodel == QT_MODEL_150) {
        unsigned char pg;
        if (y % 2 == 0) {
          fread(line, 1, 80, fp);

          pg = 0;
          for (i = 0, x = 0; i < 60;) {
            unsigned char c, g;

            g = (line[i] & 0x0F) | (line[i+1] & 0xF0);
            c = (g+pg)/2;
            PIXEL_OUTPUT(x,   y,   ((c+out[x])/2), 0);
            out[x] = c;
            c = pg = g;
            PIXEL_OUTPUT(x+1, y,   ((c+out[x+1])/2), 0);
            out[x + 1] = c;

            g = line[i+2];
            c = (g+pg)/2;
            PIXEL_OUTPUT(x+2, y,   ((c+out[x+2])/2), 0);
            out[x + 2] = c;
            c = pg = g;
            PIXEL_OUTPUT(x+3, y,   ((c+out[x+3])/2), 0);
            out[x + 3] = c;
            i += 3;
            x += 4;
          }
        } else {
          for (x = 0; x < 160; x++) {
            PIXEL_OUTPUT(x, y, out[x], 0);
          }
        }
      } else if (qtmodel == QT_MODEL_100) {
        uint8 off;
        fread(line, 1, 40, fp);

        i = 0;
        do {
          c   = line[i];
          a   = (c & 0xF0);
          b   = (c << 4);
          x = i * 2;
          PIXEL_OUTPUT(x, y, a, 0);
          PIXEL_OUTPUT(x, y+1, a, 0);
          PIXEL_OUTPUT(x+1, y, b, 0);
          PIXEL_OUTPUT(x+1, y+1, b, 0);
        } while (i++ < 40);

      } else if (qtmodel == QT_MODEL_200) {
        int i, j;
        if (y % 2 == 0) {
          fread(line, 1, 160, fp);
          i = 39;
          do {
            x = i<<2;
            line[x+2] = line [x+3] = line[x+1];
            line[x+1] = line[x];
            i--;
          } while (i >= 0);
        }
        for (x = 0; x < 160; x ++) {
          sdl_set_pixel(screen, x, y, line[x], line[x], line[x]);
          sdl_set_pixel(screen, x, y+1, line[x], line[x], line[x]);
        }
        y++;
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
      if (timeout-- == 0 && w != 80) {
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
