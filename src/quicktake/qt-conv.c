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
#include <arpa/inet.h>
#include <unistd.h>

#include "hgr.h"
#include "extrazp.h"
#include "clrzone.h"
#include "qt-conv.h"
#include "path_helper.h"
#include "progress_bar.h"

#ifdef __CC65__
  #pragma static-locals(push, on)
#endif

static void reload_menu(const char *filename);

/* Shared with decoders */
uint16 height, width;
uint16 raw_image_size = (BAND_HEIGHT) * 640;
uint8 raw_image[(BAND_HEIGHT) * 640];

/* Cache */
uint8 *cache_end;

/* Source file access. The cache mechanism is shared with decoders
 * but the cache size is set by decoders. Decoders should not have
 * access to the file pointers
 */
FILE *ifp;
static FILE *ofp;
static const char *ifname;

#pragma code-name (push, "LC")

void __fastcall__ src_file_seek(uint32 off) {
  fseek(ifp, off, SEEK_SET);
  fread(cur_cache_ptr = cache_start, 1, CACHE_SIZE, ifp);
}

void __fastcall__ src_file_get_bytes(uint8 *dst, uint16 count) {
  uint16 start, end;

  if (cur_cache_ptr + count < cache_end) {
    memcpy(dst, cur_cache_ptr, count);
    cur_cache_ptr += count;
  } else {
    start = cache_end - cur_cache_ptr;
    memcpy(dst, cur_cache_ptr, start);
    end = count - start;
    fread(cache_start, 1, CACHE_SIZE, ifp);
    memcpy(dst + start, cache_start, end);
    cur_cache_ptr = cache_start + end;
  }
}

static uint16 __fastcall__ src_file_get_uint16(void) {
  uint16 v;

  if (cur_cache_ptr == cache_end) {
    fread(cur_cache_ptr = cache_start, 1, CACHE_SIZE, ifp);
  }
  ((unsigned char *)&v)[1] = *(cur_cache_ptr++);
  if (cur_cache_ptr == cache_end) {
    fread(cur_cache_ptr = cache_start, 1, CACHE_SIZE, ifp);
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
  char head[32];

/* INIT */
  height = width = 0;

  fread (head, 1, HDR_LEN, ifp);

  printf("Converting QuickTake ");
  if (!memcmp (head, magic, 4)) {
    printf("%s", model);
  } else {
    printf("??? - Invalid file.\n");
    return -1;
  }

  if (!memcmp(head, QTKT_MAGIC, 3)) {
    src_file_seek(WH_OFFSET);
    height = src_file_get_uint16();
    width  = src_file_get_uint16();

    printf(" image %s (%dx%d)...\n", name, width, height);

    /* Skip those */
    src_file_get_uint16();
    src_file_get_uint16();

    if (src_file_get_uint16() == 30)
      cur_cache_ptr = cache_start + (738 - WH_OFFSET);
    else
      cur_cache_ptr = cache_start + (736 - WH_OFFSET);

  } else if (!memcmp(head, JPEG_EXIF_MAGIC, 4)) {
    /* FIXME QT 200 implied, 640x480 (scaled down) implied, that sucks */
    printf(" image %s (640x480)...\n", name);
    width = QT200_JPEG_WIDTH;
    height = QT200_JPEG_HEIGHT;
    rewind(ifp);
  }
  return 0;
}

static uint16 histogram[256];

static uint8 *orig_y_table[BAND_HEIGHT];
static uint16 orig_x_table[640];
static uint8 scaled_band_height;
static uint16 output_write_len;
static uint8 scaling_factor = 4;
static uint16 crop_start_x, crop_start_y, crop_end_x, crop_end_y;
static uint16 last_band = 0;
static uint16 last_band_crop = 0;
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

  row = 0;
  do {
    /* Y cropping is handled in main decode/save loop */
    orig_y_table[row] = raw_image + FILE_IDX((row) * 10 / scaling_factor, 0);
    row++;
  } while (row < scaled_band_height);

  col = 0;
  do {
    /* X cropping is handled here in lookup table */
    orig_x_table[col] = (col * 10 / scaling_factor) + crop_start_x;
    col++;
  } while (col); /* FILE_WIDTH == 256 */
}

#pragma code-name (pop) 
/* Patched func */

static void write_raw(uint16 h)
{
#ifdef __CC65__
  #define dst_ptr zp6p
  #define cur_y zp8p
  #define cur_orig_x zp10ip
  #define cur_orig_y zp12ip
  static uint8 x_len;
#else
  uint8 *dst_ptr;
  uint8 *cur_y;
  uint16 *cur_orig_x;
  uint8 **cur_orig_y;
  static uint16 x_len;
#endif
  static uint8 y_len;

  if (h == last_band && last_band_crop) {
    /* Skip end of last band if cropping */
    y_len = last_band_crop;
    output_write_len -= (scaled_band_height - last_band_crop) * FILE_WIDTH;
  } else {
    y_len = scaled_band_height;
  }

  /* Scale (nearest neighbor)*/
  dst_ptr = raw_image;
#ifndef __CC65__
  cur_orig_y = orig_y_table + 0;
  do {
    cur_orig_x = orig_x_table + 0;
    cur_y = (uint8 *)*cur_orig_y;
    x_len = FILE_WIDTH;
    do {
      *dst_ptr = *(cur_y + *cur_orig_x);
      histogram[*dst_ptr]++;
      dst_ptr++;
      cur_orig_x++;
    } while (--x_len);
    cur_orig_y++;
} while (--y_len);
#else
  __asm__("clc");
  __asm__("lda #<(%v)", orig_y_table);
  __asm__("sta %v", cur_orig_y);
  __asm__("lda #>(%v)", orig_y_table);
  __asm__("sta %v+1", cur_orig_y);
  next_y:
    __asm__("lda (%v)", cur_orig_y);    /* patch cur_orig_y in loop */
    __asm__("sta %g+1", cur_orig_y_lb);
    __asm__("ldy #1");
    __asm__("lda (%v),y", cur_orig_y);
    __asm__("sta %g+1", cur_orig_y_hb);

    __asm__("lda #<(%v)", orig_x_table);
    __asm__("sta %v", cur_orig_x);
    __asm__("lda #>(%v)", orig_x_table);
    __asm__("sta %v+1", cur_orig_x);
    __asm__("stz %v", x_len); // 256, but in 8bits
    next_x:
    /* *dst_ptr = *(*cur_orig_y + *cur_orig_x); */
    __asm__("lda (%v)", cur_orig_x);
    cur_orig_y_lb:
    __asm__("adc #$33"); /* Patched (init to 33 to avoid optimizer) */
    __asm__("sta ptr1");
    __asm__("ldy #$01");

    __asm__("lda (%v),y", cur_orig_x);
    cur_orig_y_hb:
    __asm__("adc #$33"); /* Patched (init to 33 to avoid optimizer) */
    __asm__("sta ptr1+1");
    __asm__("lda (ptr1)");
    __asm__("sta (%v)", dst_ptr);

    /* histogram[*dst_ptr]++; */
    __asm__("asl a");
    __asm__("tax");
    __asm__("bcc %g", noof6);
    __asm__("clc");
    /* Second page of histogram */
    __asm__("inc %v+256,x", histogram);
    __asm__("bne %g", inc_dst);
    __asm__("inx");
    __asm__("inc %v+256,x", histogram);
    goto inc_dst;
    noof6:
    /* first page of histogram */
    __asm__("inc %v,x", histogram);
    __asm__("bne %g", inc_dst);
    __asm__("inx");
    __asm__("inc %v,x", histogram);

    inc_dst:
    /* dst_ptr++; */
    __asm__("inc %v", dst_ptr);
    __asm__("bne %g", noof7);
    __asm__("inc %v+1", dst_ptr);

    noof7:
    /* ++cur_orig_x */
    __asm__("lda #$02");
    __asm__("adc %v", cur_orig_x);
    __asm__("sta %v", cur_orig_x);
    __asm__("bcc %g", noof5);
    __asm__("inc %v+1", cur_orig_x);
    __asm__("clc");
    noof5:
    /* x_len? */
    __asm__("dec %v", x_len);
    __asm__("bne %g", next_x);
  /* ++cur_orig_y */
  __asm__("lda #$02");
  __asm__("adc %v", cur_orig_y);
  __asm__("sta %v", cur_orig_y);
  __asm__("bcc %g", noof5b);
  __asm__("inc %v+1", cur_orig_y);
  __asm__("clc");
  noof5b:

  /* y_len? */
  __asm__("dec %v", y_len);
  __asm__("bne %g", next_y);
#endif

  fwrite (raw_image, 1, output_write_len, ofp);
}

#pragma code-name (push, "LC")

#pragma register-vars(pop)
#pragma codesize(pop)
#pragma allow-eager-inline(pop)
#pragma inline-stdfuncs(pop)

static void reload_menu(const char *filename) {
  char buffer[128];
  while (reopen_start_device() != 0) {
    clrscr();
    gotoxy(13, 12);
    printf("Please reinsert the program disk, then press any key.");
    cgetc();
  }

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
  videomode(VIDEOMODE_80COL);
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

  ofp = fopen (TMP_NAME, "wb");

  if (!ofp) {
    printf("Can't open %s\n", TMP_NAME);
    goto out;
  }

  bzero(raw_image, raw_image_size);
  build_scale_table(ofname);

  clrscr();
  printf("Decompressing...\n");
  progress_bar(0, 1, 80*22, 0, height);

  for (h = 0; h < crop_end_y; h += BAND_HEIGHT) {
    qt_load_raw(h);
    if (h >= crop_start_y)
      write_raw(h);
  }

  progress_bar(-1, -1, 80*22, height, height);

  fclose(ifp);
  fclose(ofp);

  /* Save histogram to /RAM */
  ofp = fopen(HIST_NAME, "w");
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
