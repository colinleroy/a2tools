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

void reload_menu(const char *filename);

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

#ifndef __CC65__
int fullsize_fd = -1;
#endif

static const char *ifname;

#pragma code-name (push, "LC")

#ifndef JPEGCONV
static uint16 __fastcall__ src_file_get_uint16(void) {
  uint16 v;

  ((unsigned char *)&v)[1] = *(cur_cache_ptr++);
  ((unsigned char *)&v)[0] = *(cur_cache_ptr++);
  return v;
}
#endif

#pragma inline-stdfuncs(push, on)
#pragma allow-eager-inline(push, on)
#pragma codesize(push, 200)
#pragma register-vars(push, on)

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
  
  cputsxy(0, 0, "Decompressing image ");
  if (memcmp (cache_start, magic, 4)) {
    cputs("- Invalid file.\r\n");
    return -1;
  }

#ifndef JPEGCONV
  /* For Quicktake 1x0 */
  if (!memcmp(cache_start, QTKT_MAGIC, 3)) {
    cur_cache_ptr = cache_start + WH_OFFSET;
    height = src_file_get_uint16();
    width  = src_file_get_uint16();

    cputs((char *)name);
    cputs("...\r\n");

    /* Skip those */
    src_file_get_uint16();
    src_file_get_uint16();

    if (src_file_get_uint16() == 30)
      cur_cache_ptr = cache_start + (738);
    else
      cur_cache_ptr = cache_start + (736);

    if (!memcmp(cache_start, QTKN_MAGIC, 4)) {
      width = 320;
      height = 240;
    }
  }
#endif
#ifdef JPEGCONV
  if (!memcmp(cache_start, JPEG_EXIF_MAGIC, 4)) {
    /* FIXME QT 200 implied, 640x480 (scaled down) implied, that sucks */
    cputs((char *)name);
    cputs("...\r\n");
    width = QT200_JPEG_WIDTH;
    height = QT200_JPEG_HEIGHT;
    cur_cache_ptr = cache_start;
  }
#endif
  return 0;
}

#ifndef __CC65__
static uint16 histogram[256];
#else
extern uint8 histogram_low[256];
extern uint8 histogram_high[256];
#endif

#ifndef __CC65__
static uint8 *orig_y_table[BAND_HEIGHT];
#else
uint8 orig_y_table_l[BAND_HEIGHT];
uint8 orig_y_table_h[BAND_HEIGHT];
#endif
uint8 orig_x_offset[256];
uint8 special_x_orig_offset[256];
uint8 scaled_band_height;
uint16 output_write_len;
uint8 scaling_factor = 4;
uint16 crop_start_x, crop_start_y, crop_end_x, crop_end_y;
uint16 last_band = 0;
uint8 last_band_crop = 0;
uint16 effective_width;
/* Scales:
 * 640x480 non-cropped        => 256x192, * 4  / 10, bands of 20 end up 8px
 * 640x480 cropped to 512x384 => 256x192, * 5  / 10, bands of 20 end up 10px, crop last band to 4px
 * 640x480 cropped to 320x240 => 256x192, * 8  / 10, bands of 20 end up 16px
 * 640x480 cropped to 256x192 => 256x192, * 10 / 10, bands of 20 end up 20px, crop last band to 12px
 *
 * 320x240 non-cropped        => 256x192, * 8  / 10, bands of 20 end up 16px
 * 320x240 cropped to 256x192 => 256x192, * 10 / 10, bands of 20 end up 20px, crop last band to 12px
 */

#ifdef __CC65__
void __fastcall__ build_scale_table(const char *ofname);
#else
static void __fastcall__ build_scale_table(const char *ofname) {
  uint8 row, col;
  uint16 xoff, prev_xoff;

  if (width == 320) {
    /* Crop boundaries are 640x480 bound, divide them */
    crop_start_x /= 2;
    crop_end_x   /= 2;
    crop_start_y /= 2;
    crop_end_y   /= 2;
  }
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
      if (width == 640) {
        effective_width = 321; /* Prevent re-cropping from menu */
      }
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
    default:
      cputs("Unsupported width.\r\n");
      cgetc();
      reload_menu(ofname);
      break;
  }

  col = 0;
  prev_xoff = 0;
  do {
    /* X cropping is handled in orig_y table */
    xoff = ((col) * 10 / scaling_factor) + RAW_X_OFFSET;
    if ((prev_xoff >> 8) != (xoff >> 8)) {
      orig_x_offset[col] = 0;
      special_x_orig_offset[col] = (uint8)xoff;
    } else {
      orig_x_offset[col] = (uint8)xoff;
    }
    prev_xoff = xoff;
    col++;
  } while (col); /* FILE_WIDTH == 256 */

  row = scaled_band_height;
  while (row--) {
    /* Y cropping is handled in main decode/save loop */
    orig_y_table[row] = (row*10/scaling_factor)*RAW_WIDTH + raw_image + crop_start_x + RAW_Y_OFFSET*RAW_WIDTH;
  }
}
#endif

#pragma code-name (pop)
/* Patched func */

#ifdef __CC65__
void __fastcall__ write_raw(uint16 h);
#else
static void __fastcall__ write_raw(uint16 h)
{
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

  for (y_len = 0; y_len < BAND_HEIGHT; y_len++) {
    write(fullsize_fd, dst_ptr+RAW_X_OFFSET+((y_len+RAW_Y_OFFSET)*RAW_WIDTH), width);
  }
  y_len = 0;
  dst_ptr = raw_image;

  cur_orig_y = orig_y_table;
  do {
    uint8 xoff;
    cur_orig_x = orig_x_offset + 0;
    cur = *cur_orig_y;
    x_len = FILE_WIDTH;
    goto first_col;
    do {
      if (*cur_orig_x == 0) {
        cur += 256;
        xoff = special_x_orig_offset[256-x_len];
      } else {
first_col:
        xoff = *cur_orig_x;
      }
      *dst_ptr = *(cur + xoff);
      histogram[*dst_ptr]++;
      cur_orig_x++;
      dst_ptr ++;
    } while (--x_len);
    cur_orig_y++;
  } while (++y_len < y_end);
  write(ofd, raw_image, output_write_len);
}
#endif

#pragma code-name (push, "LC")

#pragma register-vars(pop)
#pragma codesize(pop)
#pragma allow-eager-inline(pop)
#pragma inline-stdfuncs(pop)

void reload_menu(const char *filename) {
  char buffer[128];
  reopen_start_device();

  if (filename) {
    #ifndef __CC65__
    sprintf(buffer, "%s %d", filename, effective_width);
    #else
    strcpy(buffer, filename);
    strcat(buffer, " ");
    strcat(buffer, utoa(effective_width, buffer+sizeof(buffer)-5, 10));
    #endif
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
  reserve_auxhgr_file();
  try_videomode(VIDEOMODE_80COL);
  cputsxy(0, 23, "Quicktake ");
  cputs(model);
  cputs(" for Apple II decoder - Free memory: ");
  cputs(utoa(_heapmaxavail(), ofname, 10));
  cputs(" bytes\r\n");
#endif

  if (argc < 6) {
    cputs("Missing argument.\r\n");
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
    cputs("Please reinsert the disk containing ");
    cputs((char *)ifname);
    cputs(",\r\n"
          "or press Escape to cancel.\r\n");
    if (cgetc() == CH_ESC)
      goto out;
    else
      goto try_again;
  }

#ifdef __CC65__
  check_floppy();
  gotoxy (0, 5);
#endif

  if (identify(ifname) != 0) {
    goto out;
  }

  cputsxy(0, 7, "Initializing    \r\n");

  strcpy (ofname, ifname);

  unlink(TMP_NAME);
  ofd = open (TMP_NAME, O_RDWR|O_CREAT, 00600);

  #ifndef __CC65__
  fullsize_fd = open ("LARGE_"TMP_NAME, O_RDWR|O_CREAT, 00600);
  #endif

  if (ofd < 0) {
    cputs("Can't open\r\n");
    cputs(TMP_NAME);
    cgetc();
    exit(0);
  }

  bzero(raw_image, RAW_IMAGE_SIZE);
  build_scale_table(ofname);

  progress_bar(0, 8, 80, 0, height);

  for (h = 0; h < crop_end_y; h += BAND_HEIGHT) {
    cputsxy(0, 7, "Decoding    ");
    progress_bar(0, 8, 80, h, crop_end_y);

    qt_load_raw(h);
    if (h >= crop_start_y) {
      cputsxy(0, 7, "Scaling      ");
      write_raw(h);
    }
  }

  cputsxy(0, 7, "Finalizing      ");
  progress_bar(0, 8, 80, height, height);

  close(ifd);
  close(ofd);
  #ifndef __CC65__
  close(fullsize_fd);
  #endif
  ifd = ofd = -1;

  /* Save histogram to /RAM */
  unlink(HIST_NAME);
  ofd = open(HIST_NAME, O_RDWR|O_CREAT, 00600);
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
#endif
  cputsxy(0, 7, "Done.           ");

  reload_menu(ofname);
#ifndef __CC65__
  return 0;
#endif
out:
  cgetc();
  reload_menu(NULL);
  return 0;
}

#pragma code-name (pop)

#ifdef __CC65__
  #pragma static-locals(pop)
#endif
