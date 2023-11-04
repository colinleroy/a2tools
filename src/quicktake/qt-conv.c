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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "hgr.h"

#include "qt-conv.h"
#include "path_helper.h"
#include "progress_bar.h"

#ifdef __CC65__
  #pragma static-locals(push, on)
#endif

#pragma code-name (push, "LC")

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

#if SCALE
#define FILE_WIDTH 256
#define FILE_HEIGHT HGR_HEIGHT
#else
#define FILE_WIDTH width
#define FILE_HEIGHT height
#endif

static uint16 histogram[256];

static void write_raw(void)
{
  uint16 row, col;
#if SCALE
  uint8 scaling_factor = (width == 640 ? 4 : 8);
  uint8 band_height = QT_BAND * scaling_factor / 10;
#else
  uint8 scaling_factor = 1;
  uint8 band_height = QT_BAND;
#endif
  uint16 idx_dst, idx_src;
  uint8 *raw_ptr;

#if SCALE
  /* Scale (nearest neighbor)*/
  for (row = 0; row < band_height; row++) {
    uint16 orig_y = row * 10 / scaling_factor;

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
#endif
  /* Write */
  raw_ptr = raw_image;
  for (row = 0; row < band_height; row++) {
    fwrite (raw_ptr, 1, FILE_WIDTH, ofp);
    raw_ptr += raw_width;
  }
}

static void reload_menu(const char *filename) {
  while (reopen_start_device() != 0) {
    clrscr();
    gotoxy(13, 12);
    printf("Please reinsert the program disk, then press any key.");
    cgetc();
  }
  exec("qtmenu", filename);
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

  if (argc < 2) {
    printf("Missing argument.\n");
    goto out;
  }

  ifname = argv[1];
try_again:
  if (!(ifp = fopen (ifname, "rb"))) {
    printf("Please reinsert the disk containing %s,\n"
           "or press Escape to cancel.\n", ifname);
    if (cgetc() == CH_ESC)
      goto out;
    else
      goto try_again;
  }

  if (identify(ifname) != 0) {
    goto out;
  }

  strcpy (ofname, ifname);
  if ((cp = strrchr (ofname, '.'))) *cp = 0;
  strcat (ofname, ".hgr");

  ofp = fopen (TMP_NAME, "wb");

  if (!ofp) {
    printf("Can't open %s\n", ofname);
    goto out;
  }

  memset(raw_image, 0, raw_image_size);

  clrscr();
  printf("Decompressing...\n");
  progress_bar(0, 1, 80*22, 0, height);
  for (h = 0; h < height; h += QT_BAND) {
    qt_load_raw(h, QT_BAND);
    write_raw();
  }
  progress_bar(-1, -1, 80*22, height, height);

  fclose(ifp);
  fclose(ofp);

  /* Save histogram to /RAM */
  ofp = fopen(HIST_NAME,"w");
  if (ofp) {
    fwrite(histogram, sizeof(uint16), 256, ofp);
    fclose(ofp);
  }
  clrscr();
  printf("Done.");

  reload_menu(ofname);
out:
  cgetc();
  reload_menu(NULL);
  return 0;
}

#pragma code-name (pop)

#ifdef __CC65__
  #pragma static-locals(pop)
#endif
