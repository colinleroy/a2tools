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

#define DITHER_THRESHOLD 92


#ifdef __CC65__
#define TMP_NAME "/RAM/HGR"
#else
#define TMP_NAME "tmp.HGR"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "hgr.h"

#include "qt-conv.h"
#include "path_helper.h"

#ifdef __CC65__
  #pragma static-locals(push, on)
#endif

FILE *ifp, *ofp;
static const char *ifname;
static size_t data_offset;

uint16 height, width;

uint16 cache_offset;
uint16 cache_pages_read;
uint32 last_seek = 0;

void iseek(uint32 off) {
  fseek(ifp, off, SEEK_SET);
  fread(cache, 1, cache_size, ifp);
  cache_offset = 0;
  cache_pages_read = 0;
  last_seek = off;
}

uint32 cache_read_since_inval(void) {
  return last_seek + cache_pages_read + cache_offset;
}

uint8 get1() {
  // return fgetc(ifp);
  if (cache_offset == cache_size) {
    fread(cache, 1, cache_size, ifp);
    cache_offset = 0;
    cache_pages_read += cache_size;
  }
  return cache[cache_offset++];
}

uint16 get2() {
  uint16 v;

  if (cache_offset == cache_size) {
    fread(cache, 1, cache_size, ifp);
    cache_offset = 0;
    cache_pages_read += cache_size;
  }
  ((unsigned char *)&v)[1] = cache[cache_offset++];
  if (cache_offset == cache_size) {
    fread(cache, 1, cache_size, ifp);
    cache_offset = 0;
    cache_pages_read += cache_size;
  }
  ((unsigned char *)&v)[0] = cache[cache_offset++];
  return v;
}

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

uint8 getbitnohuff (uint8 nbits)
{
  uint8 c;

  GETBITS_COMMON();

  vbits -= nbits;

  return c;
}

uint8 getbithuff (uint8 nbits, uint16 *huff)
{
  uint8 c;

  GETBITS_COMMON();

  vbits -= huff[c] >> 8;
  c = (uint8) huff[c];

  return c;
}
#ifdef SURL_TO_LANGCARD
#pragma code-name (push, "LC")
#endif


#if GREYSCALE
static void grey_levels(uint8 h) {
  uint8 y;
  uint16 x;
  for (y = 0; y < h; y+= 2)
    for (x = 0; x < width; x += 2) {
      uint16 sum = RAW(y,x) + RAW(y+1,x) + RAW(y,x+1) + RAW(y+1,x+1);
      RAW(y,x) = RAW(y+1,x) = RAW(y,x+1) = RAW(y+1,x+1) = sum >> 2;
    }
}
#endif

#define HDR_LEN 32
#define WH_OFFSET 544

static uint8 identify(const char *name)
{
  char head[32];

/* INIT */
  height = width = 0;

  fread (head, 1, HDR_LEN, ifp);

  printf("Converting QuickTake ");
  if (!strcmp (head, magic)) {
    printf("%s", model);
  } else {
    printf("??? - Invalid file.\n");
    return -1;
  }

  //fseek(ifp, WH_OFFSET, SEEK_SET);
  iseek(WH_OFFSET);
  height = get2();
  width  = get2();

  printf(" image %s (%dx%d)...\n", name, width, height);

  /* Skip those */
  get2();
  get2();

  if (get2() == 30)
    data_offset = 738;
  else
    data_offset = 736;

  //fseek(ifp, data_offset, SEEK_SET);
  iseek(data_offset);
  return 0;
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

#define FILE_WIDTH 256
#define FILE_HEIGHT HGR_HEIGHT
#define X_OFFSET ((HGR_WIDTH - FILE_WIDTH) / 2)

static uint8 buf[256];
static int8 err[512];
static uint8 hgr[40];
static uint16 histogram[256];
static uint8 opt_histogram[256];

#define NUM_PIXELS 49152U //256*192

static void dither_burkes_hgr(uint16 w, uint16 h) {
  uint16 x;
  uint16 y;
  int16 cur_err;
  uint8 *ptr;
  uint8 pixel;
  uint16 curr_hist = 0;
  int8 *err_line_2 = err + w;
  uint8 *hgr_start_col = hgr + X_OFFSET/7;
  uint16 h_plus1 = h + 1;
  unsigned char dhbmono[] = {0x7e,0x7d,0x7b,0x77,0x6f,0x5f,0x3f};
  unsigned char dhwmono[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40};

  printf("Histogram equalization...\n");
  for (x = 0; x < 256; x++) {
    curr_hist += histogram[x];
    opt_histogram[x] = (uint8)((((uint32)curr_hist * 255)) / NUM_PIXELS);
  }

  printf("Dithering...");
  memset(err, 0, sizeof err);
  if (reusable_buf) {
    memset(reusable_buf, 0, HGR_LEN);
  }

  for(y = 0; y < h; ++y) {
    printf(".");
    fread(buf, 1, w, ifp);
    memset(hgr, 0, 40);
    /* Rollover next error line */
    memcpy(err, err_line_2, w);
    memset(err_line_2, 0, w);

    for(x = 0; x < w; ++x) {
      uint8 buf_plus_err = opt_histogram[buf[x]] + err[x];
      uint16 x_plus1 = x + 1;
      uint16 x_plus2 = x + 2;
      int16 x_minus1 = x - 1;
      int16 x_minus2 = x - 2;
      int16 err8, err4, err2;

      ptr = hgr_start_col + x / 7;
      pixel = x % 7;

      if (DITHER_THRESHOLD > buf_plus_err) {
        cur_err = buf_plus_err;
        ptr[0] &= dhbmono[pixel];
      } else {
        cur_err = buf_plus_err - 255;
        ptr[0] |= dhwmono[pixel];
      }
      err8 = (cur_err * 8) / 32;
      err4 = (cur_err * 4) / 32;
      err2 = (cur_err * 2) / 32;

      if (x_plus1 < w) {
        err[x_plus1]          += err8;
        err_line_2[x_plus1]   += err4;
        if (x_plus2 < w) {
          err[x_plus2]        += err4;
          err_line_2[x_plus2] += err2;
        }
      }
      if (x_minus1 > 0) {
        err_line_2[x_minus1]   += err4;
        if (x_minus2 > 0) {
          err_line_2[x_minus2] += err2;
        } 
      }
      err_line_2[x]            += err8;
    }
    if (reusable_buf) {
      memcpy(reusable_buf + baseaddr[y], hgr, 40);
    } else {
      fseek(ofp, baseaddr[y], SEEK_SET);
      fwrite(hgr, 1, 40, ofp);
    }
  }
  printf("\nSaving...\n");
  if (reusable_buf) {
    fseek(ofp, 0, SEEK_SET);
    fwrite(reusable_buf, 1, HGR_LEN, ofp);
  }
}

static void write_raw(void)
{
  uint16 row, col;
  uint8 scaling_factor = (width == 640 ? 4 : 8);
  uint8 band_height = QT_BAND * scaling_factor / 10;
  uint16 idx_dst, idx_src;
  uint8 *raw_ptr;

  printf("Scaling...");
  /* Scale (nearest neighbor)*/
  for (row = 0; row < band_height; row++) {
    uint16 orig_y = row * 10 / scaling_factor;
    printf(".");
    idx_dst = RAW_IDX(row, 0);
    idx_src = RAW_IDX(orig_y, 0);
    for (col = 0; col < FILE_WIDTH; col++) {
      uint16 orig_x = col * 10 / scaling_factor;
      uint8 val = RAW_DIRECT_IDX(idx_src + orig_x);
      RAW_DIRECT_IDX(idx_dst) = val;
      histogram[val]++;
      idx_dst++;
    }
  }
  printf("\n");

  /* Write */
  raw_ptr = raw_image;
  for (row = 0; row < band_height; row++) {
    fwrite (raw_ptr, 1, FILE_WIDTH, ofp);
    raw_ptr += width;
  }
}

static void reload_menu(void) {
  while (reopen_start_device() != 0) {
    clrscr();
    gotoxy(13, 12);
    printf("Please reinsert the program disk, then press any key.");
    cgetc();
  }
  exec("qtmenu", NULL);
}
int main (int argc, const char **argv)
{
  uint16 h;
  char ofname[64], *cp;

  register_start_device();

#ifdef __CC65__
  videomode(VIDEOMODE_80COL);
  printf("Free memory: %zu/%zuB\n", _heapmaxavail(), _heapmemavail());
#endif

  ifname = argv[1];
  if (argc < 2 || !(ifp = fopen (ifname, "rb"))) {
    printf("Can't open %s\n", ifname);
    goto out;
  }

  if (identify(ifname) != 0) {
    goto out;
  }

  strcpy (ofname, ifname);
  if ((cp = strrchr (ofname, '.'))) *cp = 0;
#if OUTPUT_PPM
  strcat (ofname, ".ppm");
#else
  strcat (ofname, ".hgr");
#endif

  ofp = fopen (TMP_NAME, "wb");

  if (!ofp) {
    perror (ofname);
    goto out;
  }


  memset(raw_image, 0, raw_image_size);

  init_base_addrs();
  for (h = 0; h < height; h += QT_BAND) {
    printf("Loading lines %d-%d", h, h + QT_BAND);
    qt_load_raw(h, QT_BAND);
    write_raw();
  }
  
  fclose(ifp);
  fclose(ofp);
  ifp = fopen(TMP_NAME, "rb");
  ofp = fopen(ofname, "wb");

#if OUTPUT_PPM
  write_ppm(width, height);
#else
  dither_burkes_hgr(FILE_WIDTH, FILE_HEIGHT);
#endif
  fclose(ifp);
  fclose(ofp);
  printf("Done.");

#ifdef __CC65__
  reload_menu();
out:
  cgetc();
  reload_menu();
#else
out:
#endif
  return 0;
}

#ifdef SURL_TO_LANGCARD
#pragma code-name (pop)
#endif

#ifdef __CC65__
  #pragma static-locals(pop)
#endif
