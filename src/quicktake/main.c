/*
   dcraw.c -- Dave Coffin's raw photo decoder
   Copyright 1997-2018 by Dave Coffin, dcoffin a cybercom o net

   This is a command-line ANSI C program to convert raw photos from
   any digital camera on any computer running any operating system.

   No license is required to download and use dcraw.c.  However,
   to lawfully redistribute dcraw, you must either (a) offer, at
   no extra charge, full source code* for all executable files
   containing RESTRICTED functions, (b) distribute this code under
   the GPL Version 2 or later, (c) remove all RESTRICTED functions,
   re-implement them, or copy them from an earlier, unrestricted
   Revision of dcraw.c, or (d) purchase a license from the author.

   The functions that process Foveon images have been RESTRICTED
   since Revision 1.237.  All other code remains free for all uses.

   *If you have not modified dcraw.c in any way, a link to my
   homepage qualifies as "full source code".

   $Revision: 1.478 $
   $Date: 2018/06/01 20:36:25 $
 */

/* RADC (quicktake 150/200): 9mn per 640*480 pic on 16MHz ZipGS Apple IIgs
 *                           90mn on 1MHz Apple IIc
 *
 * Storage: ~64 kB per QT150 pic   ~128 kB per QT100 pic
 *            8 kB per output HGR     8 kB per output HGR
 * Main RAM: Stores code, vars, raw_image array
 *           Approx 1kB free        ??
 *           LC more stuffable, stack can be shortened
 */

/* Handle pic by horizontal bands for memory constraints reasons.
 * Bands need to be a multiple of 4px high for compression reasons
 * on QT 150/200 pictures,
 * and a multiple of 5px for nearest-neighbor scaling reasons.
 * (480 => 192 = *0.4, 240 => 192 = *0.8)
 */
#define QT150_BAND 20

#define COLORS 3
#define HGR_WIDTH 280
#define HGR_HEIGHT 192
#define HGR_LEN 8192

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "extended_conio.h"

#ifdef __CC65__
  #define uint8  unsigned char
  #define uint16 unsigned int
  #define uint32 unsigned long
  #define int8  signed char
  #define int16 signed int
  #define int32 signed long
  #pragma static-locals(push, on)
#else
  #define uint8  unsigned char
  #define uint16 unsigned short
  #define uint32 unsigned int
  #define int8  signed char
  #define int16 signed short
  #define int32 signed int
#endif

static FILE *ifp, *ofp;
static const char *ifname;
static size_t data_offset;

static uint16 height, width;
static uint8 raw_image[QT150_BAND * 640];
static void (*load_raw)(uint16 top, uint8 h);

#define FORC(cnt) for (c=0; c < cnt; c++)
#define FORC3 FORC(3)
#define FORC4 FORC(4)
#define FORCC FORC(COLORS)

#define ABS(x) (x < 0 ? -x : x)
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define LIM(x,min,max) MAX(min,MIN(x,max))
#define SWAP(a,b) { a=a+b; b=a-b; a=a-b; }

/*
   In order to inline this calculation, I make the risky
   assumption that all filter patterns can be described
   by a repeating pattern of eight rows and two columns

   All RGB cameras use one of these Bayer grids:

        0x16161616:        0x61616161:        0x49494949:        0x94949494:

          0 1 2 3 4 5          0 1 2 3 4 5          0 1 2 3 4 5          0 1 2 3 4 5
        0 B G B G B G        0 G R G R G R        0 G B G B G B        0 R G R G R G
        1 G R G R G R        1 B G B G B G        1 R G R G R G        1 G B G B G B
        2 B G B G B G        2 G R G R G R        2 G B G B G B        2 R G R G R G
        3 G R G R G R        3 B G B G B G        3 R G R G R G        3 G B G B G B
 */

#define RAW(row,col) raw_image[(row)*width+(col)]

static void tread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  if( fread (ptr, size, nmemb, stream) != nmemb ) {
    printf("Error reading input\n");
    exit(1);
  }
}

static uint8 get1()
{
  return fgetc(ifp);
}

static uint16 get2()
{
  uint16 v;
  tread (&v, 1, 2, ifp);
  return ntohs(v);
}

static uint8 getbithuff (int16 nbits, uint16 *huff)
{
  static uint32 bitbuf=0;
  static int16 vbits=0;
  uint8 c;

  if (nbits > 25 || nbits == 0 || vbits < 0) return 0;

  if (nbits < 0)
    return bitbuf = vbits = 0;

  while (vbits < nbits) {
    c = get1();
    bitbuf = (bitbuf << 8) + (uint8) c;
    vbits += 8;
  }
  c = (uint8)(((uint32)(bitbuf << (32-vbits))) >> (32-nbits));

  if (huff) {
    vbits -= huff[c] >> 8;
    c = (uint8) huff[c];
  } else
    vbits -= nbits;

  return c;
}

#define getbits(n) getbithuff(n,0)

// static void quicktake_100_load_raw(int top, int h)
// {
//   uint8 pixel[BAND + 4][644];
//   static const short gstep[16] =
//   { -89,-60,-44,-32,-22,-15,-8,-2,2,8,15,22,32,44,60,89 };
//   static const short rstep[6][4] =
//   { {  -3,-1,1,3  }, {  -5,-1,1,5  }, {  -8,-2,2,8  },
//     { -13,-3,3,13 }, { -19,-4,4,19 }, { -28,-6,6,28 } };
//   static const short curve[256] =
//   { 0,1,2,3,4,5,6,7,8,9,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,
//     28,29,30,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,53,
//     54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,74,75,76,77,78,
//     79,80,81,82,83,84,86,88,90,92,94,97,99,101,103,105,107,110,112,114,116,
//     118,120,123,125,127,129,131,134,136,138,140,142,144,147,149,151,153,155,
//     158,160,162,164,166,168,171,173,175,177,179,181,184,186,188,190,192,195,
//     197,199,201,203,205,208,210,212,214,216,218,221,223,226,230,235,239,244,
//     248,252,257,261,265,270,274,278,283,287,291,296,300,305,309,313,318,322,
//     326,331,335,339,344,348,352,357,361,365,370,374,379,383,387,392,396,400,
//     405,409,413,418,422,426,431,435,440,444,448,453,457,461,466,470,474,479,
//     483,487,492,496,500,508,519,531,542,553,564,575,587,598,609,620,631,643,
//     654,665,676,687,698,710,721,732,743,754,766,777,788,799,810,822,833,844,
//     855,866,878,889,900,911,922,933,945,956,967,978,989,1001,1012,1023 };
//   int rb, row, col, sharp, val=0;
// 
//   if (top == 0)
//     getbits(-1);
//   memset (pixel, 0x80, sizeof pixel);
//   for (row=2; row < h+2; row++) {
//     for (col=2+(row & 1); col < width+2; col+=2) {
//       val = ((pixel[row-1][col-1] + 2*pixel[row-1][col+1] +
//                 pixel[row][col-2]) >> 2) + gstep[getbits(4)];
//       pixel[row][col] = val = LIM(val,0,255);
//       if (col < 4)
//         pixel[row][col-2] = pixel[row+1][~row & 1] = val;
//       if (row == 2)
//         pixel[row-1][col+1] = pixel[row-1][col+3] = val;
//     }
//     pixel[row][col] = val;
//   }
//   for (rb=0; rb < 2; rb++)
//     for (row=2+rb; row < h+2; row+=2)
//       for (col=3-(row & 1); col < width+2; col+=2) {
//         if (row < 4 || col < 4) sharp = 2;
//         else {
//           val = ABS(pixel[row-2][col] - pixel[row][col-2])
//               + ABS(pixel[row-2][col] - pixel[row-2][col-2])
//               + ABS(pixel[row][col-2] - pixel[row-2][col-2]);
//           sharp = val <  4 ? 0 : val <  8 ? 1 : val < 16 ? 2 :
//                   val < 32 ? 3 : val < 48 ? 4 : 5;
//         }
//         val = ((pixel[row-2][col] + pixel[row][col-2]) >> 1)
//               + rstep[sharp][getbits(2)];
//         pixel[row][col] = val = LIM(val,0,255);
//         if (row < 4) pixel[row-2][col+2] = val;
//         if (col < 4) pixel[row+2][col-2] = val;
//       }
//   for (row=2; row < h+2; row++)
//     for (col=3-(row & 1); col < width+2; col+=2) {
//       val = ((pixel[row][col-1] + (pixel[row][col] << 2) +
//               pixel[row][col+1]) >> 1) - 0x100;
//       pixel[row][col] = LIM(val,0,255);
//     }
//   for (row=0; row < h; row++)
//     for (col=0; col < width; col++)
//       RAW(row,col) = curve[pixel[row+2][col+2]] / 256;
// }

#define radc_token(tree) ((int8) getbithuff(8,huff[tree]))

#define FORYX for (y=1; y < 3; y++) for (x=col+1; x >= col; x--)

#define PREDICTOR (c ? (buf[c][y-1][x] + buf[c][y][x+1]) / 2 \
: (buf[c][y-1][x+1] + 2*buf[c][y-1][x] + buf[c][y][x+1]) / 4)

static void kodak_radc_load_raw(uint16 top, uint8 h)
{
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
  static uint16 huff[19][256];
  static int16 row, col, tree, nreps, rep, step, i, c, s, r, x, y, val, half_width;
  static uint16 last[3] = { 16,16,16 }, mul[3], buf[3][3][386], t;
  static uint16 *midbuf1, *midbuf2;

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
    getbits(-1);

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
            val = ((uint32)buf[c][y+1][x] >> 4) / t;
            if (val < 0)
              val = 0;
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
          RAW(y,x) = 0;
        }
      }
    }
  }
}

#undef FORYX
#undef PREDICTOR

#ifdef SURL_TO_LANGCARD
#pragma code-name (push, "LC")
#endif

static void grey_levels(uint8 h) {
  uint8 y;
  uint16 x;
  for (y = 0; y < h; y+= 2)
    for (x = 0; x < width; x += 2) {
      uint8 sum = RAW(y,x) + RAW(y+1,x) + RAW(y,x+1) + RAW(y+1,x+1);
      RAW(y,x) = RAW(y+1,x) = RAW(y,x+1) = RAW(y+1,x+1) = sum;
    }
}

#define HDR_LEN 32
#define WH_OFFSET 544

static void identify()
{
  char head[32];
  uint16 i;

/* INIT */
  height = width = 0;

  tread (head, 1, HDR_LEN, ifp);

  printf("Doing QuickTake ");
  if (!strcmp (head, "qktk")) {
    printf("100");
    //load_raw = &quicktake_100_load_raw;
  } else if (!strcmp (head, "qktn")) {
    load_raw = &kodak_radc_load_raw;
  }

  if (head[5]) {
    printf("200");
  } else {
    printf("150");
  }

  /* Skip to 544 */
  for(i = HDR_LEN; i < WH_OFFSET; i++) {
    get1();
  }

  height = get2();
  width  = get2();

  printf(" image (%dx%d)\n", width, height);

  /* Skip those */
  get2();
  get2();

  if (get2() == 30)
    data_offset = 738;
  else
    data_offset = 736;

  /* We just read 10 bytes, now skip to data offset */
  for(i = WH_OFFSET + 10; i < data_offset; i++) {
    get1();
  }
}

static void dither_bayer(uint16 w, uint8 h) {
  uint16 x;
  uint8 y;

  // Ordered dither kernel
  uint8 map[8][8] = {
    { 1, 49, 13, 61, 4, 52, 16, 64 },
    { 33, 17, 45, 29, 36, 20, 48, 32 },
    { 9, 57, 5, 53, 12, 60, 8, 56 },
    { 41, 25, 37, 21, 44, 28, 40, 24 },
    { 3, 51, 15, 63, 2, 50, 14, 62 },
    { 25, 19, 47, 31, 34, 18, 46, 30 },
    { 11, 59, 7, 55, 10, 58, 6, 54 },
    { 43, 27, 39, 23, 42, 26, 38, 22 }
  };

  for(y = 0; y < h; ++y) {
    for(x = 0; x < w; ++x) {
      uint16 in = RAW(y, x);

      in += in * map[y % 8][x % 8] / 63;

      if(in >= 14)
        RAW(y, x) = 255;
      else
        RAW(y, x) = 0;
    }
  }
}

static unsigned baseaddr[192];
static void init_base_addrs (void)
{
  uint16 i, group_of_eight, line_of_eight, group_of_sixtyfour;

  for (i = 0; i < HGR_HEIGHT; ++i)
  {
    line_of_eight = i % 8;
    group_of_eight = (i % 64) / 8;
    group_of_sixtyfour = i / 64;

    baseaddr[i] = line_of_eight * 1024 + group_of_eight * 128 + group_of_sixtyfour * 40;
  }
}

static void write_hgr(uint16 top, uint8 h)
{
  uint8 line[40];
  uint16 row, col;
  uint8 scaled_top;
  uint16 pixel;
  unsigned char *ptr;

  unsigned char dhbmono[] = {0x7e,0x7d,0x7b,0x77,0x6f,0x5f,0x3f};
  unsigned char dhwmono[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40};
  uint8 scaling_factor = (width == 640 ? 4 : 8);
  uint8 band_final_height = h * scaling_factor / 10;

  #define image_final_width 256
  #define x_offset ((HGR_WIDTH - image_final_width) / 2)

  /* Greyscale */
  printf(" Greyscaling...\n");
  grey_levels(h);

  /* Scale (nearest neighbor)*/
  printf(" Scaling...\n");
  for (row = 0; row < band_final_height; row++) {
    uint16 orig_y = row * 10 / scaling_factor;

    for (col = 0; col < image_final_width; col++) {
      uint16 orig_x = col * 10 / scaling_factor;
      RAW(row, col + x_offset) = RAW(orig_y, orig_x);
    }

    /* clear black bands */
    for (col = 0; col < x_offset; col++)
      RAW(row, col) = 0;
    for (col = image_final_width + x_offset + 1; col < HGR_WIDTH; col++)
      RAW(row, col) = 0;
  }

  /* Dither (Bayes) */
  printf(" Dithering...\n");
  dither_bayer(HGR_WIDTH, h);

  /* Write */
  printf(" Saving...\n");
  scaled_top = top * scaling_factor / 10;
  for (row = 0; row < band_final_height; row++) {
    for (col = 0; col < HGR_WIDTH; col++) {
      ptr = line + col / 7;
      pixel = col % 7;
      if (RAW(row,col) != 0) {
        ptr[0] |= dhwmono[pixel];
      } else {
        ptr[0] &= dhbmono[pixel];
      }
    }
    fseek(ofp, baseaddr[scaled_top + row], SEEK_SET);
    fwrite(line, 40, 1, ofp);
  }
}

int main (int argc, const char **argv)
{
  uint16 h;
  char *ofname, *cp;

  ofname = 0;

#ifdef __CC65__
  videomode(VIDEOMODE_80COL);
  printf("Free: %zu/%zuB\n", _heapmaxavail(), _heapmemavail());
#endif

  ifname = "test1.qtk";
  if (!(ifp = fopen (ifname, "rb"))) {
    printf("Can't open input\n");
    cgetc();
    exit(1);
  }

  identify();

  ofname = (char *) malloc (strlen(ifname) + 64);
  strcpy (ofname, ifname);
  if ((cp = strrchr (ofname, '.'))) *cp = 0;
  strcat (ofname, ".hgr");
  ofp = fopen (ofname, "wb");

  if (!ofp) {
    perror (ofname);
    cgetc();
    exit(1);
  }

  memset(raw_image, 0, sizeof(raw_image));
  fwrite(raw_image, HGR_LEN, 1, ofp);

  init_base_addrs();

  for (h = 0; h < height; h += QT150_BAND) {
    printf("Loading %d-%d", h, h + QT150_BAND);
    (*load_raw)(h, QT150_BAND);
    printf("\nConverting...\n");
    write_hgr(h, QT150_BAND);
  }
  printf("Done.\n");

  fclose(ifp);
  fclose(ofp);

  if (ofname) free (ofname);

  return 0;
}
#ifdef SURL_TO_LANGCARD
#pragma code-name (pop)
#endif

#ifdef __CC65__
  #pragma static-locals(pop)
#endif
