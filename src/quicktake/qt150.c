#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "qt-conv.h"

#define radc_token(tree) ((int8) getbithuff(8,huff[tree]))

#define FORYX for (y=1; y < 3; y++) for (x=col+1; x >= col; x--)

#define PREDICTOR (c ? (buf[c][y-1][x] + buf[c][y][x+1]) / 2 \
: (buf[c][y-1][x+1] + 2*buf[c][y-1][x] + buf[c][y][x+1]) / 4)

char magic[5] = QT150_MAGIC;
char *model = "150/200";
uint16 raw_width = 640;
uint16 raw_image_size = (QT_BAND) * 640;
uint8 raw_image[(QT_BAND) * 640];

char *cache[2];
uint16 cache_size = 1024;

void alloc_cache(void) {
  cache[CACHE_A] = malloc(cache_size);
  cache[CACHE_B] = NULL;
  if (cache[CACHE_A] == NULL) {
    printf("Cannot allocate memory");
    exit(1);
  }
}

void qt_load_raw(uint16 top, uint8 h)
{
  static uint16 huff[19][256];
  static int16 c, col, tree, step, i, s, x, half_width;
  static uint8 r, nreps, rep, row, y, mul[3];
  static uint16 buf[3][3][386], t;
  static uint16 *midbuf1, *midbuf2;
  static uint16 val;

  static const int8 src[] = {
    1,1, 2,3, 3,4, 4,2, 5,7, 6,5, 7,6, 7,8,
    1,0, 2,1, 3,3, 4,4, 5,2, 6,7, 7,6, 8,5, 8,8,
    2,1, 2,3, 3,0, 3,2, 3,4, 4,6, 5,5, 6,7, 6,8,
    2,0, 2,1, 2,3, 3,2, 4,4, 5,6, 6,7, 7,5, 7,8,
    2,1, 2,4, 3,0, 3,2, 3,3, 4,7, 5,5, 6,6, 6,8,
    2,3, 3,1, 3,2, 3,4, 3,5, 3,6, 4,7, 5,0, 5,8,
    2,3, 2,6, 3,0, 3,1, 4,4, 4,5, 4,7, 5,2, 5,8,
    2,4, 2,7, 3,3, 3,6, 4,1, 4,2, 4,5, 5,0, 5,8,
    2,6, 3,1, 3,3, 3,5, 3,7, 3,8, 4,0, 5,2, 5,4,
    2,0, 2,1, 3,2, 3,3, 4,4, 4,5, 5,6, 5,7, 4,8,
    1,0, 2,2, 2,-2,
    1,-3, 1,3,
    2,-17, 2,-5, 2,5, 2,17,
    2,-7, 2,2, 2,9, 2,18,
    2,-18, 2,-9, 2,-2, 2,7,
    2,-28, 2,28, 3,-49, 3,-9, 3,9, 4,49, 5,-79, 5,79,
    2,-1, 2,13, 2,26, 3,39, 4,-16, 5,55, 6,-37, 6,76,
    2,-26, 2,-13, 2,1, 3,-39, 4,16, 5,-55, 6,-76, 6,37
  };
  static uint8 last[3] = { 16,16,16 };

  if (top == 0) {
    /* Init */
    for (s = i = 0; i < sizeof src; i += 2) {
      FORC(256 >> src[i]) {
        ((uint16 *)(huff))[s] = (src[i] << 8 | (uint8) src[i+1]);
        s++;
      }
    }

    FORC(256) {
      //huff[18][c] = ((8-s) << 8 | c >> s << s | 1 << (s-1));
      huff[18][c] = (1284 | c);
    }
    getbits(0);

    for (i=0; i < sizeof(buf)/sizeof(uint16); i++)
      ((uint16 *)buf)[i] = 2048;

    half_width = width / 2;
  }

  for (row=0; row < h; row+=4) {
    printf(".");
    mul[0] = getbits(6);
    mul[1] = getbits(6);
    mul[2] = getbits(6);
    FORC3 {
      t = mul[c];
      val = ((0x1000000L/(uint32)last[c] + 0x7ff) >> 12) * t;
      x = ~(-1 << (s-1));
      for (i=0; i < sizeof(buf[0])/sizeof(uint16); i++) {
        uint32 l = (uint32)(((uint16 *)buf[c])[i] * (uint32)val + x);
        ((uint16 *)buf[c])[i] = (l) >> 12;
      }
      last[c] = t;
      midbuf1 = &(buf[c][1][half_width]);
      midbuf2 = &(buf[c][2][half_width]);
      for (r=0; r <= !c; r++) {
        tree = t << 7;
        *midbuf1 = tree;
        *midbuf2 = tree;

        for (tree = 1, col = half_width; col > 0; ) {
          if ((tree = (int16)radc_token(tree))) {
            col -= 2;
            if (tree == 8)
              FORYX {
                buf[c][y][x] = (uint8) radc_token(18) * t;
              }
            else
              FORYX {
                int8 tk = radc_token(tree+10);
                buf[c][y][x] = (int)tk * 16 + PREDICTOR;
              }
          } else
            do {
              nreps = (col > 2) ? radc_token(9) + 1 : 1;
              for (rep=0; rep < 8 && rep < nreps && col > 0; rep++) {
                col -= 2;
                FORYX buf[c][y][x] = PREDICTOR;
                if (rep & 1) {
                  int8 tk = radc_token(10);
                  step = tk << 4;
                  FORYX buf[c][y][x] += step;
                }
              }
            } while (nreps == 9);
        }
        for (y=0; y < 2; y++) {
          for (x=0; x < half_width; x++) {
            val = ((uint32)buf[c][y+1][x]) / t;
            if (val > 255)
              val = 255;
            if (c) {
              RAW(row+y*2+c-1,x*2+2-c) = val;
            }
            else {
              RAW(row+r*2+y,x*2+y) = val;
            }
          }
        }
        memcpy (buf[c][0]+!c, buf[c][2], sizeof buf[c][0]-2*!c);
      }
    }
    for (y = row; y < row + 4; y++) {
      for (x = 0; x < width; x++) {
        if ((x+y) & 1) {
          RAW(y,x) = RAW(y,x-1);
        }
      }
    }
  }
}

#undef FORYX
#undef PREDICTOR
