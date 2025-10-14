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

/* Shared with decoders */
uint16 height, width;

/* Cache */
uint8 *cache_end;

/* Source file access. The cache mechanism is shared with decoders
 * but the cache size is set by decoders. Decoders should not have
 * access to the file pointers
 */
static const char *ifname;

#ifndef __CC65__
extern uint16 histogram[256];
#else
extern uint8 histogram_low[256];
extern uint8 histogram_high[256];
#endif

extern uint16 crop_start_x, crop_start_y, crop_end_x, crop_end_y;
extern uint16 effective_width;

void __fastcall__ build_scale_table(const char *ofname);
void __fastcall__ write_raw(uint16 h);

int ifd = -1;
int ofd = -1;
int fullsize_fd = -1;


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

#ifdef QTKNCONV
    if (!memcmp(cache_start, QTKN_MAGIC, 4)) {
      width = DECODE_WIDTH;
      height = DECODE_HEIGHT;
    }
#endif
  }
#endif
#ifdef JPEGCONV
  if (!memcmp(cache_start, JPEG_EXIF_MAGIC, 4)) {
    /* FIXME QT 200 implied, 640x480 (scaled down) implied, that sucks */
    cputs((char *)name);
    cputs("...\r\n");
    width = DECODE_WIDTH;
    height = DECODE_HEIGHT;
    cur_cache_ptr = cache_start;
  }
#endif
  return 0;
}

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
