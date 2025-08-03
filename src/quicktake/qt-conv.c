/*
  QTKT/QTKN decoding wrapper
  Copyright 2023, Colin Leroy-Mira <colin@colino.net>

  Based on dcraw.c -- Dave Coffin's raw photo decoder
  Copyright 1997-2018 by Dave Coffin, dcoffin a cybercom o net

  Main decoding program, link with either qtkt.c or qtkn.c to
  build the decoder.

  Decoding implementations are expected to provide global variables:
  char magic[5];
  char *model;
  uint16 raw_image_size;
  uint8 raw_image[<of raw_image_size>];

  and the decoding function:
  void qt_load_raw(uint16 top)

  This file provides the actual uint16 height and width to the decoder.
  uint16 raw_image_size;
  uint8 raw_image[<of raw_image_size>];
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
#include <unistd.h>
#include <fcntl.h>

#include "hgr.h"
#include "extrazp.h"
#include "clrzone.h"
#include "qt-conv.h"
#include "platform.h"
#include "path_helper.h"
#include "progress_bar.h"
#include "check_floppy.h"
#include "a2_features.h"

#ifdef __CC65__
  #pragma static-locals(push, on)
#endif

static void reload_menu(const char *filename);

/* Shared with decoders */
uint16 height, width;

/* Cache */
uint8 *cache_end;

/* Source file access. The cache mechanism is shared with decoders
 * but the cache size is set by decoders. Decoders should not have
 * access to the file pointers
 */
int ifd = -1;
int ofd = -1;
static const char *ifname;

#pragma code-name (push, "LC")

void __fastcall__ src_file_seek(uint32 off) {
  lseek(ifd, off, SEEK_SET);
  read(ifd, (cur_cache_ptr = cache_start), CACHE_SIZE);
}

static uint16 __fastcall__ src_file_get_uint16(void) {
  uint16 v;

  if (cur_cache_ptr == cache_end) {
    read(ifd, cur_cache_ptr = cache_start, CACHE_SIZE);
  }
  ((unsigned char *)&v)[1] = *(cur_cache_ptr++);
  if (cur_cache_ptr == cache_end) {
    read(ifd, cur_cache_ptr = cache_start, CACHE_SIZE);
  }
  ((unsigned char *)&v)[0] = *(cur_cache_ptr++);
  return v;
}

#pragma inline-stdfuncs(push, on)
#pragma allow-eager-inline(push, on)
#pragma codesize(push, 200)
#pragma register-vars(push, on)

/* bithuff state */
uint8 bitbuf_nohuff=0;
uint32 bitbuf=0;
uint8 vbits=0;

#define HDR_LEN 32
#define WH_OFFSET 544

#ifndef __CC65__
uint8 *cur_cache_ptr;
#endif

static uint8 identify(const char *name)
{
/* INIT */
  height = width = 0;

  read(ifd, cache_start, CACHE_SIZE);

  printf("Decompressing ");
  if (!memcmp (cache_start, magic, 4)) {
    printf("QT%s", model);
  } else {
    printf("- Invalid file.\n");
    return -1;
  }

  /* For Quicktake 1x0 */
  if (!memcmp(cache_start, QTKT_MAGIC, 3)) {
    cur_cache_ptr = cache_start + WH_OFFSET;
    height = src_file_get_uint16();
    width  = src_file_get_uint16();

    printf(" image %s (%dx%d)...\n", name, width, height);

    /* Skip those */
    src_file_get_uint16();
    src_file_get_uint16();

    if (src_file_get_uint16() == 30)
      cur_cache_ptr = cache_start + (738);
    else
      cur_cache_ptr = cache_start + (736);

  } else if (!memcmp(cache_start, JPEG_EXIF_MAGIC, 4)) {
    /* FIXME QT 200 implied, 640x480 (scaled down) implied, that sucks */
    printf(" image %s (640x480)...\n", name);
    width = QT200_JPEG_WIDTH;
    height = QT200_JPEG_HEIGHT;
    cur_cache_ptr = cache_start;
  }
  return 0;
}

#ifndef __CC65__
static uint16 histogram[256];
#else
static uint8 histogram_low[256];
static uint8 histogram_high[256];
#endif

static uint8 *orig_y_table[BAND_HEIGHT];
static uint16 orig_x0_offset;
static uint8 orig_x_offset[256];
static uint8 scaled_band_height;
static uint16 output_write_len;
static uint8 scaling_factor = 4;
static uint16 crop_start_x, crop_start_y, crop_end_x, crop_end_y;
static uint16 last_band = 0;
static uint8 last_band_crop = 0;
static uint16 effective_width;
/* Scales:
 * 640x480 non-cropped        => 256x192, * 4  / 10, bands of 20 end up 8px
 * 640x480 cropped to 512x384 => 256x192, * 5  / 10, bands of 20 end up 10px, crop last band to 4px
 * 640x480 cropped to 320x240 => 256x192, * 8  / 10, bands of 20 end up 16px
 * 640x480 cropped to 256x192 => 256x192, * 10 / 10, bands of 20 end up 20px, crop last band to 12px
 *
 * 320x240 non-cropped        => 256x192, * 8  / 10, bands of 20 end up 16px
 * 320x240 cropped to 256x192 => 256x192, * 10 / 10, bands of 20 end up 20px, crop last band to 12px
 */

static void build_scale_table(const char *ofname) {
  uint8 row, col;
  uint16 xoff, prev_xoff;

  if (width == 640) {
    effective_width = crop_end_x - crop_start_x;
    switch (effective_width) {
      case 640:
        scaling_factor = 4;
        scaled_band_height = (BAND_HEIGHT * 4 / 10);
        output_write_len = FILE_WIDTH * (BAND_HEIGHT * 4 / 10);
        break;
      case 320:
        scaling_factor = 8;
        scaled_band_height = (BAND_HEIGHT * 8 / 10);
        output_write_len = FILE_WIDTH * (BAND_HEIGHT * 8 / 10);
        effective_width = 321; /* Prevent re-cropping from menu */
        break;
      case 512:
        scaling_factor = 5;
        scaled_band_height = (BAND_HEIGHT * 5 / 10);
        output_write_len = FILE_WIDTH * (BAND_HEIGHT * 5 / 10);
        last_band = crop_start_y + 380;
        last_band_crop = 2; /* 4, scaled */
        break;
      case 256:
        scaling_factor = 10;
        scaled_band_height = (BAND_HEIGHT * 10 / 10);
        output_write_len = FILE_WIDTH * (BAND_HEIGHT * 10 / 10);
        last_band = crop_start_y + 180;
        last_band_crop = 12;
        break;
    }
  } else if (width == 320) {
    /* Crop boundaries are 640x480 bound, divide them */
    crop_start_x /= 2;
    crop_end_x   /= 2;
    crop_start_y /= 2;
    crop_end_y   /= 2;
    effective_width = crop_end_x - crop_start_x;
    switch (effective_width) {
      case 320:
        scaling_factor = 8;
        scaled_band_height = (BAND_HEIGHT * 8 / 10);
        output_write_len = FILE_WIDTH * (BAND_HEIGHT * 8 / 10);
        break;
      case 256:
        scaling_factor = 10;
        scaled_band_height = (BAND_HEIGHT * 10 / 10);
        output_write_len = FILE_WIDTH * (BAND_HEIGHT * 10 / 10);
        last_band = crop_start_y + 180;
        last_band_crop = 12;
        break;
      case 128:
        printf("Can not reframe 128x96 zone of 320x240 image.\n"
               "Please try again with less zoom.\n");
        cgetc();
        /* Reset effective width to go back where we were */
        effective_width = 320;
        reload_menu(ofname);
        break;
    }
  }

  col = 0;
  prev_xoff = 0;
  do {
    /* X cropping is handled here in lookup table */
    xoff = ((col) * 10 / scaling_factor) + crop_start_x + RAW_X_OFFSET;
    if (col == 0) {
      orig_x0_offset = xoff;  /* Hack to keep x offsets uint8 */
      orig_x_offset[col] = 0;
    } else {
      orig_x_offset[col] = (uint8)((uint16)xoff - (uint16)prev_xoff);
    }
    prev_xoff = xoff;
    col++;
  } while (col); /* FILE_WIDTH == 256 */

  row = scaled_band_height;
  do {
    /* Y cropping is handled in main decode/save loop */
    row--;
    orig_y_table[row] = raw_image + FILE_IDX((row) * 10 / scaling_factor, 0) + RAW_Y_OFFSET*RAW_WIDTH + orig_x0_offset;
  } while (row);

}

#pragma code-name (pop)
/* Patched func */

static void write_raw(uint16 h)
{
#ifndef __CC65__
  uint8 *dst_ptr;
  uint8 *cur;
  uint8 *cur_orig_x;
  uint8 **cur_orig_y;
  static uint16 x_len;
  static uint8 y_end;
  static uint8 y_len;

  if (last_band_crop && h == last_band) {
    /* Skip end of last band if cropping */
    y_end = last_band_crop;
    output_write_len -= (scaled_band_height - last_band_crop) * FILE_WIDTH;
  } else {
    y_end = scaled_band_height;
  }
  y_len = 0;
  /* Scale (nearest neighbor)*/
  dst_ptr = raw_image;

  cur_orig_y = orig_y_table + 0;
  do {
    cur_orig_x = orig_x_offset + 0;
    cur = *cur_orig_y;
    x_len = FILE_WIDTH;
    do {
      cur += *cur_orig_x;
      *dst_ptr = *(cur);
      histogram[*dst_ptr]++;
      cur_orig_x++;
      dst_ptr ++;
    } while (--x_len);
    cur_orig_y++;
  } while (++y_len < y_end);

#else
  #define y_ptr      zp4
  #define cur_orig_x zp10p
  #define cur_orig_y zp12p

  __asm__("lda %v", last_band_crop);
  __asm__("beq %g", full_band);
  __asm__("ldy %o", h);
  __asm__("lda (c_sp),y");
  __asm__("cmp %v", last_band);
  __asm__("bne %g", full_band);
  __asm__("iny");
  __asm__("lda (c_sp),y");
  __asm__("cmp %v+1", last_band);
  __asm__("bne %g", full_band);

  __asm__("lda %v", last_band_crop);
  __asm__("asl");
  __asm__("sta %g+1", y_end);

  /* output_write_len -= (scaled_band_height - last_band_crop) * FILE_WIDTH; */
  /* FILE_WIDTH = 256 so shift 8 */
  __asm__("lda %v", scaled_band_height);
  __asm__("sec");
  __asm__("sbc %v", last_band_crop);
  __asm__("sta tmp1");
  __asm__("sec");
  __asm__("lda %v+1", output_write_len);
  __asm__("sbc tmp1");
  __asm__("sta %v+1", output_write_len);
  __asm__("jmp %g", no_crop);

full_band:
  __asm__("lda %v", scaled_band_height);
  __asm__("asl");
  __asm__("sta %g+1", y_end);

no_crop:
  __asm__("lda #>%v", raw_image);
  __asm__("sta %g+2", dst_ptr);

  __asm__("clc");
  __asm__("lda #<%v", orig_y_table);
  __asm__("sta %v", cur_orig_y);
  __asm__("lda #>%v", orig_y_table);
  __asm__("sta %v+1", cur_orig_y);

  __asm__("lda #<%v", orig_x_offset);
  __asm__("sta %v", cur_orig_x);
  __asm__("lda #>%v", orig_x_offset);
  __asm__("sta %v+1", cur_orig_x);

  __asm__("ldy #0");
  __asm__("sty %v", y_ptr);
  next_y:
    __asm__("lda (%v),y", cur_orig_y);
    __asm__("sta %g+1", cur_orig_y_addr);
    __asm__("iny");
    __asm__("lda (%v),y", cur_orig_y);
    __asm__("iny");
    __asm__("sty %v", y_ptr);

    __asm__("sta %g+2", cur_orig_y_addr);

    __asm__("ldy #0");

    next_x:
    /* cur += *cur_orig_x; */
    __asm__("lda (%v),y", cur_orig_x);
    __asm__("adc %g+1", cur_orig_y_addr);
    __asm__("sta %g+1", cur_orig_y_addr);
    __asm__("bcc %g", cur_orig_y_addr);
    __asm__("inc %g+2", cur_orig_y_addr);
    __asm__("clc");

    cur_orig_y_addr:
    /* *dst_ptr = *(cur); */
    __asm__("lda $FFFF");   /* Patched */
    dst_ptr:
    __asm__("sta %v,y", raw_image);

    /* histogram[*dst_ptr]++; */
    __asm__("tax");

    __asm__("inc %v,x", histogram_low);
    __asm__("bne %g", noof7);
    __asm__("inc %v,x", histogram_high);
    noof7:
    /* ++cur_orig_x */
    __asm__("iny");
    __asm__("bne %g", next_x);

  /* Increment dst_ptr by FILE_WIDTH (256) */
  __asm__("inc %g+2", dst_ptr);

  /* y_len? */
  __asm__("ldy %v", y_ptr);
  y_end:
  __asm__("cpy #$FF"); /* PATCHED */
  __asm__("bcc %g", next_y);
#endif

  write(ofd, raw_image, output_write_len);
}

#pragma code-name (push, "LC")

#pragma register-vars(pop)
#pragma codesize(pop)
#pragma allow-eager-inline(pop)
#pragma inline-stdfuncs(pop)

static void reload_menu(const char *filename) {
  char buffer[128];
  reopen_start_device();

  if (filename) {
    sprintf(buffer, "%s %d", filename, effective_width);
    exec("slowtake", buffer);
  } else {
    exec("slowtake", NULL);
  }
}

int main (int argc, const char **argv)
{
  uint16 h;
  char ofname[64];

  register_start_device();

#ifdef __CC65__
  try_videomode(VIDEOMODE_80COL);
  printf("Free memory: %zu/%zuB\n", _heapmaxavail(), _heapmemavail());
#endif

  if (argc < 6) {
    printf("Missing argument.\n");
    goto out;
  }

  ifname = argv[1];
  crop_start_x = atoi(argv[2]);
  crop_start_y = atoi(argv[3]);
  crop_end_x   = atoi(argv[4]);
  crop_end_y   = atoi(argv[5]);

  cache_end = cache_start + CACHE_SIZE;

try_again:
  if ((ifd = open (ifname, O_RDONLY)) < 0) {
    printf("Please reinsert the disk containing %s,\n"
           "or press Escape to cancel.\n", ifname);
    if (cgetc() == CH_ESC)
      goto out;
    else
      goto try_again;
  }

#ifdef __CC65__
  check_floppy();
  clrscr();
#endif

  if (identify(ifname) != 0) {
    goto out;
  }

  strcpy (ofname, ifname);

  ofd = open (TMP_NAME, O_WRONLY|O_CREAT);

  if (ofd < 0) {
    printf("Can't open %s\n", TMP_NAME);
    goto out;
  }

  bzero(raw_image, RAW_IMAGE_SIZE);
  build_scale_table(ofname);

  progress_bar(0, 1, 80*22, 0, height);

  for (h = 0; h < crop_end_y; h += BAND_HEIGHT) {
    qt_load_raw(h);
    if (h >= crop_start_y)
      write_raw(h);
  }

  progress_bar(-1, -1, 80*22, height, height);

  close(ifd);
  close(ofd);
  ifd = ofd = -1;

  /* Save histogram to /RAM */
  ofd = open(HIST_NAME, O_WRONLY|O_CREAT);
  if (ofd > 0) {
#ifndef __CC65__
    write(ofd, histogram, sizeof(uint16)*256);
#else
    write(ofd, histogram_low, sizeof(uint8)*256);
    write(ofd, histogram_high, sizeof(uint8)*256);
#endif
    close(ofd);
    ofd = -1;
  }

#ifdef __CC65__
  clrscr();
#endif
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
