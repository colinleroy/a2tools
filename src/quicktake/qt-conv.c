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
  uint16 cache_size;
  uint8 cache[<of cache_size size>];

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

#include "qt-conv.h"
#include "path_helper.h"
#include "progress_bar.h"

#ifdef __CC65__
  #pragma static-locals(push, on)
#endif

static void reload_menu(const char *filename);

/* Shared with decoders */
uint16 height, width;
uint16 raw_image_size = (QT_BAND) * 640;
uint8 raw_image[(QT_BAND) * 640];


/* Source file access. The cache mechanism is shared with decoders
 * but the cache size is set by decoders. Decoders should not have
 * access to the file pointers
 */
static FILE *ifp, *ofp;
static const char *ifname;
static size_t data_offset;

static uint16 cache_offset;

#ifdef __CC65__
/* Can't use zp* as they're used by codecs,
 * but can use serial-reserved ZP vars as
 * we don't do serial */
#define cur_cache_ptr prev_ram_irq_vector
#else
static uint8 *cur_cache_ptr;
#endif

static uint16 cache_pages_read;
static uint32 last_seek = 0;

void __fastcall__ src_file_seek(uint32 off) {
  fseek(ifp, off, SEEK_SET);
  fread(cache, 1, cache_size, ifp);
  cache_offset = 0;
  cur_cache_ptr = cache;
  cache_pages_read = 0;
  last_seek = off;
}

uint32 __fastcall__ cache_read_since_inval(void) {
  return last_seek + cache_pages_read + cache_offset;
}

void __fastcall__ src_file_get_bytes(uint8 *dst, uint16 count) {
  uint16 start, end;

  if (cache_offset + count < cache_size) {
    memcpy(dst, cur_cache_ptr, count);
    cur_cache_ptr += count;
    cache_offset += count;
  } else {
    start = cache_size - cache_offset;
    memcpy(dst, cur_cache_ptr, count);
    end = count - start;
    fread(cache, 1, cache_size, ifp);
    memcpy(dst + start, cache, end);
    cur_cache_ptr = cache + end;
    cache_offset = end;
    cache_pages_read += cache_size;
  }
}

static uint16 __fastcall__ src_file_get_uint16(void) {
  uint16 v;

  if (cache_offset == cache_size) {
    fread(cache, 1, cache_size, ifp);
    cache_offset = 0;
    cur_cache_ptr = cache;
    cache_pages_read += cache_size;
  }
  cache_offset++;
  ((unsigned char *)&v)[1] = *(cur_cache_ptr++);
  if (cache_offset == cache_size) {
    fread(cache, 1, cache_size, ifp);
    cache_offset = 0;
    cur_cache_ptr = cache;
    cache_pages_read += cache_size;
  }
  cache_offset++;
  ((unsigned char *)&v)[0] = *(cur_cache_ptr++);
  return v;
}

#pragma code-name (push, "LC")

#pragma inline-stdfuncs(push, on)
#pragma allow-eager-inline(push, on)
#pragma codesize(push, 200)
#pragma register-vars(push, on)

static uint32 tmp;
static uint8 shift;

/* bithuff state */
uint32 bitbuf=0;
uint8 vbits=0;

uint8 __fastcall__ getbitnohuff (uint8 n)
{
#ifndef __CC65__
  uint8 c;
  uint8 nbits = n;

  if (nbits == 0) {
    bitbuf = 0;
    vbits = 0;
    return 0;
  }
  if (nbits >= vbits) {
    FAST_SHIFT_LEFT_8_LONG(bitbuf);
    if (cache_offset == cache_size) {
      fread(cache, 1, cache_size, ifp);
      cache_offset = 0;
      cur_cache_ptr = cache;
      cache_pages_read += cache_size;
    }
    cache_offset++;
    bitbuf += *(cur_cache_ptr++);
    vbits += 8;
  }
  shift = 32-vbits;
  if (shift >= 24) {
    FAST_SHIFT_LEFT_24_LONG_TO(bitbuf, tmp);
  } else if (shift >= 16) {
    FAST_SHIFT_LEFT_16_LONG_TO(bitbuf, tmp);
  } else if (shift >= 8) {
    FAST_SHIFT_LEFT_8_LONG_TO(bitbuf, tmp);
  }
  shift %= 8;
  if (shift)
    tmp <<= shift;

  shift = 32-nbits;
  if (shift >= 24) {
    FAST_SHIFT_RIGHT_24_LONG(tmp);
  } else if (shift >= 16) {
    FAST_SHIFT_RIGHT_16_LONG(tmp);
  } else if (shift >= 8) {
    FAST_SHIFT_RIGHT_8_LONG(tmp);
  }
  shift %= 8;
  if (shift)
    c = (uint8)(tmp >> shift);
  else
    c = (uint8)tmp;

  vbits -= nbits;

  return c;
#else
  uint8 nbits = n;

  /* is nbits 0 ? */
  __asm__("bne %g", nbits_pos);
  /* Reset vars */
  __asm__("stz %v", bitbuf);
  __asm__("stz %v+1", bitbuf);
  __asm__("stz %v+2", bitbuf);
  __asm__("stz %v+3", bitbuf);
  __asm__("stz %v", vbits);
  /* Return 0 */
  __asm__("tax");
  __asm__("jmp incsp1");

  nbits_pos:
  __asm__("cmp %v", vbits);
  __asm__("jcc %g", no_read);
  /* shift bitbuf << 8 */
  FAST_SHIFT_LEFT_8_LONG(bitbuf);
  /* Is cache finished? */
  __asm__("lda %v", cache_size);
  __asm__("cmp %v", cache_offset);
  __asm__("bne %g", cache_ok);
  __asm__("ldx %v+1", cache_size);
  __asm__("cpx %v+1", cache_offset);
  __asm__("bne %g", cache_ok);

  /* Read cache */
  fread(cache, 1, cache_size, ifp);
  cache_offset = 0;
  cur_cache_ptr = cache;
  cache_pages_read += cache_size;

  cache_ok:
  /* Inc cache offset */
  __asm__("inc %v", cache_offset);
  __asm__("bne %g", noof1);
  __asm__("inc %v+1", cache_offset);
  noof1:
  /* bitbuf += *(cur_cache_ptr++); */
  __asm__("lda %v", bitbuf);
  __asm__("clc");
  __asm__("adc (%v)", cur_cache_ptr);
  __asm__("sta %v", bitbuf);
  __asm__("bcc %g", noof3);
  __asm__("inc %v+1", bitbuf);
  __asm__("bne %g", noof3);
  __asm__("inc %v+2", bitbuf);
  __asm__("bne %g", noof3);
  __asm__("inc %v+3", bitbuf);
  noof3:
  /* inc pointer */
  __asm__("inc %v", cur_cache_ptr);
  __asm__("bne %g", noof2);
  __asm__("inc %v+1", cur_cache_ptr);

  noof2:
  /* vbits += 8; */
  __asm__("lda #%b", 8);
  __asm__("clc");
  __asm__("adc %v", vbits);
  __asm__("sta %v", vbits);

  no_read:
  /* shift = 32-vbits; */
  __asm__("lda #%b", 32);
  __asm__("sec");
  __asm__("sbc %v", vbits);

  __asm__("cmp #%b", 24);
  __asm__("bcc %g", check_l16);
  FAST_SHIFT_LEFT_24_LONG_TO(bitbuf, tmp);
  goto finish_left_shift;
  check_l16:
  __asm__("cmp #%b", 16);
  __asm__("bcc %g", check_l8);
  FAST_SHIFT_LEFT_16_LONG_TO(bitbuf, tmp);
  goto finish_left_shift;
  check_l8:
  __asm__("cmp #%b", 8);
  __asm__("bcc %g", finish_left_shift);
  FAST_SHIFT_LEFT_8_LONG_TO(bitbuf, tmp);

  finish_left_shift:
  /* % 8 */
  __asm__("and #$07");
  __asm__("sta %v", shift);
  if (shift)
    tmp <<= shift;

  vbits -= nbits;

  /* shift = 32-nbits; */
  __asm__("lda #%b", 32);
  __asm__("sec");
  __asm__("sbc %v", nbits);

  __asm__("cmp #%b", 24);
  __asm__("bcc %g", check_r16);
  FAST_SHIFT_RIGHT_24_LONG_SHORT_ONLY(tmp);
  goto finish_right_shift;
  check_r16:
  __asm__("cmp #%b", 16);
  __asm__("bcc %g", check_r8);
  FAST_SHIFT_RIGHT_16_LONG_SHORT_ONLY(tmp);
  goto finish_right_shift;
  check_r8:
  __asm__("cmp #%b", 8);
  __asm__("bcc %g", finish_right_shift);
  FAST_SHIFT_RIGHT_8_LONG_SHORT_ONLY(tmp);

  finish_right_shift:
  __asm__("ldx #$00");
  /* % 8 */
  __asm__("and #$07");

  /* Should we finish shifting? */
  __asm__("bne %g", finish_and_return);
  /* Else we're done! */
  __asm__("lda %v", tmp);
  __asm__("jmp incsp1");

  finish_and_return:
  /* Finish shift */
  __asm__("tay");
  __asm__("lda %v", tmp);
  cont_shift:
  __asm__("lsr a");
  __asm__("dey");
  __asm__("bpl %g", cont_shift);
  __asm__("rol a");
  __asm__("jmp incsp1");

  /* Unreachable */
  return 0;
#endif
}

uint8 __fastcall__ getbithuff (uint8 n)
{
#ifndef __CC65__
  uint16 h;
  uint8 c;
  uint8 nbits = n;

  if (nbits == 0) {
    bitbuf = 0;
    vbits = 0;
    return 0;
  }
  if (nbits >= vbits) {
    FAST_SHIFT_LEFT_8_LONG(bitbuf);
    if (cache_offset == cache_size) {
      fread(cache, 1, cache_size, ifp);
      cache_offset = 0;
      cur_cache_ptr = cache;
      cache_pages_read += cache_size;
    }
    cache_offset++;
    bitbuf += *(cur_cache_ptr++);
    vbits += 8;
  }
  shift = 32-vbits;
  if (shift >= 24) {
    FAST_SHIFT_LEFT_24_LONG_TO(bitbuf, tmp);
  } else if (shift >= 16) {
    FAST_SHIFT_LEFT_16_LONG_TO(bitbuf, tmp);
  } else if (shift >= 8) {
    FAST_SHIFT_LEFT_8_LONG_TO(bitbuf, tmp);
  }
  shift %= 8;
  if (shift)
    tmp <<= shift;

  shift = 32-nbits;
  if (shift >= 24) {
    FAST_SHIFT_RIGHT_24_LONG(tmp);
  } else if (shift >= 16) {
    FAST_SHIFT_RIGHT_16_LONG(tmp);
  } else if (shift >= 8) {
    FAST_SHIFT_RIGHT_8_LONG(tmp);
  }
  shift %= 8;
  if (shift)
    c = (uint8)(tmp >> shift);
  else
    c = (uint8)tmp;

  h = huff_ptr[c];
  vbits -= h >> 8;
  c = (uint8) h;

  return c;
#else
  uint8 nbits = n;

  /* is nbits 0 ? */
  __asm__("bne %g", hnbits_pos);
  /* Reset vars */
  __asm__("stz %v", bitbuf);
  __asm__("stz %v+1", bitbuf);
  __asm__("stz %v+2", bitbuf);
  __asm__("stz %v+3", bitbuf);
  __asm__("stz %v", vbits);
  /* Return 0 */
  __asm__("tax");
  __asm__("jmp incsp3");

  hnbits_pos:
  __asm__("cmp %v", vbits);
  __asm__("jcc %g", hno_read);
  /* shift bitbuf << 8 */
  FAST_SHIFT_LEFT_8_LONG(bitbuf);
  /* Is cache finished? */
  __asm__("lda %v", cache_size);
  __asm__("cmp %v", cache_offset);
  __asm__("bne %g", hcache_ok);
  __asm__("ldx %v+1", cache_size);
  __asm__("cpx %v+1", cache_offset);
  __asm__("bne %g", hcache_ok);

  /* Read cache */
  fread(cache, 1, cache_size, ifp);
  cache_offset = 0;
  cur_cache_ptr = cache;
  cache_pages_read += cache_size;

  hcache_ok:
  /* Inc cache offset */
  __asm__("inc %v", cache_offset);
  __asm__("bne %g", hnoof1);
  __asm__("inc %v+1", cache_offset);
  hnoof1:
  /* bitbuf += *(cur_cache_ptr++); */
  __asm__("lda %v", bitbuf);
  __asm__("clc");
  __asm__("adc (%v)", cur_cache_ptr);
  __asm__("sta %v", bitbuf);
  __asm__("bcc %g", hnoof3);
  __asm__("inc %v+1", bitbuf);
  __asm__("bne %g", hnoof3);
  __asm__("inc %v+2", bitbuf);
  __asm__("bne %g", hnoof3);
  __asm__("inc %v+3", bitbuf);
  hnoof3:
  /* inc pointer */
  __asm__("inc %v", cur_cache_ptr);
  __asm__("bne %g", hnoof2);
  __asm__("inc %v+1", cur_cache_ptr);

  hnoof2:
  /* vbits += 8; */
  __asm__("lda #%b", 8);
  __asm__("clc");
  __asm__("adc %v", vbits);
  __asm__("sta %v", vbits);

  hno_read:
  /* shift = 32-vbits; */
  __asm__("lda #%b", 32);
  __asm__("sec");
  __asm__("sbc %v", vbits);

  __asm__("cmp #%b", 24);
  __asm__("bcc %g", hcheck_l16);
  FAST_SHIFT_LEFT_24_LONG_TO(bitbuf, tmp);
  goto hfinish_left_shift;
  hcheck_l16:
  __asm__("cmp #%b", 16);
  __asm__("bcc %g", hcheck_l8);
  FAST_SHIFT_LEFT_16_LONG_TO(bitbuf, tmp);
  goto hfinish_left_shift;
  hcheck_l8:
  __asm__("cmp #%b", 8);
  __asm__("bcc %g", hfinish_left_shift);
  FAST_SHIFT_LEFT_8_LONG_TO(bitbuf, tmp);

  hfinish_left_shift:
  /* % 8 */
  __asm__("and #$07");
  __asm__("sta %v", shift);
  if (shift)
    tmp <<= shift;

  /* shift = 32-nbits; */
  __asm__("lda #%b", 32);
  __asm__("sec");
  __asm__("sbc %v", nbits);

  __asm__("cmp #%b", 24);
  __asm__("bcc %g", hcheck_r16);
  FAST_SHIFT_RIGHT_24_LONG_SHORT_ONLY(tmp);
  goto hfinish_right_shift;
  hcheck_r16:
  __asm__("cmp #%b", 16);
  __asm__("bcc %g", hcheck_r8);
  FAST_SHIFT_RIGHT_16_LONG_SHORT_ONLY(tmp);
  goto hfinish_right_shift;
  hcheck_r8:
  __asm__("cmp #%b", 8);
  __asm__("bcc %g", hfinish_right_shift);
  FAST_SHIFT_RIGHT_8_LONG_SHORT_ONLY(tmp);

  hfinish_right_shift:
  __asm__("ldx #$00");
  /* % 8 */
  __asm__("and #$07");

  /* Should we finish shifting? */
  __asm__("beq %g", hfinish_and_return);

  /* Finish shift */
  __asm__("tay");
  __asm__("lda %v", tmp);
  hcont_shift:
  __asm__("lsr a");
  __asm__("dey");
  __asm__("bpl %g", hcont_shift);
  __asm__("rol a");
  goto do_huff;

  hfinish_and_return:
  __asm__("lda %v", tmp);

  do_huff:
  /* h = huff[c]; */
  __asm__("asl a"); /* shift index because uint16 array */
  __asm__("bcc %g", hnoof4);
  __asm__("inx");
  __asm__("clc");
  hnoof4:
  __asm__("adc %v", huff_ptr);
  __asm__("sta ptr1");
  __asm__("txa");
  __asm__("adc %v+1", huff_ptr);
  __asm__("sta ptr1+1");
  __asm__("ldy #$01");
  __asm__("lda (ptr1),y");
  /* h>>8: We don't care about low byte yet */
  __asm__("eor #$FF");
  __asm__("sec");
  __asm__("adc %v", vbits);
  __asm__("sta %v", vbits);
  /* Return h low byte */
  __asm__("ldx #$00");
  __asm__("lda (ptr1)");
  __asm__("jmp incsp1");
  /* unreachable */
  return 0;
#endif

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
      data_offset = 738;
    else
      data_offset = 736;

    src_file_seek(data_offset);
  } else if (!memcmp(head, JPEG_EXIF_MAGIC, 4)) {
    /* FIXME QT 200 implied, 640x480 (scaled down) implied, that sucks */
    width = QT200_JPEG_WIDTH;
    height = QT200_JPEG_HEIGHT;
    /* Init cache */
    src_file_seek(0);
  }
  return 0;
}

static uint16 histogram[256];

static uint8 *orig_y_table[QT_BAND];
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
  uint16 row, col;

  if (width == 640) {
    effective_width = crop_end_x - crop_start_x;
    switch (effective_width) {
      case 640:
        scaling_factor = 4;
        break;
      case 320:
        scaling_factor = 8;
        effective_width = 321; /* Prevent re-cropping from menu */
        break;
      case 512:
        scaling_factor = 5;
        last_band = crop_start_y + 380;
        last_band_crop = 2; /* 4, scaled */
        break;
      case 256:
        scaling_factor = 10;
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
        break;
      case 256:
        scaling_factor = 10;
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
  scaled_band_height = QT_BAND * scaling_factor / 10;
  output_write_len = FILE_WIDTH * scaled_band_height;

  for (row = 0; row < scaled_band_height; row++) {
    /* Y cropping is handled in main decode/save loop */
    orig_y_table[row] = raw_image + FILE_IDX((row) * 10 / scaling_factor, 0);

    for (col = 0; col < FILE_WIDTH; col++) {
      /* X cropping is handled here in lookup table */
      orig_x_table[col] = (col * 10 / scaling_factor) + crop_start_x;
    }
  }
}

static void write_raw(uint16 h)
{
#ifdef __CC65__
  #define dst_ptr zp6p
  #define cur_y zp8p
  #define cur_orig_x zp10ip
  #define cur_orig_y zp12ip
#else
  uint8 *dst_ptr;
  uint8 *cur_y;
  uint16 *cur_orig_x;
  uint8 **cur_orig_y;
#endif
  static uint16 *end_orig_x = NULL;
  static uint8 **end_orig_y = NULL;

  if (end_orig_y == NULL) {
    end_orig_y = orig_y_table + scaled_band_height;
    end_orig_x = orig_x_table + FILE_WIDTH;
  } else if (h == last_band && last_band_crop) {
    /* Skip end of last band if cropping */
    end_orig_y = orig_y_table + last_band_crop;
    output_write_len -= (scaled_band_height - last_band_crop) * FILE_WIDTH;
  }

  /* Scale (nearest neighbor)*/
  dst_ptr = raw_image;
  cur_orig_y = orig_y_table + 0;
  do {
    cur_orig_x = orig_x_table + 0;
    cur_y = (uint8 *)*cur_orig_y;
    do {
      *dst_ptr = *(cur_y + *cur_orig_x);
      histogram[*dst_ptr]++;
      dst_ptr++;
    } while (++cur_orig_x < end_orig_x);
  } while (++cur_orig_y < end_orig_y);

  fwrite (raw_image, 1, output_write_len, ofp);
}

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

  memset(raw_image, 0, raw_image_size);
  build_scale_table(ofname);

  clrscr();
  printf("Decompressing...\n");
  progress_bar(0, 1, 80*22, 0, height);

  for (h = 0; h < crop_end_y; h += QT_BAND) {
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
