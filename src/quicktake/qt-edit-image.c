#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dget_text.h"
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
#include "a2_features.h"

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

#define DITHER_NONE   2
#define DITHER_BAYER  1
#define DITHER_SIERRA 0
#define DITHER_THRESHOLD 128U /* Must be 128 for sierra dithering sign check */
#define DEFAULT_BRIGHTEN 0

int8 err_buf[(FILE_WIDTH) * 2];

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
uint8 x_offset;
uint8 crop_pos = 1;

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
    tmp = file_select(0, "Select an image file");
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

    reopen_start_device();

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

#ifndef __CC65__
static uint16 histogram[256];
#else
static uint8 histogram_low[256];
static uint8 histogram_high[256];
#endif

uint8 opt_histogram[256];
#define NUM_PIXELS 49152U //256*192

#ifndef __CC65__
uint16 *cur_histogram;
uint8 *cur_opt_histogram;
#endif

static void histogram_equalize(void) {
#ifndef __CC65__
  uint8 x = 0;
#endif
  uint16 curr_hist = 0;

  if (auto_level) {
    ifp = fopen(HIST_NAME, "r");
    if (ifp == NULL) {
      goto fallback_std;
    }
#ifndef __CC65__
    fread(histogram, sizeof(uint16), 256, ifp);
#else
    fread(histogram_low, sizeof(uint8), 256, ifp);
    fread(histogram_high, sizeof(uint8), 256, ifp);
#endif
    fclose(ifp);

    cputs("Histogram equalization...\r\n");
#ifndef __CC65__
    cur_opt_histogram = opt_histogram;
    cur_histogram = histogram;
    do {
      uint32 tmp;
      curr_hist += *(cur_histogram++);
      tmp = ((uint32)curr_hist << 8) - curr_hist;
      tmp >>= 8;  /* / 256 */
      tmp /= 192; /* / 256/192 */
      *(cur_opt_histogram++) = tmp;
    } while (++x);
#else
    __asm__("ldy #0");
    next_h:
    __asm__("clc");
    __asm__("lda %v,y", histogram_low);
    __asm__("adc %v", curr_hist);
    __asm__("sta %v", curr_hist);
    __asm__("tax"); /* *256 */

    __asm__("lda %v,y", histogram_high);
    __asm__("adc %v+1", curr_hist);
    __asm__("sta %v+1", curr_hist);
    __asm__("sta sreg"); /* * 256 */

    /* Finish curr_hist * 256 with low byte 0 */
    __asm__("lda #0");

    /* -curr_hist => curr_hist * 255 */
    __asm__("sec");
    __asm__("sbc %v", curr_hist);
    __asm__("txa");
    __asm__("sbc %v+1", curr_hist);
    __asm__("tax");
    __asm__("lda sreg");
    __asm__("sbc #0");
    __asm__("sta sreg");

    /* / 256 */
    __asm__("txa");
    __asm__("ldx sreg");

    /* / 192 */
    __asm__("sty tmp1");
    __asm__("jsr pushax");
    __asm__("lda #<%w", HGR_HEIGHT);
    __asm__("jsr tosudiva0");
    __asm__("ldy tmp1");

    __asm__("sta %v,y", opt_histogram);

    __asm__("iny");
    __asm__("bne %g", next_h);
#endif
  } else {
fallback_std:
#ifndef __CC65__
    cur_opt_histogram = opt_histogram;
    do {
      *(cur_opt_histogram++) = x;
    } while (++x);
#else
    __asm__("ldy #0");
    next_val:
    __asm__("tya");
    __asm__("sta %v,y", opt_histogram);
    __asm__("iny");
    __asm__("bne %g", next_val);
#endif
  }
}

#ifdef __CC65__
#define cur_histogram zp6ip
#define cur_opt_histogram zp8p
#define cur_thumb_data zp10p
#else
uint8 *cur_thumb_data;
#endif

static void thumb_histogram(FILE *ifp) {
  uint8 x = 0, read;
  uint16 *histogram = (uint16 *)err_buf;
  uint16 curr_hist = 0;

  while ((read = fread(buffer, 1, 255, ifp)) != 0) {
    cur_thumb_data = buffer;
#ifndef __CC65__
    do {
      uint8 v = *cur_thumb_data;
      histogram[((v&0x0F) << 4)]++;
      histogram[((v&0xF0))]++;
      cur_thumb_data++;
      x++;
    } while (x != read);
#else
    __asm__("lda %v", histogram);
    __asm__("sta ptr1");
    __asm__("sta ptr2");
    __asm__("ldy #0");
    next_byte:
    __asm__("ldx %v+1", histogram);
    __asm__("stx ptr1+1");
    __asm__("stx ptr2+1");

    __asm__("sty %v", x);
    __asm__("lda (%v),y", cur_thumb_data); /* read byte */
    __asm__("tax"); /* backup it */
    __asm__("asl"); /* << 4 low nibble */
    __asm__("asl");
    __asm__("asl");
    __asm__("asl");

    __asm__("asl"); /* shift left for array access */
    __asm__("tay");
    __asm__("bcc %g", noof22);
    __asm__("inc ptr1+1");
    __asm__("clc");
    noof22:
    __asm__("lda (ptr1),y");
    __asm__("adc #1");
    __asm__("sta (ptr1),y");
    __asm__("iny");
    __asm__("lda (ptr1),y");
    __asm__("adc #0");
    __asm__("sta (ptr1),y");

    __asm__("txa");
    __asm__("and #$F0");

    __asm__("asl");
    __asm__("tay");
    __asm__("bcc %g", noof23);
    __asm__("inc ptr2+1");
    __asm__("clc");
    noof23:
    __asm__("lda (ptr2),y");
    __asm__("adc #1");
    __asm__("sta (ptr2),y");
    __asm__("iny");
    __asm__("lda (ptr2),y");
    __asm__("adc #0");
    __asm__("sta (ptr2),y");

    __asm__("ldy %v", x);
    __asm__("iny");
    __asm__("cpy %v", read);
    __asm__("bne %g", next_byte);
#endif
  }
  x = 0;
  cur_histogram = histogram;
  cur_opt_histogram = opt_histogram;

  do {
    uint32 tmp_large;
    uint16 tmp;
    curr_hist += *(cur_histogram++);
    tmp_large = ((uint32)curr_hist * 0xF0);
    tmp = tmp_large >> 6; /* /64 */
    tmp /= 75;            /* /64/75 = /80/60 */
    *(cur_opt_histogram++) = tmp;
  } while (x++ < 0xF0);


  return;
}
#ifdef __CC65__
#undef cur_histogram
#undef cur_opt_histogram
#undef cur_thumb_data
#endif

int8 bayer_map[64] = {
  -128,   0,  -96,  32,  -120,   8,  -88,  40,
    64, -64,   96, -32,   72,  -56,  104, -24,
   -80,  48, -112,  16,  -72,   56, -104,  24,
   112, -16,   80, -48,  120,   -8,   88, -40,
  -116,  12,  -84,  44, -124,    4,  -92,  36,
    76, -52,  108, -20,   68,  -60,  100, -28,
   -68,  60, -100,  28,  -76,   52, -108,  20,
   124,  -4,   92, -36,  116,  -12,   84, -44,
};

static void init_data (void)
{
  static uint8 init_done = 0;
  if (init_done) {
    return;
  }

  init_hgr_base_addrs();
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

  lx = centered_div7_table[dsx];
  rx = centered_div7_table[dex];

  /* Invert horizontal lines */
  a = (uint8 *)(hgr_baseaddr_l[dsy]|(hgr_baseaddr_h[dsy]<<8)) + lx;
  b = (uint8 *)(hgr_baseaddr_l[dey]|(hgr_baseaddr_h[dey]<<8)) + lx;
  for (x = dsx; x < dex; x+=7) {
    *a = ~(*a);
    *b = ~(*b);
    a++;
    b++;
  }
  /* Invert vertical lines */
  for (y = dsy + 1; y < dey - 1; y++) {
    uint8 *by = (uint8 *)(hgr_baseaddr_l[y]|(hgr_baseaddr_h[y]<<8));
    a = by + lx;
    b = by + rx;
    *a = ~(*a);
    *b = ~(*b);
  }
#ifdef __CC65__
  #undef y
  #undef a
  #undef b
#endif
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
        cputs("; C: Fit - Up/down: Adjust crop");
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
      if (!cropping) {
        switch(c) {
          case CH_ESC:
            clrscr();
            hgr_mixon();
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
          case CH_CURS_UP:
            if (crop_pos > 0) {
              crop_pos--;
              return 1;
            }
            break;
          case CH_CURS_DOWN:
            if (crop_pos < 2) {
              crop_pos++;
              return 1;
            }
            break;
          default:
            if (hgr_mix_is_on)
              hgr_mixoff();
            else
              hgr_mixon();
        }
      } else {
        /* will be divided by 2 if 320x240, we want it
         * to start on a band boundary */
        uint8 move_offset;
crop_again:
        move_offset = src_width == 640 ? BAND_HEIGHT : BAND_HEIGHT*2;
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
          default:
            hgr_mixon();
        }
        invert_selection();
        c = cgetc();
        goto crop_again;
      }
  } while (1);

save:
  strcpy((char *)buffer, ofname);
  if ((cp = strrchr ((char *)buffer, '.')))
    *cp = 0;
  strcat ((char *)buffer, ".hgr");
  hgr_mixon();
  cputs("Save to: ");
  dget_text_single((char *)buffer, 63, NULL);
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

uint16 file_width;
uint8 file_height;
uint8 is_thumb;
uint8 is_qt100;
uint8 first_byte_idx;
uint16 off_y;
uint8 off_x;
uint8 *shifted_div7_table = div7_table + 0;
uint8 *shifted_mod7_table = mod7_table + 0;
uint8 end_dx;
int8 bayer_map_x, bayer_map_y;
int8 end_bayer_map_x, end_bayer_map_y;
uint8 *cur_buf_page;
uint8 y;
uint16 dy;

uint8 prev_scaled_dx, prev_scaled_dy;
uint8 cur_hgr_row;
uint8 cur_hgr_mod;


#ifdef __CC65__
  #define thumb_buf_ptr zp2p /* Used for thumbnail decoding */
#endif

#if (BUFFER_SIZE != 2048)
#error Wrong BUFFER_SIZE, has to be 2048
#endif

#ifndef __CC65__
void load_normal_data(void) {
      if (y % 8) {
        cur_buf_page += FILE_WIDTH;
      } else {
        fread(buffer, 1, BUFFER_SIZE, ifp);
        cur_buf_page = buffer;
      }
}
#else
/* in qt-dither.s */
void load_normal_data(void);
#endif

void load_thumbnail_data(uint8 line) {
  uint8 a, b, c, d, dx, i;
  /* assume thumbnail at 4bpp and zoom it */
  if (is_qt100) {
    if (!(line & 1)) {
      fread(buffer, 1, THUMB_WIDTH / 2, ifp);
      /* Unpack */
#ifndef __CC65__
      uint8 off;
      i = 39;
      do {
        c   = buffer[i];
        a   = (c & 0xF0);
        b   = (c << 4);
        off = i * 4;
        buffer[off++] = a;
        buffer[off++] = a;
        buffer[off++] = b;
        buffer[off] = b;
      } while (i--);
#else
      __asm__("ldy #39");
      next_thumb_x:
      __asm__("lda (%v),y", thumb_buf_ptr); /* Load byte at index Y */
      __asm__("tax");       /* backup value */
      __asm__("asl");       /* low nibble, << 4 */
      __asm__("asl");
      __asm__("asl");
      __asm__("asl");
      __asm__("sta tmp1"); /* Store low nibble */
      __asm__("txa");      /* Restore value */

      __asm__("and #$F0"); /* high nibble */
      __asm__("tax");      /* Store high nibble */

      __asm__("tya");      /* *4 offset */
      __asm__("sty tmp2"); /* Backup index */

      __asm__("asl");
      __asm__("asl");
      __asm__("tay");

      __asm__("txa");      /* Store high nibble twice */
      __asm__("sta (%v),y", thumb_buf_ptr);
      __asm__("iny");
      __asm__("sta (%v),y", thumb_buf_ptr);

      __asm__("lda tmp1");/* Store low nibble twice */
      __asm__("iny");
      __asm__("sta (%v),y", thumb_buf_ptr);
      __asm__("iny");
      __asm__("sta (%v),y", thumb_buf_ptr);

      __asm__("ldy tmp2");  /* Restore index */
      __asm__("dey");
      __asm__("bpl %g", next_thumb_x);
#endif
    }
  } else {
    unsigned char *cur_in, *cur_out;
    unsigned char *orig_in, *orig_out;
    /* Whyyyyyy do they do that */
    if (!(line % 4)) {
      /* Expand the next two lines from 4bpp thumb_buf to 8bpp buffer */
      fread(thumb_buf, 1, THUMB_WIDTH, ifp);
      orig_in = cur_in = thumb_buf;
      orig_out = cur_out = buffer;
      for (dx = 0; dx < THUMB_WIDTH; dx++) {
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
      for (dx = 0; dx < THUMB_WIDTH; dx++) {
        *cur_out = *cur_in;
        cur_out++;
        *cur_out = *cur_in;
        cur_out++;
        cur_in++;
      }
    } else if (!(line % 2)) {
      /* Copy the second line of thumb_buf to buffer for display,
       * upscaling horizontally */
      orig_in = cur_in = thumb_buf + THUMB_WIDTH;
      orig_out = cur_out = buffer;
      for (dx = 0; dx < THUMB_WIDTH; dx++) {
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

#ifndef __CC65__
void setup_angle_0(void) {
        off_x = 0;
        off_y = (HGR_HEIGHT - file_height) / 2;
        shifted_div7_table = div7_table + x_offset;
        shifted_mod7_table = mod7_table + x_offset;
        cur_hgr_baseaddr_ptr = (hgr_baseaddr + off_y);
        first_byte_idx = *(shifted_div7_table);
        hgr_byte = *cur_hgr_baseaddr_ptr + first_byte_idx;

        xdir = +1;
        ydir = +1;
        invert_coords = 0;
}
void setup_angle_90(void) {
      off_x = 0;
      if (resize) {
        off_y = 212U * 4 / 3;
        end_dx = file_width;
      } else {
        off_y = HGR_WIDTH - 45;
        end_dx = HGR_HEIGHT;
      }

      xdir = +1;
      ydir = -1;
      invert_coords = 1;
}
void setup_angle_180(void) {
      off_x = file_width - 1;
      off_y = file_height - 1;
      end_dx = 0;

      shifted_div7_table = div7_table + x_offset;
      shifted_mod7_table = mod7_table + x_offset;

      cur_hgr_baseaddr_ptr = (uint16 *)(hgr_baseaddr + off_y);
      first_byte_idx = *(shifted_div7_table + off_x);
      hgr_byte = *cur_hgr_baseaddr_ptr + first_byte_idx;
      xdir = -1;
      ydir = -1;
      invert_coords = 0;
}
void setup_angle_270(void) {
      if (resize) {
        off_x = file_width - 1;
        off_y = 68U * 4 / 3;
        end_dx = 0;
      } else {
        off_x = HGR_HEIGHT - 1;
        off_y = 44;
        end_dx = file_width - 1;
      }
      xdir = -1;
      ydir = +1;
      invert_coords = 1;
}
#else
void setup_angle_0(void);
void setup_angle_90(void);
void setup_angle_180(void);
void setup_angle_270(void);
#endif

#ifndef __CC65__
uint8 **cur_hgr_baseaddr_ptr;
uint8 *buf_ptr;
uint8 *hgr_byte;
uint8 scaled_dx, scaled_dy;
uint8 dx;
uint8 pixel;
int16 buf_plus_err;
uint8 opt_val;
int8 err2;
int8 xdir = 1, ydir = 1;
int8 err1;
uint8 invert_coords = 0;
#endif

#ifndef __CC65__
void do_dither(void) {
  end_dx = file_width == 256 ? 0 : file_width;
  /* Init to safe value */
  prev_scaled_dx = prev_scaled_dy = 100;
  /* Setup offsets and directions */
  switch (angle) {
    case 0:
      setup_angle_0();
      break;
    case 90:
      setup_angle_90();
      break;
    case 180:
      setup_angle_180();
      break;
    case 270:
      setup_angle_270();
      break;
  }

  /* Line loop */
  bayer_map_y = 0;
  y = 0;
  dy = off_y;
  end_bayer_map_y = 64;
  cur_buf_page = buffer; /* Init once (in case of thumbnail) */


  next_y:
  //for (y = 0, dy = off_y; y != file_height;) {
    /* Load data from file */
    if (!is_thumb) {
      load_normal_data();
    } else {
      load_thumbnail_data(y);
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
      cur_hgr_row = shifted_div7_table[scaled_dy];
      cur_hgr_mod = shifted_mod7_table[scaled_dy];
    }

    //dither_line:
    buf_ptr = cur_buf_page;
    dx = off_x;

    pixel = *(shifted_mod7_table + off_x);

    if (dither_alg == DITHER_SIERRA) {
      goto prepare_dither_sierra;
    } else if (dither_alg == DITHER_NONE) {
      goto prepare_dither_none;
    } // else prepare_dither_bayer

    //prepare_dither_bayer:
      bayer_map_x = bayer_map_y + 0;
      end_bayer_map_x = bayer_map_x + 8;
      goto prepare_dither_none;

    prepare_dither_sierra:
    err2 = 0;

/* Column loop */
    prepare_dither_none:
    next_x:
      opt_val = opt_histogram[*buf_ptr];
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
        hgr_byte = hgr_baseaddr[scaled_dx] + cur_hgr_row;
        pixel = cur_hgr_mod;
      }

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
        goto dither_sierra;
      } else if (dither_alg == DITHER_NONE) {
        goto dither_none;
      } // else dither_bayer

      //dither_bayer:
        opt_val += bayer_map[bayer_map_x];

        if (opt_val < DITHER_THRESHOLD) {
          x86_64_tgi_set(dx, y, TGI_COLOR_BLACK);
        } else {
          *hgr_byte |= pixel;
          x86_64_tgi_set(dx, y, TGI_COLOR_WHITE);
        }

        /* Advance Bayer X */
        bayer_map_x++;
        if (bayer_map_x == end_bayer_map_x)
          bayer_map_x = bayer_map_y + 0;

        goto next_pixel;

      dither_none:
        if (opt_val < DITHER_THRESHOLD) {
          x86_64_tgi_set(dx, y, TGI_COLOR_BLACK);
        } else {
          *hgr_byte |= pixel;
          x86_64_tgi_set(dx, y, TGI_COLOR_WHITE);
        }
        goto next_pixel;

      dither_sierra:
        buf_plus_err = opt_val + err2;
        buf_plus_err += sierra_safe_err_buf[dx];
        if (buf_plus_err < DITHER_THRESHOLD) {
          /* pixel's already black */
          x86_64_tgi_set(dx, y, TGI_COLOR_BLACK);
        } else {
          *hgr_byte |= pixel;
          x86_64_tgi_set(dx, y, TGI_COLOR_WHITE);
        }
        err2 = ((int8)buf_plus_err) >> 1; /* cur_err * 2 / 4 */
        err1 = err2 >> 1;    /* cur_err * 1 / 4 */

        sierra_safe_err_buf[dx]   = err1;
        if (dx > 0) {
          sierra_safe_err_buf[dx-1]    += err1;
        }


next_pixel:
      if (xdir < 0) {
        dx--;
        pixel >>= 1;
        if (pixel == 0x00) {
          pixel = 0x40;
          hgr_byte--;
        }
      } else {
        dx++;
        pixel <<= 1;
        if (pixel == 0x80) {
          pixel = 0x01;
          hgr_byte++;
        }
      }
      buf_ptr++;

    if (dx != end_dx)
      goto next_x;

    if (y % 16 == 0) {
      progress_bar(-1, -1, scrw, y, file_height);
      if (kbhit()) {
        if (cgetc() == CH_ESC)
          return;
      }
    }

    if (dither_alg == DITHER_BAYER) {
      /* Advance Bayer Y */
      bayer_map_y += 8;
      if (bayer_map_y == end_bayer_map_y) {
        bayer_map_y = 0;
      }
    }

next_line:
    if (ydir < 0) {
      cur_hgr_baseaddr_ptr--;
      dy--;
    } else {
      cur_hgr_baseaddr_ptr++;
      dy++;
    }
    hgr_byte = *cur_hgr_baseaddr_ptr + first_byte_idx;

    y++;
    if (y != file_height) {
      goto next_y;
    }
}
#else
void do_dither(void);
#endif

void dither_to_hgr(const char *ifname, const char *ofname, uint16 p_width, uint16 p_height, uint8 serial_model) {
  is_thumb = (p_width == THUMB_WIDTH*2);
  is_qt100 = (serial_model == QT_MODEL_100);

  file_width = p_width;
  file_height = p_height;
  x_offset = ((HGR_WIDTH - file_width) / 2);

  init_data();

  clrscr();
  printf("Converting %s (Esc to stop)...\n", ofname);

  ifp = fopen(ifname, "r");
  if (ifp == NULL) {
    printf("Can't open %s\n", ifname);
    return;
  }

  bzero(err_buf, sizeof err_buf);

  if (is_thumb) {
    thumb_histogram(ifp);
    /* Re-zero */
    bzero(err_buf, sizeof err_buf);
    rewind(ifp);
    dither_alg = DITHER_BAYER;
  }

  hgr_mixon();
  cputs("Rendering... (Press space to toggle menu once done.)\r\n");
  bzero((char *)HGR_PAGE, HGR_LEN);

  progress_bar(wherex(), wherey(), scrw, 0, file_height);

  do_dither();
  progress_bar(-1, -1, scrw, file_height, file_height);
  if (!is_thumb) {
    hgr_mixoff();
  }

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
    dither_to_hgr(TMP_NAME, ofname, FILE_WIDTH, FILE_HEIGHT, QT_MODEL_UNKNOWN);
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
