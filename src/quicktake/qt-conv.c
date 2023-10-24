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

#define COLORS 3
#define HGR_WIDTH 280
#define HGR_HEIGHT 192
#define HGR_LEN 8192

#define RESIZE    1
#define DITHER    1
#define GREYSCALE 0
#define DITHER_THRESHOLD 100

#define OUTPUT_PPM 0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "qt-conv.h"

#ifdef __CC65__
  #pragma static-locals(push, on)
#endif

FILE *ifp, *ofp;
static const char *ifname;
static size_t data_offset;

uint16 height, width;
extern uint8 raw_image[];


extern char *cache[2];
extern uint16 cache_size;
uint8 cur_cache;
uint16 cur_cache_offset;

void set_cache(uint16 offset, uint8 cache_num) {
  fseek(ifp, offset, SEEK_SET);
  fread(cache[cache_num], cache_size, 1, ifp);
  cur_cache = cache_num;
  cur_cache_offset = 0;
}

uint8 get1() {
  //return fgetc(ifp);
  
  if (cur_cache_offset == cache_size) {
    fread(cache[cur_cache], cache_size, 1, ifp);
    cur_cache_offset = 0;
  }
  return cache[cur_cache][cur_cache_offset++];
}

uint16 get2() {
  uint16 v;
  // fread ((char *)&v, 1, 2, ifp);
  // return ntohs(v);
  if (cur_cache_offset == cache_size) {
    fread(cache[cur_cache], cache_size, 1, ifp);
    cur_cache_offset = 0;
  }
  ((unsigned char *)&v)[1] = cache[cur_cache][cur_cache_offset++];
  
  if (cur_cache_offset == cache_size) {
    fread(cache[cur_cache], cache_size, 1, ifp);
    cur_cache_offset = 0;
  }
  ((unsigned char *)&v)[0] = cache[cur_cache][cur_cache_offset++];
  return v;
}

#ifdef SURL_TO_LANGCARD
#pragma code-name (push, "LC")
#endif

#define GETBITS_COMMON() {                                          \
  uint32 tmp;                                                       \
  uint8 shift;                                                      \
  if (nbits == 0)                                                   \
    return bitbuf = vbits = 0;                                      \
  if (vbits < nbits) {                                              \
    c = get1();                                                     \
    FAST_SHIFT_LEFT_8_LONG(bitbuf);                                 \
    bitbuf += c;                                                    \
    vbits += 8;                                                     \
  }                                                                 \
  shift = 32-vbits;                                                 \
  if (shift >= 24) {                                                \
    FAST_SHIFT_LEFT_24_LONG_TO(bitbuf, tmp);                        \
    shift -= 24;                                                    \
  } else if (shift >= 16) {                                         \
    FAST_SHIFT_LEFT_16_LONG_TO(bitbuf, tmp);                        \
    shift -= 16;                                                    \
  } else if (shift >= 8) {                                          \
    FAST_SHIFT_LEFT_8_LONG_TO(bitbuf, tmp);                         \
    shift -= 8;                                                     \
  }                                                                 \
  if (shift)                                                        \
    tmp <<= shift;                                                  \
                                                                    \
  shift = 32-nbits;                                                 \
  if (shift >= 24) {                                                \
    FAST_SHIFT_RIGHT_24_LONG(tmp);                                  \
    shift -= 24;                                                    \
  } else if (shift >= 16) {                                         \
    FAST_SHIFT_RIGHT_16_LONG(tmp);                                  \
    shift -= 16;                                                    \
  } else if (shift >= 8) {                                          \
    FAST_SHIFT_RIGHT_8_LONG(tmp);                                   \
    shift -= 8;                                                     \
  }                                                                 \
  c = (uint8)(tmp >> shift);                                        \
}

/* bithuff state */
uint32 bitbuf=0;
uint8 vbits=0;
uint8 getbithuff (uint8 nbits, uint16 *huff)
{
  uint8 c;

  GETBITS_COMMON();

  vbits -= huff[c] >> 8;
  c = (uint8) huff[c];

  return c;
}
uint8 getbitnohuff (uint8 nbits)
{
  uint8 c;

  GETBITS_COMMON();

  vbits -= nbits;

  return c;
}

#if GREYSCALE
static void grey_levels(uint8 h) {
  uint8 y;
  uint16 x;
  for (y = 0; y < h; y+= 2)
    for (x = 0; x < width; x += 2) {
      uint8 sum = RAW(y,x) + RAW(y+1,x) + RAW(y,x+1) + RAW(y+1,x+1);
      RAW(y,x) = RAW(y+1,x) = RAW(y,x+1) = RAW(y+1,x+1) = sum;
    }
}
#endif

#define HDR_LEN 32
#define WH_OFFSET 544

static void identify()
{
  char head[32];
  uint16 i;

/* INIT */
  height = width = 0;

  fread (head, 1, HDR_LEN, ifp);

  printf("Doing QuickTake ");
  if (!strcmp (head, magic)) {
    printf("%s", model);
  } else {
    printf("??? - Invalid file.\n");
    cgetc();
    exit(1);
  }

  set_cache(WH_OFFSET, CACHE_A);
  //fseek(ifp, WH_OFFSET, SEEK_SET);

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

  set_cache(data_offset, CACHE_A);
  //fseek(ifp, data_offset, SEEK_SET);
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
    uint8 y_mod8 = y % 8;
    for(x = 0; x < w; ++x) {
      uint16 in = RAW(y, x);

      in += in * map[y_mod8][x % 8] / 63;

      if(in >= DITHER_THRESHOLD)
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

static uint8 *hgr_buf = NULL;

static void write_hgr(uint16 top, uint8 h)
{
  uint8 line[40];
  uint16 row, col;
  uint8 scaled_top;
  uint16 pixel, prev_offset = 0;
  unsigned char *ptr;

  unsigned char dhbmono[] = {0x7e,0x7d,0x7b,0x77,0x6f,0x5f,0x3f};
  unsigned char dhwmono[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40};
  uint8 scaling_factor = (width == 640 ? 4 : 8);
  uint8 band_final_height = h * scaling_factor / 10;

  #define image_final_width 256
  #define x_offset ((HGR_WIDTH - image_final_width) / 2)

#if GREYSCALE
  /* Greyscale */
  printf(" Greyscaling...\n");
  grey_levels(h);
#endif

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
    if (hgr_buf) {
      memcpy(hgr_buf + baseaddr[scaled_top + row], line, 40);
    } else {
      fseek(ofp, baseaddr[scaled_top + row], SEEK_SET);
      fwrite(line, 40, 1, ofp);
    }
  }
}

#if OUTPUT_PPM
static void write_ppm_tiff(int top, int h)
{
  static uint8 *ppm;
  int c, row, col;
  int scaling_factor = (width == 640 ? 4 : 8);
  int band_final_height = h * scaling_factor / 10;
#if RESIZE
  #define FILE_WIDTH HGR_WIDTH
  #define FILE_HEIGHT HGR_HEIGHT
  #define BAND_HEIGHT band_final_height
#else
  #define FILE_WIDTH width
  #define FILE_HEIGHT height
  #define BAND_HEIGHT QT_BAND
#endif

  if (top == 0) {
    /* Header */
    ppm = malloc(width * COLORS);
    fprintf (ofp, "P%d\n%d %d\n%d\n",
        COLORS/2+5, FILE_WIDTH, FILE_HEIGHT, (1 << 8)-1);
  }

#if GREYSCALE
  /* Greyscale */
  grey_levels(QT_BAND);
#endif

#if RESIZE
  /* Scale (nearest neighbor)*/
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
#endif

#if DITHER
  /* Dither (Bayes) */
  dither_bayer(FILE_WIDTH, h);
#endif

  /* Write */
  for (row = 0; row < BAND_HEIGHT; row++) {
    for (col=0; col < FILE_WIDTH; col++) {
      char val = RAW(row,col);
      FORCC ppm [col*COLORS+c] = val;
    }
    fwrite (ppm, COLORS, FILE_WIDTH, ofp);
  }
}
#endif

int main (int argc, const char **argv)
{
  uint16 h;
  char *ofname, *cp;

  ofname = 0;

  alloc_cache();

#ifdef __CC65__
  videomode(VIDEOMODE_80COL);
  printf("Free: %zu/%zuB\n", _heapmaxavail(), _heapmemavail());
#endif

  ifname = argv[1];
  if (!(ifp = fopen (ifname, "rb"))) {
    printf("Can't open %s\n", ifname);
    cgetc();
    exit(1);
  }

  identify();

  ofname = (char *) malloc (strlen(ifname) + 64);
  strcpy (ofname, ifname);
  if ((cp = strrchr (ofname, '.'))) *cp = 0;
#if OUTPUT_PPM
  strcat (ofname, ".ppm");
#else
  strcat (ofname, ".hgr");
#endif
  ofp = fopen (ofname, "wb");

  if (!ofp) {
    perror (ofname);
    cgetc();
    exit(1);
  }


  memset(raw_image, 0, raw_image_size);

#ifdef __CC65__
  if (_heapmaxavail() > HGR_LEN) {
#else
  if (1) {
#endif
    hgr_buf = malloc(HGR_LEN);
  } else {
#if !OUTPUT_PPM
    fwrite(raw_image, HGR_LEN, 1, ofp);
#endif
  }

  init_base_addrs();
  for (h = 0; h < height; h += QT_BAND) {
    printf("Loading %d-%d", h, h + QT_BAND);
    qt_load_raw(h, QT_BAND);
    printf("\nConverting...\n");
#if OUTPUT_PPM
    write_ppm_tiff(h, QT_BAND);
#else
    write_hgr(h, QT_BAND);
#endif
  }
  if (hgr_buf) {
    printf("Saving to disk...\n");
    fwrite(hgr_buf, HGR_LEN, 1, ofp);
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
