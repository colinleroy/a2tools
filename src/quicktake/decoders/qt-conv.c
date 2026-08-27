/*
  decoders wrapper
  Copyright 2023, Colin Leroy-Mira <colin@colino.net>

  Decoder's core code. Depends on scaler for scaling and an
  implementation of qt_load_raw(uint16 top), expected to decode
  a band of BAND_HEIGHT pixels high and fill in raw_image[]

  Decoding implementations are expected to provide global variables
  and defines:
  char magic[5];
  char *model;
  uint8 raw_image[]; - the decoded band buffer


  and the decoding functions:
  char qt_setup_decode(void) - check file validity and setup width/height/cache position
  uint8 qt_load_raw(uint16 top) - decode a 20px band

  This file provides the actual uint16 height and width to the decoder.
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

#pragma inline-stdfuncs(push, on)
#pragma allow-eager-inline(push, on)
#pragma codesize(push, 200)
#pragma register-vars(push, on)

#define HDR_LEN 32
#define WH_OFFSET 544

#ifndef __CC65__
uint8 *cur_cache_ptr;
#endif

#ifndef INITIAL_CACHE_READ
#define INITIAL_CACHE_READ CACHE_SIZE
#endif

static uint8 identify(const char *name)
{
  /* INIT */
  height = width = 0;

  /* Fill cache */
  read(ifd, cache_start, INITIAL_CACHE_READ);

  cputsxy(0, 0, "Decompressing image ");
  cputs((char *)name);
  cputs("...\r\n");

  /* Check file type, figure out height and width,
   * and position cache at the beginning of the
   * data stream */
  return qt_setup_decode();
}

#ifdef JPEGCONV
#pragma code-name (pop)
#endif

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
  try_videomode(VIDEOMODE_80COL);
  cputsxy(0, 23, decoder_name);
  cputs(" decoder for Apple II - Free memory: ");
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
  unlink("LARGE_"TMP_NAME);
  fullsize_fd = open ("LARGE_"TMP_NAME, O_RDWR|O_CREAT, 00600);
  if (height == 480) {
    write(fullsize_fd, PNM_HEADER_480, PNM_HEADER_SIZE);
  } else {
    write(fullsize_fd, PNM_HEADER_240, PNM_HEADER_SIZE);
  }
  #endif

  if (ofd < 0) {
    cputs("Can't open\r\n");
    cputs(TMP_NAME);
    cgetc();
    exit(0);
  }

  write(ofd, PNM_HEADER, PNM_HEADER_SIZE);

  build_scale_table(ofname);

  progress_bar(0, 8, 80, 0, height);

  for (h = 0; h < crop_end_y; h += BAND_HEIGHT) {
#ifdef __CC65__
    cputsxy(0, 7, "Decoding    ");
#endif
    progress_bar(-1, -1, 80, h, crop_end_y);

    if (qt_load_raw(h) != 0) {
      cputs("\r\nUnsupported file. Press a key to return.");
      goto out;
    }
    if (h >= crop_start_y) {
#ifdef __CC65__
      cputsxy(0, 7, "Scaling      ");
#endif
      write_raw(h);
    }
  }

#ifdef __CC65__
  cputsxy(0, 7, "Finalizing      ");
#endif
  progress_bar(0, 8, 80, height, height);

  close(ifd);
  #ifndef __CC65__
  close(fullsize_fd);
  #endif
  ifd = -1;

  /* Append histogram */
  lseek(ofd, PNM_HEADER_SIZE + 256*192UL, SEEK_SET);
#ifndef __CC65__
  write(ofd, histogram, sizeof(uint16)*256);
#else
  // histogram_low and high must be contiguous
  write(ofd, histogram_low, sizeof(uint8)*512);
#endif
  close(ofd);
  ofd = -1;

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

#ifndef JPEGCONV
#pragma code-name (pop)
#endif

#ifdef __CC65__
  #pragma static-locals(pop)
#endif
