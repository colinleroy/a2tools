
#ifdef __CC65__
#include <tgi.h>
#else
#include "tgi_compat.h"
#endif

#include "tgi_fastline.h"

#ifdef __CC65__
#pragma code-name (push, "LOWCODE")
#endif

/* Bresenham line algorithm */
void tgi_fastline(int x1, int y1, int x2, int y2) {
  char d, dx, dy, ai, bi, xi, yi;
  
  if (x1 < x2) {
    xi = 1;
    dx = x2 - x1;
  } else {
    xi = -1;
    dx = x1 - x2;
  }

  if (y1 < y2) {
    yi = 1;
    dy = y2 - y1;
  } else {
    yi = -1;
    dy = y1 - y2;
  }

  tgi_setpixel (x1, y1);

  if (dx > dy) {
    ai = (dy - dx) * 2;
    bi = dy * 2;
    d = bi - dx;

    while (x1 != x2) {
      if (d > 0) {
        y1 += yi;
        d += ai;
      } else {
        d += bi;
      }
      x1 += xi;
      tgi_setpixel (x1, y1);
    }
  } else {
    ai = (dx - dy) * 2;
    bi = dx * 2;
    d = bi - dy;

    while (y1 != y2) {
      if (d > 0) {
        x1 += xi;
        d += ai;
      } else {
        d += bi;
      }
      y1 += yi;
      tgi_setpixel (x1, y1);
    }
  }
}
#ifdef __CC65__
#pragma code-name (pop)
#endif
