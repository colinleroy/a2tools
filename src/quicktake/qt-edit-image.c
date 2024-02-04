#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dgets.h"
#include "extended_conio.h"
#include "file_select.h"
#include "hgr.h"
#include "hgr_addrs.h"
#include "path_helper.h"
#include "progress_bar.h"
#include "scrollwindow.h"
#include "simple_serial.h"
#include "clrzone.h"
#include "qt-conv.h"
#include "qt-edit-image.h"
#include "qt-serial.h"
#include "qt-state.h"

#ifndef __CC65__
#include "tgi_compat.h"
void x86_64_tgi_set(int x, int y, int color) {
  tgi_setcolor(color);
  tgi_setpixel(x, y);
}
#else
#define x86_64_tgi_set(x,y,c)
#endif

extern uint8 scrw, scrh;

#ifdef __CC65__
  #pragma static-locals(push, on)
#endif

#define BUF_SIZE 64

#define DITHER_NONE   0
#define DITHER_BAYER  1
#define DITHER_SIERRA 2
#define DITHER_THRESHOLD 128U
#define DEFAULT_BRIGHTEN 0

#define X_OFFSET ((HGR_WIDTH - file_width) / 2)

static int8 err[FILE_WIDTH * 2];

FILE *ifp, *ofp;

int16 angle = 0;
uint8 auto_level = 1;
uint8 dither_alg = DITHER_SIERRA;
uint8 resize = 1;
uint8 cropping = 0;
uint8 zoom_level = 0;
uint16 crop_start_x = 0, crop_end_x;
uint16 crop_start_y = 0, crop_end_y;
int8 brighten = DEFAULT_BRIGHTEN;

void get_program_disk(void) {
  while (reopen_start_device() != 0) {
    clrscr();
    gotoxy(13, 12);
    cputs("Please reinsert the program disk, then press any key.");
    cgetc();
  }
}

#pragma code-name(push, "LOWCODE")

static char imgname[FILENAME_MAX];
#ifdef __CC65__
#define FOUR_NUM_WIDTH 16
#else
#define FOUR_NUM_WIDTH 64
#endif
static char args[FILENAME_MAX + FOUR_NUM_WIDTH];

void qt_convert_image_with_crop(const char *filename, uint16 sx, uint16 sy, uint16 ex, uint16 ey) {
  set_scrollwindow(0, scrh);
  clrscr();
  cputs("Image conversion\r\n\r\n");
  if (!filename) {
    char *tmp;

    cputs("Image: ");
    tmp = file_select(wherex(), wherey(), scrw - wherex(), wherey() + 10, 0, "Select an image file");
    if (tmp == NULL)
      return;
    strcpy(imgname, tmp);
    free(tmp);
  } else {
    strcpy(imgname, filename);
  }

  if (imgname[0]) {
    FILE *fp = fopen(imgname, "rb");
    if (fp) {
      fread(magic, 1, 4, fp);
      magic[4] = '\0';
      fclose(fp);
    }

    get_program_disk();

    snprintf(args, FILENAME_MAX + FOUR_NUM_WIDTH - 1, "%s %d %d %d %d", imgname, sx, sy, ex, ey);

    if (!strcmp(magic, QTKT_MAGIC)) {
      exec("qtktconv", args);
    } else if (!strcmp(magic, QTKN_MAGIC)) {
      exec("qtknconv", args);
    } else if (!strcmp(magic, JPEG_EXIF_MAGIC)) {
      exec("jpegconv", args);
    } else {
      cputs("\r\nUnknown file type.\r\n");
      cgetc();
    }
  }
}

void qt_convert_image(const char *filename) {
  qt_convert_image_with_crop(filename, 0, 0, 640, 480);
}

static uint8 **cur_hgr_baseaddr_ptr;
static uint8 *cur_hgr_baseaddr_val; /* shortcut ptr */
static uint16 histogram[256];
static uint8 opt_histogram[256];
#define NUM_PIXELS 49152U //256*192

#ifdef __CC65__
#define cur_histogram zp6ip
#define cur_opt_histogram zp8p
#else
uint16 *cur_histogram;
uint8 *cur_opt_histogram;
#endif

static void histogram_equalize(void) {
  uint8 x = 0;
  uint16 curr_hist = 0;

  if (auto_level) {
    ifp = fopen(HIST_NAME, "r");
    if (ifp == NULL) {
      goto fallback_std;
    }
    fread(histogram, sizeof(uint16), 256, ifp);
    fclose(ifp);

    cputs("Histogram equalization...\r\n");
    cur_histogram = histogram;
    cur_opt_histogram = opt_histogram;
    do {
      curr_hist += *(cur_histogram++);
      *(cur_opt_histogram++) = (uint8)((((uint32)curr_hist * 255)) / NUM_PIXELS);
    } while (++x);
  } else {
fallback_std:
    cur_opt_histogram = opt_histogram;
    do {
      *(cur_opt_histogram++) = x;
    } while (++x);
  }
}

#pragma code-name(pop)

#ifdef __CC65__
#define cur_thumb_data zp10p
#else
uint8 *cur_thumb_data;
#endif

static void thumb_histogram(FILE *ifp) {
  uint8 x = 0, read;
  uint16 *histogram = (uint16 *)err;
  uint16 curr_hist = 0;

  while ((read = fread(buffer, 1, 255, ifp)) != 0) {
    cur_thumb_data = buffer;
    do {
      uint8 v = *cur_thumb_data;
      histogram[((v&0x0F) << 4)]++;
      histogram[((v&0xF0))]++;
      cur_thumb_data++;
      x++;
    } while (x != read);
  }
  x = 0;
  cur_histogram = histogram;
  cur_opt_histogram = opt_histogram;
  do {
    curr_hist += *(cur_histogram++);
    *(cur_opt_histogram++) = (uint8)((((uint32)curr_hist * 255)) / (THUMB_WIDTH*THUMB_HEIGHT));
  } while (++x);
  return;
}

int8 bayer_map[64] = {
   0, 32,  8, 40,  2, 34, 10, 42,
  48, 16, 56, 24, 50, 18, 58, 26,
  12, 44,  4, 36, 14, 46,  6, 38,
  60, 28, 52, 20, 62, 30, 54, 22,
   3, 35, 11, 43,  1, 33,  9, 41,
  51, 19, 59, 27, 49, 17, 57, 25,
  15, 47,  7, 39, 13, 45,  5, 37,
  63, 31, 55, 23, 61, 29, 53, 21
};

static void init_data (void)
{
  static uint8 init_done = 0;
  int8 *m;
  uint16 x;

  if (init_done) {
    return;
  }

  init_hgr_base_addrs();
  /* Fixup (standardize, divide) Bayer map once and for all*/
  m = bayer_map + 0;
  for (x = 0; x < 64; x++) {
    *m = (*m - 32) << 2;
    m++;
  }
  histogram_equalize();
  init_done = 1;
}

#ifndef __CC65__
char HGR_PAGE[HGR_LEN];
#endif

static void invert_selection(void) {
  uint16 x, lx, rx;
#ifdef __CC65__
  #define y zp6
  #define a zp8p
  #define b zp10p
#else
  uint8 y, *a, *b;
#endif

  /* Scale back, we use 640x480 based crop values but display
   * them at 256x192
   */
  uint16 dsx = crop_start_x * 4 / 10;
  uint16 dex = crop_end_x * 4 / 10;
  uint16 dsy = crop_start_y * 4 / 10;
  uint16 dey = crop_end_y * 4 / 10;

  #define START_OFFSET ((HGR_WIDTH - FILE_WIDTH) / 2)

  lx = div7_table[dsx + START_OFFSET];
  rx = div7_table[dex + START_OFFSET];

  /* Invert horizontal lines */
  a = hgr_baseaddr[dsy] + lx;
  b = hgr_baseaddr[dey] + lx;
  for (x = dsx; x < dex; x+=7) {
    *a = ~(*a);
    *b = ~(*b);
    a++;
    b++;
  }
  /* Invert vertical lines */
  for (y = dsy + 1; y < dey - 1; y++) {
    uint8 *by = hgr_baseaddr[y];
    a = by + lx;
    b = by + rx;
    *a = ~(*a);
    *b = ~(*b);
  }
}

static uint8 reedit_image(const char *ofname, uint16 src_width) {
  char c, *cp;

start_edit:
  do {
    clrscr();
    cputs("Rotate: L:left - U:180 - R:right");
    if (angle == 90 || angle == 270) {
      if (resize)
        cputs("; C: Crop");
      else
        cputs("; C: Fit");
    } else if (angle == 0 && !(src_width % 320)) {
      cputs("; C: Reframe");
    }
    printf("\nH: Auto-level %s; B: Brighten - D: Darken (Current %s%d)\n",
           auto_level ? "off":"on",
           brighten > 0 ? "+":"",
           brighten);
    printf("Dither with E: Sierra Lite / Y: Bayer / N: No dither (Current: %s)\n"
           "S: Save - Escape: Exit - Any other key: Hide help",
           dither_alg == DITHER_BAYER ? "Bayer"
            : dither_alg == DITHER_SIERRA ? "Sierra Lite" : "None");
  c = tolower(cgetc());
#ifdef __CC65__
    if (!hgr_mix_is_on()) {
      hgr_mixon();
    } else
#endif
    {
      if (!cropping) {
        switch(c) {
          case CH_ESC:
            clrscr();
            cputs("Exit without saving? (y/N)");
            c = tolower(cgetc());
            if (c == 'y')
              goto done;
            break;
          case 's':
            clrscr();
            goto save;
          case 'r':
            angle += 90;
            return 1;
          case 'l':
            angle -= 90;
            return 1;
          case 'u':
            angle += 180;
            return 1;
          case 'h':
            auto_level = !auto_level;
            histogram_equalize();
            return 1;
          case 'c':
            if (angle == 0 && !(src_width % 320)) {
              cropping = 1;
              goto crop_again;
            } else {
              resize = !resize;
            }
            return 1;
          case 'y':
            dither_alg = DITHER_BAYER;
            return 1;
          case 'e':
            dither_alg = DITHER_SIERRA;
            return 1;
          case 'n':
            dither_alg = DITHER_NONE;
            return 1;
          case 'b':
            brighten += 16;
            return 1;
          case 'd':
            brighten -= 16;
            return 1;
          default:
            hgr_mixoff();
        }
      } else {
        /* will be divided by 2 if 320x240, we want it
         * to start on a band boundary */
        uint8 move_offset;
crop_again:
        move_offset = src_width == 640 ? QT_BAND : QT_BAND*2;
        clrscr();
        if (src_width == 640) {
          cputs("+: Zoom in; -: Zoom out; ");
        }
        cputs("Arrow keys: Move selection\r\n"
               "Enter: Reframe; Escape: Cancel");
        if (zoom_level) {
          /* Set back pixels at previous crop border */
          invert_selection();
        } else {
zoom_level_1:
          zoom_level = 1;
          crop_start_x = crop_start_y = 0;
          crop_end_x = crop_start_x + 512;
          crop_end_y = crop_start_y + 384;
        }
        switch(c) {
          case CH_ESC:
            cropping = 0;
            zoom_level = 0;
            return 1;
          case CH_ENTER:
            cropping = 0;
            zoom_level = 0;
            init_text();
            qt_convert_image_with_crop(ofname, crop_start_x, crop_start_y, crop_end_x, crop_end_y);
            return 1;
          case '+':
            if (zoom_level == 1 && src_width == 640) {
zoom_level_2:
              zoom_level = 2;
              crop_start_x = crop_start_y = 0;
              crop_end_x = crop_start_x + 320;
              crop_end_y = crop_start_y + 240;
            } else if (zoom_level == 2) {
              zoom_level = 3;
              crop_start_x = crop_start_y = 0;
              crop_end_x = crop_start_x + 256;
              crop_end_y = crop_start_y + 192;
            }
            break;
          case '-':
            if (zoom_level == 3)
              goto zoom_level_2;
            else if (zoom_level == 2)
              goto zoom_level_1;
            break;
          case CH_CURS_RIGHT:
            if (crop_end_x < 640) {
              crop_start_x += move_offset;
              crop_end_x += move_offset;
            }
            break;
          case CH_CURS_LEFT:
            if (crop_start_x > 0) {
              crop_start_x -= move_offset;
              crop_end_x -= move_offset;
            }
            break;
          case CH_CURS_DOWN:
            if (crop_end_y < 480) {
              crop_start_y += move_offset;
              crop_end_y += move_offset;
            }
            break;
          case CH_CURS_UP:
            if (crop_start_y > 0) {
              crop_start_y -= move_offset;
              crop_end_y -= move_offset;
            }
            break;
        }
        invert_selection();
        c = cgetc();
        goto crop_again;
      }
    }
  } while (1);

save:
  strcpy((char *)buffer, ofname);
  if ((cp = strrchr ((char *)buffer, '.')))
    *cp = 0;
  strcat ((char *)buffer, ".hgr");
  cputs("Save to: ");
  dget_text((char *)buffer, 63, NULL, 0);
  if (buffer[0] == '\0') {
    goto start_edit;
  }

open_again:
  ofp = fopen((char *)buffer, "w");
  if (ofp == NULL) {
    printf("Please insert image floppy for %s, or Escape to return\n", (char *)buffer);
    if (cgetc() != CH_ESC)
      goto open_again;
    goto start_edit;
  }
  printf("Saving...\n");
  fseek(ofp, 0, SEEK_SET);
  if (fwrite((char *)HGR_PAGE, 1, HGR_LEN, ofp) < HGR_LEN) {
    printf("Error. Press a key to continue...\n");
    fclose(ofp);
    cgetc();
    goto start_edit;
  }
  fclose(ofp);

  printf("Done. Go back to Edition, View, or main Menu? (E/v/m)");
  c = tolower(cgetc());
  if (c == 'v') {
    state_set(STATE_EDIT, src_width, (char *)buffer);
    qt_view_image((char *)buffer);
    goto done;
  }
  if (c != 'm') {
    goto start_edit;
  }
done:
  hgr_mixoff();
  init_text();
  clrscr();

  return 0;
}

static uint8 thumb_buf[THUMB_WIDTH * 2];

#pragma inline-stdfuncs(push, on)
#pragma allow-eager-inline(push, on)
#pragma codesize(push, 200)
#pragma register-vars(push, on)

void convert_temp_to_hgr(const char *ifname, const char *ofname, uint16 p_width, uint16 p_height, uint8 serial_model) {
  /* Rotation/cropping variables */
  uint8 start_x, i;
  uint8 x, end_x;
  uint16 dx;

  uint16 dy;
  uint16 off_x, y, off_y;
  uint16 file_width;
#ifdef __CC65__
  #define cur_d7 zp6p
  #define cur_m7 zp8p
  #define buf_ptr zp10p
  #define ptr zp12p
#else
  uint8 *buf_ptr;
  uint8 *cur_d7, *cur_m7;
  uint8 *ptr;
#endif
  uint8 scaled_dx, scaled_dy, prev_scaled_dx, prev_scaled_dy;
  int8 xdir = 1, ydir = 1;
  int8 err2, err1;

  uint8 invert_coords = 0;

  /* Used for both Sierra and Bayer */
  register int8 *regptr1, *regptr2, *regptr3;

  /* Sierra variables */
  #define cur_err_x_y regptr1
  #define cur_err_xmin1_yplus1 regptr2
  #define cur_err_x_yplus1 regptr3
  int16 buf_plus_err;
  int8 *cur_err_line = err;
  int8 *next_err_line;

  /* Bayer variables */
  #define bayer_map_x regptr1
  #define bayer_map_y regptr2
  #define end_bayer_map_x regptr3
  int8 *end_bayer_map_y;

  uint8 pixel;
  uint8 file_height;

  /* General variables */
  uint8 cur_hgr_row;
  uint8 cur_hgr_mod;
  uint8 opt_val;
  uint8 is_thumb = (p_width == THUMB_WIDTH*2);
  uint8 is_qt100 = (serial_model == QT_MODEL_100);

  file_width = p_width;
  file_height = p_height;

  next_err_line = err + file_width;
  init_data();

  clrscr();
  printf("Converting %s (Esc to stop)...\n", ofname);

  ifp = fopen(ifname, "r");
  if (ifp == NULL) {
    printf("Can't open %s\n", ifname);
    return;
  }
  if (is_thumb) {
    thumb_histogram(ifp);
    rewind(ifp);
    dither_alg = DITHER_BAYER;
  }

  cputs("Dithering...\r\n");
  bzero(err, sizeof err);
  bzero((char *)HGR_PAGE, HGR_LEN);

  progress_bar(wherex(), wherey(), scrw, 0, file_height);

  start_x = 0;
  end_x = file_width == 256 ? 0 : file_width;

  /* Init to safe value */
  prev_scaled_dx = prev_scaled_dy = 100;

  /* Setup offsets and directions */
  switch (angle) {
    case 0:
      off_x = X_OFFSET;
      off_y = (HGR_HEIGHT - file_height) / 2;
      cur_hgr_baseaddr_ptr = hgr_baseaddr + off_y;
      cur_hgr_baseaddr_val = *cur_hgr_baseaddr_ptr;
      xdir = +1;
      ydir = +1;
      invert_coords = 0;
      break;
    case 90:
      off_x = 0;
      if (resize) {
        off_y = 212 * 4 / 3;
      } else {
        off_y = HGR_WIDTH - 45;
        start_x = 32;
        end_x = file_width - 33;
      }
      xdir = +1;
      ydir = -1;
      invert_coords = 1;
      break;
    case 270:
      if (resize) {
        off_x = file_width - 1;
        off_y = 68 * 4 / 3;
      } else {
        off_x = HGR_HEIGHT - 1;
        off_y = 44;
        start_x = 32;
        end_x = file_width - 33;
      }
      xdir = -1;
      ydir = +1;
      invert_coords = 1;
      break;
    case 180:
      off_x = HGR_WIDTH - X_OFFSET;
      off_y = file_height - 1;
      cur_hgr_baseaddr_ptr = hgr_baseaddr + off_y;
      cur_hgr_baseaddr_val = *cur_hgr_baseaddr_ptr;
      xdir = -1;
      ydir = -1;
      invert_coords = 0;
      break;
  }

  /* Line loop */
  bayer_map_y = bayer_map + 0;
  end_bayer_map_y = bayer_map_y + 64;

  for(y = 0, dy = off_y; y != file_height;) {

    /* Load data from file */
    if (!is_thumb) {
      fread(buffer, 1, FILE_WIDTH, ifp);
    } else {
      uint8 a, b, c, d, off;
      /* assume thumbnail at 4bpp and zoom it */
      if (is_qt100) {
        if (!(y & 1)) {
          fread(buffer, 1, THUMB_WIDTH / 2, ifp);
          /* Unpack */
          i = 39;
          do {
            c   = buffer[i];
            a   = (c & 0xF0);
            b   = ((c & 0x0F) << 4);
            off = i * 4;
            buffer[off++] = a;
            buffer[off++] = a;
            buffer[off++] = b;
            buffer[off] = b;
          } while (i--);
        }
      } else {
        unsigned char *cur_in, *cur_out;
        unsigned char *orig_in, *orig_out;
        /* Whyyyyyy do they do that */
        if (!(y % 4)) {
          /* Expand the next two lines from 4bpp thumb_buf to 8bpp buffer */
          fread(thumb_buf, 1, THUMB_WIDTH, ifp);
          orig_in = cur_in = thumb_buf;
          orig_out = cur_out = buffer;
          for (x = 0; x < THUMB_WIDTH; x++) {
            c = *cur_in++;
            a   = (c & 0xF0);
            b   = ((c & 0x0F) << 4);
            *cur_out++ = a;
            *cur_out++ = b;
          }

          /* Reorder bytes from buffer back to thumb_buf */
          orig_in = cur_in = buffer;
          orig_out = cur_out = thumb_buf;
          for (i = 0; i < THUMB_WIDTH * 2; ) {
            if (i < THUMB_WIDTH*3/2) {
              a = *cur_in++;
              b = *cur_in++;
              c = *cur_in++;

              *(cur_out) = a;
              *(cur_out + THUMB_WIDTH) = c;
              cur_out++;
              *(cur_out) = b;
              cur_out++;
              i+=3;
            } else {
              i++;
              cur_out++;
              d = *cur_in++;
              *(cur_out) = d;
              cur_out++;
            }
          }
          /* Finally copy the first line of thumb_buf to buffer for display,
           * upscaling horizontally */
          orig_in = cur_in = thumb_buf;
          orig_out = cur_out = buffer;
          for (x = 0; x < THUMB_WIDTH; x++) {
            *cur_out = *cur_in;
            cur_out++;
            *cur_out = *cur_in;
            cur_out++;
            cur_in++;
          }
        } else if (!(y % 2)) {
          /* Copy the second line of thumb_buf to buffer for display,
           * upscaling horizontally */
          orig_in = cur_in = thumb_buf + THUMB_WIDTH;
          orig_out = cur_out = buffer;
          for (x = 0; x < THUMB_WIDTH; x++) {
            *cur_out = *cur_in;
            cur_out++;
            *cur_out = *cur_in;
            cur_out++;
            cur_in++;
          }
        } else {
          /* Reuse the previous buffer line once for upscaling */
        }
      }
    }

    /* Calculate hgr base coordinates for the line */
    if (invert_coords) {
      if (resize) {
        scaled_dy = (dy + (dy << 1)) >> 2;  /* *3/4 */
        if (scaled_dy == prev_scaled_dy) {
          /* Avoid rewriting same destination line twice
           * It results in ugly dithering */
          goto next_line;
        }
        prev_scaled_dy = scaled_dy;
      } else {
        scaled_dy = dy;
      }
      cur_hgr_row = div7_table[scaled_dy];
      cur_hgr_mod = mod7_table[scaled_dy];
    }
#ifndef __CC65__
    x = start_x;
    buf_ptr = buffer + x;
    dx = off_x;

    cur_d7 = div7_table + dx;
    cur_m7 = mod7_table + dx;
#else
    __asm__("lda %v", start_x);
    __asm__("sta %v", x);

    __asm__("ldx #>(%v)", buffer);
    __asm__("clc");
    __asm__("adc #<(%v)", buffer);
    __asm__("sta %v", buf_ptr);
    __asm__("bcc %g", noof2);
    __asm__("inx");
    __asm__("clc");
    noof2:
    __asm__("stx %v+1", buf_ptr);

    __asm__("lda %v", off_x);
    __asm__("sta %v", dx);
    __asm__("ldx %v+1", off_x);
    __asm__("stx %v+1", dx);

    __asm__("tay"); /* backup dx low */
    __asm__("adc #<(%v)", div7_table);
    __asm__("sta %v", cur_d7);
    __asm__("txa");
    __asm__("adc #>(%v)", div7_table);
    __asm__("sta %v+1", cur_d7);

    __asm__("tya"); /* restore dx low */
    __asm__("adc #<(%v)", mod7_table);
    __asm__("sta %v", cur_m7);
    __asm__("txa");
    __asm__("adc #>(%v)", mod7_table);
    __asm__("sta %v+1", cur_m7);
#endif

    if (dither_alg == DITHER_SIERRA) {
#ifndef __CC65__
      /* Rollover next error line */
      int8 *tmp = cur_err_line;

      cur_err_line = next_err_line;
      cur_err_x_y = cur_err_line + x;

      next_err_line = tmp;
      cur_err_x_yplus1 = next_err_line + x;
      cur_err_xmin1_yplus1 = cur_err_x_yplus1 - 1;

      bzero(next_err_line, file_width);

      err2 = 0;
#else
      __asm__("lda %v", cur_err_line);
      __asm__("ldx %v+1", cur_err_line);
      __asm__("tay"); /* start swapping cur/next, now tmp in YX */
      __asm__("phx");
      __asm__("lda %v", next_err_line);
      __asm__("ldx %v+1", next_err_line);
      __asm__("sta %v", cur_err_line);
      __asm__("stx %v+1", cur_err_line);

      /* cur_err_x_y = cur_err_line + x; */
      __asm__("clc");
      __asm__("adc %v", x);
      __asm__("sta %v", cur_err_x_y);
      __asm__("bcc %g", noof3);
      __asm__("inx");
      noof3:
      __asm__("stx %v+1", cur_err_x_y);

      __asm__("plx"); /* finish swapping cur/next */
      __asm__("tya");
      __asm__("sta %v", next_err_line);
      __asm__("stx %v+1", next_err_line);
      __asm__("jsr pushax"); /* Push for bzero */

      /* cur_err_x_yplus1 = next_err_line + x; */
      __asm__("clc");
      __asm__("adc %v", x);
      __asm__("sta %v", cur_err_x_yplus1);
      __asm__("bcc %g", noof4);
      __asm__("inx");
      noof4:
      __asm__("stx %v+1", cur_err_x_yplus1);

      /* cur_err_xmin1_yplus1 = cur_err_x_yplus1 - 1;*/
      __asm__("sec");
      __asm__("sbc #1");
      __asm__("sta %v", cur_err_xmin1_yplus1);
      __asm__("bcs %g", nouf1);
      __asm__("dex");
      nouf1:
      __asm__("stx %v+1", cur_err_xmin1_yplus1);

      __asm__("lda %v", file_width);
      __asm__("ldx %v+1", file_width);
      __asm__("jsr %v", bzero);

      __asm__("stz %v", err2);
#endif

    } else if (dither_alg == DITHER_BAYER) {
      bayer_map_x = bayer_map_y + 0;
      end_bayer_map_x = bayer_map_x + 8;
    }

    /* Column loop */
    do {
      /* Get destination pixel */
      if (invert_coords) {
        if (resize) {
          scaled_dx = (dx + (dx << 1)) >> 2; /* *3/4 */
          if (scaled_dx == prev_scaled_dx) {
            /* Avoid rewriting same destination pixel twice
             * It results in ugly dithering */
            goto next_pixel;
          }
          prev_scaled_dx = scaled_dx;
        } else {
          scaled_dx = dx;
        }
        ptr = hgr_baseaddr[scaled_dx] + cur_hgr_row;
        pixel = cur_hgr_mod;
      } else {
#ifndef __CC65__
        ptr = cur_hgr_baseaddr_val + *cur_d7;
        pixel = *cur_m7;
#else
        __asm__("ldx %v+1", cur_hgr_baseaddr_val);
        __asm__("lda %v", cur_hgr_baseaddr_val);
        __asm__("clc");
        __asm__("adc (%v)", cur_d7);
        __asm__("sta %v", ptr);
        __asm__("bcc %g", noof1);
        __asm__("inx");
        noof1:
        __asm__("stx %v+1", ptr);

        __asm__("lda (%v)", cur_m7);
        __asm__("sta %v", pixel);
#endif
      }

#ifndef __CC65__
      opt_val = *buf_ptr;
      opt_val = opt_histogram[opt_val];
#else
      /* Compensate optimizer */
      __asm__("lda   (%v)", buf_ptr);
      __asm__("tay");
      __asm__("lda %v,y",   opt_histogram);
      __asm__("sta %v",     opt_val);
#endif
      if (brighten) {
        int16 t = opt_val + brighten;
        if (t < 0)
          opt_val = 0;
        else if (t & 0xff00) /* > 255, but faster as we're sure it's non-negative */
          opt_val = 255;
        else
          opt_val = t;
      }

      /* Dither */
      if (dither_alg == DITHER_SIERRA) {
        buf_plus_err = opt_val + err2;
#ifndef __CC65__
        buf_plus_err += *cur_err_x_y;
        if (buf_plus_err < DITHER_THRESHOLD) {
          /* pixel's already black */
          x86_64_tgi_set(dx, y, TGI_COLOR_BLACK);
        } else {
          *ptr |= pixel;
          x86_64_tgi_set(dx, y, TGI_COLOR_WHITE);
        }
        err2 = ((int8)buf_plus_err) >> 1; /* cur_err * 2 / 4 */
        err1 = err2 >> 1;    /* cur_err * 1 / 4 */
#else
        __asm__("ldx #$00");
        __asm__("lda (%v)", cur_err_x_y);
        __asm__("bpl %g", positive_s);
        __asm__("dex");
        positive_s:
        __asm__("clc");
        __asm__("adc %v", buf_plus_err);
        __asm__("sta %v", buf_plus_err);
        __asm__("txa");
        __asm__("adc %v+1", buf_plus_err);
        __asm__("sta %v+1", buf_plus_err);

        __asm__("bmi %g", black_pix);
        __asm__("bne %g", white_pix);
        __asm__("lda %v", buf_plus_err);
        __asm__("cmp #<(%b)", DITHER_THRESHOLD);
        __asm__("bcc %g", black_pix_direct);
        white_pix:
        __asm__("lda (%v)", ptr);
        __asm__("ora %v", pixel);
        __asm__("sta (%v)", ptr);

        black_pix:
        __asm__("lda %v", buf_plus_err);
        black_pix_direct:
        __asm__("cmp #$80");
        __asm__("ror a");
        __asm__("sta %v", err2);
        __asm__("cmp #$80");
        __asm__("ror a");
        __asm__("sta %v", err1);
#endif

        if (x > 0) {
#ifndef __CC65__
          *cur_err_xmin1_yplus1    += err1;
#else
          __asm__("lda (%v)", cur_err_xmin1_yplus1);
          __asm__("clc");
          __asm__("adc %v", err1);
          __asm__("sta (%v)", cur_err_xmin1_yplus1);
#endif
        }
#ifndef __CC65__
        *cur_err_x_yplus1          += err1;
#else
        __asm__("lda (%v)", cur_err_x_yplus1);
        __asm__("clc");
        __asm__("adc %v", err1);
        __asm__("sta (%v)", cur_err_x_yplus1);
#endif

        /* Advance cursors */
        cur_err_x_y++;
        cur_err_x_yplus1++;
        cur_err_xmin1_yplus1++;
      } else if (dither_alg == DITHER_BAYER) {
        buf_plus_err = opt_val;
#ifndef __CC65__
        buf_plus_err += *bayer_map_x;
#else
        __asm__("ldx #$00");
        __asm__("lda (%v)", bayer_map_x);
        __asm__("bpl %g", positive_b);
        __asm__("dex");
        positive_b:
        __asm__("clc");
        __asm__("adc %v", buf_plus_err);
        __asm__("sta %v", buf_plus_err);
        __asm__("txa");
        __asm__("adc %v+1", buf_plus_err);
        __asm__("sta %v+1", buf_plus_err);
#endif
        if (buf_plus_err < DITHER_THRESHOLD) {
          x86_64_tgi_set(dx, y, TGI_COLOR_BLACK);
        } else {
          *ptr |= pixel;
          x86_64_tgi_set(dx, y, TGI_COLOR_WHITE);
        }
        /* Advance Bayer X */
        bayer_map_x++;
        if (bayer_map_x == end_bayer_map_x)
          bayer_map_x = bayer_map_y + 0;
      } else if (dither_alg == DITHER_NONE) {
        if (opt_val < DITHER_THRESHOLD) {
          x86_64_tgi_set(dx, y, TGI_COLOR_BLACK);
        } else {
          *ptr |= pixel;
          x86_64_tgi_set(dx, y, TGI_COLOR_WHITE);
        }
      }

next_pixel:
#ifndef __CC65__
      if (xdir < 0) {
        dx--;
        cur_d7--;
        cur_m7--;
      } else {
        dx++;
        cur_d7++;
        cur_m7++;
      }
      x++;
      buf_ptr++;
#else
      __asm__("lda %v", xdir);
      __asm__("bmi %g", dec_counters);
        dx++;
        cur_d7++;
        cur_m7++;
        __asm__("bra %g", inc_x);
      dec_counters:
        dx--;
        cur_d7--;
        cur_m7--;
      inc_x:
      x++;
      buf_ptr++;
#endif
    } while (x != end_x);
    if (y % 16 == 0) {
      progress_bar(-1, -1, scrw, y, file_height);
      if (kbhit()) {
        if (cgetc() == CH_ESC)
          goto stop;
      }
    }
    if (dither_alg == DITHER_BAYER) {
      /* Advance Bayer Y */
      bayer_map_y += 8;
      if (bayer_map_y == end_bayer_map_y) {
        bayer_map_y = bayer_map + 0;
      }
    }
next_line:
#ifndef __CC65__
    if (ydir < 0) {
      cur_hgr_baseaddr_ptr--;
      cur_hgr_baseaddr_val = *cur_hgr_baseaddr_ptr;
      dy--;
    } else {
      cur_hgr_baseaddr_ptr++;
      cur_hgr_baseaddr_val = *cur_hgr_baseaddr_ptr;
      dy++;
    }
    y++;
#else
    __asm__("lda %v", ydir);
    __asm__("bmi %g", dec_ycounters);
      dy++;
      __asm__("lda %v", cur_hgr_baseaddr_ptr);
      __asm__("ldx %v+1", cur_hgr_baseaddr_ptr);
      __asm__("clc");
      __asm__("adc #2");
      __asm__("bcc %g", finish_inc);
      __asm__("inx");
      goto finish_inc;

    dec_ycounters:
      dy--;
      __asm__("lda %v", cur_hgr_baseaddr_ptr);
      __asm__("ldx %v+1", cur_hgr_baseaddr_ptr);
      __asm__("sec");
      __asm__("sbc #2");
      __asm__("bcs %g", finish_inc);
      __asm__("dex");
    finish_inc:
    __asm__("sta %v", cur_hgr_baseaddr_ptr);
    __asm__("stx %v+1", cur_hgr_baseaddr_ptr);
    __asm__("sta ptr1");
    __asm__("stx ptr1+1");
    __asm__("lda (ptr1)");
    __asm__("sta %v", cur_hgr_baseaddr_val);
    __asm__("ldy #$01");
    __asm__("lda (ptr1),y");
    __asm__("sta %v+1", cur_hgr_baseaddr_val);
    y++;
#endif
  }
  progress_bar(-1, -1, scrw, file_height, file_height);
stop:
  fclose(ifp);
#ifndef __CC65__
  ifp = fopen("HGR","wb");
  fwrite((char *)HGR_PAGE, 1, HGR_LEN, ifp);
  fclose(ifp);
#endif
}

#pragma register-vars(pop)
#pragma codesize(pop)
#pragma allow-eager-inline(pop)
#pragma inline-stdfuncs(pop)

void qt_edit_image(const char *ofname, uint16 src_width) {
  set_scrollwindow(20, scrh);
  do {
    if (angle >= 360)
      angle -= 360;
    if (angle < 0)
      angle += 360;
    convert_temp_to_hgr(TMP_NAME, ofname, FILE_WIDTH, FILE_HEIGHT, QT_MODEL_UNKNOWN);
  } while (reedit_image(ofname, src_width));
}

uint8 qt_view_image(const char *filename) {
  if (filename)
    snprintf((char *)args, sizeof(args) - 1, "%s SLOWTAKE", filename);
  else
    snprintf((char *)args, sizeof(args) - 1, "___SEL___ SLOWTAKE");

  init_text();
  return exec("imgview", (char *)args);
}

#ifdef __CC65__
  #pragma static-locals(pop)
#endif
