/*
 * Quicktake for Apple II - scaler
 * Copyright 2023, Colin Leroy-Mira <colin@colino.net>
 *
 * Scaler code - builds scales tables both column and rows at start,
 * picks pixels from 8bpp buffer according to scaling, and writes
 * them out to output file.
 * Handles picture by horizontal bands for memory constraints reasons.
 * Bands need to be a multiple of 4px high for file format reasons
 * on QT 150 and 200 pictures.
 * Bands also needs to a multiple of 5px for scaling reasons.
 * (480 => 192 = *0.4, 240 => 192 = *0.8)
 * 
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

uint16 histogram[256];

uint8 *orig_y_table[BAND_HEIGHT];
uint8 orig_x_offset[256];
uint8 special_x_orig_offset[256];
uint8 scaled_band_height;
uint16 output_write_len;
uint8 scaling_factor = 4;
uint16 crop_start_x, crop_start_y, crop_end_x, crop_end_y;
uint16 last_band = 0;
uint8 last_band_crop = 0;
uint16 effective_width;

extern int ifd;
extern int ofd;
extern int fullsize_fd;
void reload_menu(const char *filename);

/* Scales:
 * 640x480 non-cropped        => 256x192, * 4  / 10, bands of 20 end up 8px
 * 640x480 cropped to 512x384 => 256x192, * 5  / 10, bands of 20 end up 10px, crop last band to 4px
 * 640x480 cropped to 320x240 => 256x192, * 8  / 10, bands of 20 end up 16px
 * 640x480 cropped to 256x192 => 256x192, * 10 / 10, bands of 20 end up 20px, crop last band to 12px
 *
 * 320x240 non-cropped        => 256x192, * 8  / 10, bands of 20 end up 16px
 * 320x240 cropped to 256x192 => 256x192, * 10 / 10, bands of 20 end up 20px, crop last band to 12px
 */

void __fastcall__ build_scale_table(const char *ofname) {
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

void __fastcall__ write_raw(uint16 h)
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
