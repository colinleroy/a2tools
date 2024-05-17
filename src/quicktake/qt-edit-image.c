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

#define DITHER_NONE   2
#define DITHER_BAYER  1
#define DITHER_SIERRA 0
#define DITHER_THRESHOLD 128U /* Must be 128 for sierra dithering sign check */
#define DEFAULT_BRIGHTEN 0

#define X_OFFSET ((HGR_WIDTH - file_width) / 2)

static int8 err_buf[FILE_WIDTH * 2];

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

#ifndef __CC65__
static uint16 histogram[256];
#else
static uint8 histogram_low[256];
static uint8 histogram_high[256];
#endif

static uint8 opt_histogram[256];
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
    __asm__("phy");
    __asm__("jsr pushax");
    __asm__("lda #<%w", HGR_HEIGHT);
    __asm__("jsr tosudiva0");
    __asm__("ply");

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

void dither_to_hgr(const char *ifname, const char *ofname, uint16 p_width, uint16 p_height, uint8 serial_model) {
  /* Rotation/cropping variables */
  uint8 i;
  uint8 y;

  uint8 off_x;
  uint16 off_y;
  uint16 file_width;
  uint8 *cur_buf_page;

  uint8 *shifted_div7_table = div7_table + 0;
  uint8 *shifted_mod7_table = mod7_table + 0;

  uint8 end_dx;
  uint8 d7;
  uint8 prev_scaled_dx, prev_scaled_dy;

#ifdef __CC65__
  #define cur_hgr_baseaddr_ptr zp2ip
  #define thumb_buf_ptr zp4p /* Used for thumbnail decoding */
  #define pixel zp6s
  #define opt_val zp7
  #define dx zp8
  #define err2 zp9
  #define dy zp10i
  #define ptr zp12p
#else
  uint8 **cur_hgr_baseaddr_ptr;
  uint8 *cur_hgr_baseaddr_val; /* shortcut ptr */
  uint8 *buf_ptr;
  uint8 *ptr;
  uint8 scaled_dx, scaled_dy;
  uint8 dx;
  uint8 pixel;
  int16 buf_plus_err;
  uint8 opt_val;
  int8 err2;
  uint16 dy;
  int8 xdir = 1, ydir = 1;
  int8 err1;
  uint8 invert_coords = 0;
#endif

  /* Used for both Sierra and Bayer */
  register int8 *regptr1, *regptr2, *regptr3;

  /* Sierra variables */
  #define sierra_err regptr1

  /* Bayer variables */
  #define bayer_map_x regptr1
  #define bayer_map_y regptr2
  #define end_bayer_map_x regptr3
  int8 *end_bayer_map_y;

  uint8 file_height;

  /* General variables */
  uint8 cur_hgr_row;
  uint8 cur_hgr_mod;
  uint8 is_thumb = (p_width == THUMB_WIDTH*2);
  uint8 is_qt100 = (serial_model == QT_MODEL_100);

  file_width = p_width;
  file_height = p_height;

  sierra_err = err_buf;

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

  cputs("Dithering...\r\n");
  bzero((char *)HGR_PAGE, HGR_LEN);

  progress_bar(wherex(), wherey(), scrw, 0, file_height);

  end_dx = file_width == 256 ? 0 : file_width;

  /* Init to safe value */
  prev_scaled_dx = prev_scaled_dy = 100;

  /* Setup offsets and directions */
  switch (angle) {
    case 0:
      off_x = 0;
      off_y = (HGR_HEIGHT - file_height) / 2;
      shifted_div7_table = div7_table + X_OFFSET;
      shifted_mod7_table = mod7_table + X_OFFSET;
#ifndef __CC65__
      cur_hgr_baseaddr_ptr = (hgr_baseaddr + off_y);
      d7 = *(shifted_div7_table + off_x);
      cur_hgr_baseaddr_val = *cur_hgr_baseaddr_ptr + d7;

      xdir = +1;
      ydir = +1;
      invert_coords = 0;
#else
      cur_hgr_baseaddr_ptr = (uint16 *)(hgr_baseaddr + off_y);
      __asm__("clc");
      __asm__("lda %v", off_x);
      __asm__("adc %v", shifted_div7_table);
      __asm__("sta ptr1");
      __asm__("lda #0");
      __asm__("adc %v+1", shifted_div7_table);
      __asm__("sta ptr1+1");

      __asm__("lda (ptr1)");
      __asm__("sta %v", d7);
      __asm__("adc (%v)", cur_hgr_baseaddr_ptr);
      __asm__("sta %v", ptr);
      __asm__("ldy #1");
      __asm__("lda (%v),y", cur_hgr_baseaddr_ptr);
      __asm__("adc #0");
      __asm__("sta %v+1", ptr);

      /* Patch Sierra error forwarding */
      __asm__("lda #$88"); //dey
      __asm__("sta %g", sierra_err_forward);

      /* Patch xcounters direction */
      __asm__("lda #<(%g)", inc_xcounters);
      __asm__("ldx #>(%g)", inc_xcounters);
      __asm__("sta %g+1", xcounters_dir);
      __asm__("stx %g+2", xcounters_dir);

      /* Patch ycounters direction */
      __asm__("lda #<(%g)", inc_ycounters);
      __asm__("ldx #>(%g)", inc_ycounters);
      __asm__("sta %g+1", ycounters_dir);
      __asm__("stx %g+2", ycounters_dir);

      /* Patch line coords inversion jmp */
      __asm__("lda #<(%g)", dither_line);
      __asm__("ldx #>(%g)", dither_line);
      __asm__("sta %g+1", compute_line_coords);
      __asm__("stx %g+2", compute_line_coords);

      /* Patch pixel coords inversion jmp */
      __asm__("lda #<(%g)", dither_pixel);
      __asm__("ldx #>(%g)", dither_pixel);
      __asm__("sta %g+1", compute_pixel_coords_a);
      __asm__("stx %g+2", compute_pixel_coords_a);
      /* There are more, which we'll do later */

#endif
      break;
    case 90:
      off_x = 0;
      if (resize) {
        off_y = 212U * 4 / 3;
        end_dx = file_width;
      } else {
        off_y = HGR_WIDTH - 45;
        end_dx = HGR_HEIGHT;
      }
#ifndef __CC65__
      xdir = +1;
      ydir = -1;
      invert_coords = 1;
#else
      /* Patch Sierra error forwarding */
      __asm__("lda #$88"); //dey
      __asm__("sta %g", sierra_err_forward);

      /* Patch xcounters direction */
      __asm__("lda #<(%g)", inc_xcounters);
      __asm__("ldx #>(%g)", inc_xcounters);
      __asm__("sta %g+1", xcounters_dir);
      __asm__("stx %g+2", xcounters_dir);

      /* Patch ycounters direction */
      __asm__("lda #<(%g)", dec_ycounters);
      __asm__("ldx #>(%g)", dec_ycounters);
      __asm__("sta %g+1", ycounters_dir);
      __asm__("stx %g+2", ycounters_dir);

      /* Patch line coords inversion jmp */
      __asm__("lda #<(%g)", invert_line_coords);
      __asm__("ldx #>(%g)", invert_line_coords);
      __asm__("sta %g+1", compute_line_coords);
      __asm__("stx %g+2", compute_line_coords);

      /* Patch pixel coords inversion jmp */
      __asm__("lda #<(%g)", invert_pixel_coords);
      __asm__("ldx #>(%g)", invert_pixel_coords);
      __asm__("sta %g+1", compute_pixel_coords_a);
      __asm__("stx %g+2", compute_pixel_coords_a);

#endif
      break;
    case 270:
      if (resize) {
        off_x = file_width - 1;
        off_y = 68U * 4 / 3;
        end_dx = 0;
      } else {
        off_x = HGR_HEIGHT - 1;
        off_y = 44;
        end_dx = file_width - 1;
      }
#ifndef __CC65__
      xdir = -1;
      ydir = +1;
      invert_coords = 1;
#else
      /* Patch Sierra error forwarding */
      __asm__("lda #$C8"); //iny
      __asm__("sta %g", sierra_err_forward);

      /* Patch xcounters direction */
      __asm__("lda #<(%g)", dec_xcounters);
      __asm__("ldx #>(%g)", dec_xcounters);
      __asm__("sta %g+1", xcounters_dir);
      __asm__("stx %g+2", xcounters_dir);

      /* Patch ycounters direction */
      __asm__("lda #<(%g)", inc_ycounters);
      __asm__("ldx #>(%g)", inc_ycounters);
      __asm__("sta %g+1", ycounters_dir);
      __asm__("stx %g+2", ycounters_dir);

      /* Patch line coords inversion jmp */
      __asm__("lda #<(%g)", invert_line_coords);
      __asm__("ldx #>(%g)", invert_line_coords);
      __asm__("sta %g+1", compute_line_coords);
      __asm__("stx %g+2", compute_line_coords);

      /* Patch pixel coords inversion jmp */
      __asm__("lda #<(%g)", invert_pixel_coords);
      __asm__("ldx #>(%g)", invert_pixel_coords);
      __asm__("sta %g+1", compute_pixel_coords_a);
      __asm__("stx %g+2", compute_pixel_coords_a);

#endif
      break;
    case 180:
      off_x = file_width - 1;
      off_y = file_height - 1;
      end_dx = 0;

      shifted_div7_table = div7_table + X_OFFSET;
      shifted_mod7_table = mod7_table + X_OFFSET;

#ifndef __CC65__
      cur_hgr_baseaddr_ptr = (hgr_baseaddr + off_y);
      d7 = *(shifted_div7_table + off_x);
      cur_hgr_baseaddr_val = *cur_hgr_baseaddr_ptr + d7;
      xdir = -1;
      ydir = -1;
      invert_coords = 0;
#else
      cur_hgr_baseaddr_ptr = (uint16 *)(hgr_baseaddr + off_y);
      __asm__("clc");
      __asm__("lda %v", off_x);
      __asm__("adc %v", shifted_div7_table);
      __asm__("sta ptr1");
      __asm__("lda #0");
      __asm__("adc %v+1", shifted_div7_table);
      __asm__("sta ptr1+1");

      __asm__("lda (ptr1)");
      __asm__("sta %v", d7);
      __asm__("adc (%v)", cur_hgr_baseaddr_ptr);
      __asm__("sta %v", ptr);
      __asm__("ldy #1");
      __asm__("lda (%v),y", cur_hgr_baseaddr_ptr);
      __asm__("adc #0");
      __asm__("sta %v+1", ptr);

      /* Patch Sierra error forwarding */
      __asm__("lda #$C8"); //iny
      __asm__("sta %g", sierra_err_forward);

      /* Patch xcounters direction */
      __asm__("lda #<(%g)", dec_xcounters);
      __asm__("ldx #>(%g)", dec_xcounters);
      __asm__("sta %g+1", xcounters_dir);
      __asm__("stx %g+2", xcounters_dir);

      /* Patch ycounters direction */
      __asm__("lda #<(%g)", dec_ycounters);
      __asm__("ldx #>(%g)", dec_ycounters);
      __asm__("sta %g+1", ycounters_dir);
      __asm__("stx %g+2", ycounters_dir);

      /* Patch line coords inversion jmp */
      __asm__("lda #<(%g)", dither_line);
      __asm__("ldx #>(%g)", dither_line);
      __asm__("sta %g+1", compute_line_coords);
      __asm__("stx %g+2", compute_line_coords);

      /* Patch pixel coords inversion jmp */
      __asm__("lda #<(%g)", dither_pixel);
      __asm__("ldx #>(%g)", dither_pixel);
      __asm__("sta %g+1", compute_pixel_coords_a);
      __asm__("stx %g+2", compute_pixel_coords_a);

#endif
      break;
  }

#ifdef __CC65__
  /* Finish patching compute_pixel_coords branches */
  __asm__("lda %g+1", compute_pixel_coords_a);
  __asm__("ldx %g+2", compute_pixel_coords_a);
  __asm__("sta %g+1", compute_pixel_coords_b);
  __asm__("stx %g+2", compute_pixel_coords_b);
  __asm__("sta %g+1", compute_pixel_coords_c);
  __asm__("stx %g+2", compute_pixel_coords_c);
  __asm__("sta %g+1", compute_pixel_coords_d);
  __asm__("stx %g+2", compute_pixel_coords_d);

  /* Patch dither branches */
  __asm__("lda #<(%g)", next_line);
  __asm__("ldx #>(%g)", next_line);
  __asm__("sta %g+1", line_wrapup);
  __asm__("stx %g+2", line_wrapup);

  __asm__("lda %v", dither_alg);
  __asm__("beq %g", set_dither_sierra);
  __asm__("cmp #%b", DITHER_NONE);
  __asm__("beq %g", set_dither_none);

  //set_dither_bayer:
  __asm__("lda #<(%g)", prepare_dither_bayer);
  __asm__("ldx #>(%g)", prepare_dither_bayer);
  __asm__("sta %g+1", prepare_dither);
  __asm__("stx %g+2", prepare_dither);

  __asm__("lda #<(%g)", advance_bayer_y);
  __asm__("ldx #>(%g)", advance_bayer_y);
  __asm__("sta %g+1", line_wrapup);
  __asm__("stx %g+2", line_wrapup);

  __asm__("lda #<(%g)", dither_bayer);
  __asm__("ldx #>(%g)", dither_bayer);
  __asm__("bra %g", dither_is_set);
  set_dither_none:
  __asm__("lda #<(%g)", prepare_dither_none);
  __asm__("ldx #>(%g)", prepare_dither_none);
  __asm__("sta %g+1", prepare_dither);
  __asm__("stx %g+2", prepare_dither);

  __asm__("lda #<(%g)", dither_none);
  __asm__("ldx #>(%g)", dither_none);
  __asm__("bra %g", dither_is_set);
  set_dither_sierra:
  __asm__("lda #<(%g)", prepare_dither_sierra);
  __asm__("ldx #>(%g)", prepare_dither_sierra);
  __asm__("sta %g+1", prepare_dither);
  __asm__("stx %g+2", prepare_dither);

  __asm__("lda #<(%g)", dither_sierra);
  __asm__("ldx #>(%g)", dither_sierra);

  dither_is_set:
  __asm__("sta %g+1", do_dither);
  __asm__("stx %g+2", do_dither);

  __asm__("ldy %v", brighten);
  __asm__("beq %g", skip_brighten);
  __asm__("lda #<(%g)", do_brighten);
  __asm__("ldx #>(%g)", do_brighten);
  skip_brighten:
  __asm__("sta %g+1", brighten_branch);
  __asm__("stx %g+2", brighten_branch);

  /* Patch shifted mod7 table */
  __asm__("lda %v", shifted_mod7_table);
  __asm__("sta %g+1", shifted_mod7_load);
  __asm__("lda %v+1", shifted_mod7_table);
  __asm__("sta %g+2", shifted_mod7_load);

  /* Patch Y loop bound */
  __asm__("lda %v", file_height);
  __asm__("sta %g+1", y_loop_bound);

  /* Patch X loop bounds */
  __asm__("lda %v", end_dx);
  __asm__("sta %g+1", x_loop_bound_a);
  __asm__("sta %g+1", x_loop_bound_b);

#endif
  /* Line loop */
  bayer_map_y = bayer_map + 0;
  end_bayer_map_y = bayer_map_y + 64;

#ifndef __CC65__
  cur_buf_page = buffer; /* Init once (in case of thumbnail) */
#else
  __asm__("ldx #>(%v)", buffer);
  __asm__("stx %v+1", cur_buf_page);
  __asm__("stx %v+1", thumb_buf_ptr);
  __asm__("lda #<(%v)", buffer);
  __asm__("sta %v", cur_buf_page);
  __asm__("sta %v", thumb_buf_ptr);
#endif
  y = 0;
  dy = off_y;

  next_y:
  //for (y = 0, dy = off_y; y != file_height;) {

    /* Load data from file */
    if (!is_thumb) {

#if (BLOCK_SIZE*2 != 1024)
#error Wrong BLOCK_SIZE, has to be 2*256
#endif

#ifndef __CC65__
      if (y % 4) {
        cur_buf_page += FILE_WIDTH;
      } else {
        fread(buffer, 1, BLOCK_SIZE*2, ifp);
        cur_buf_page = buffer;
      }
#else
      __asm__("lda %v", y);
      __asm__("and #3");
      __asm__("beq %g", read_buf);
      __asm__("inc %v+1", cur_buf_page);
      __asm__("jmp %g", prepare_line);
      read_buf:
      fread(buffer, 1, BLOCK_SIZE*2, ifp);
      __asm__("lda #>(%v)", buffer);
      __asm__("sta %v+1", cur_buf_page);
#endif
    } else {
      uint8 a, b, c, d;
      /* assume thumbnail at 4bpp and zoom it */
      if (is_qt100) {
        if (!(y & 1)) {
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
          __asm__("phy");

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

          __asm__("ply");     /* Restore index */
          __asm__("dey");
          __asm__("bpl %g", next_thumb_x);
#endif
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
        } else if (!(y % 2)) {
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
#else

    prepare_line:
    __asm__("ldx %v+1", cur_buf_page);
    __asm__("stx %g+2", buf_ptr_load);
    __asm__("lda %v", cur_buf_page);
    __asm__("sta %g+1", buf_ptr_load);

    compute_line_coords:
    __asm__("jmp $FFFF"); /* PATCHED, will either go to invert_line_coords or dither_line */
                          /* The cycle benefit is very little, but at least it works the  */
                          /* same as for the pixel coords inversion. */

    invert_line_coords:
    /* Calculate hgr base coordinates for the line */
    __asm__("ldy %v", resize);
    __asm__("beq %g", no_resize);

    //scaled_dy = (dy + (dy << 1)) >> 2;  /* *3/4 */
    __asm__("lda %v", dy);
    __asm__("ldx %v+1", dy);
    __asm__("stx tmp1");
    __asm__("asl a");
    __asm__("rol tmp1");

    __asm__("adc %v", dy);
    __asm__("tay"); /* backup low byte */
    __asm__("lda %v+1", dy);
    __asm__("adc tmp1");
    __asm__("lsr a");
    __asm__("sta tmp1");

    __asm__("tya");
    __asm__("ror a");
    __asm__("lsr tmp1");
    __asm__("ror a");

    __asm__("cmp %v", prev_scaled_dy);
    __asm__("beq %g", next_line);

    __asm__("sta %v", prev_scaled_dy);

    __asm__("tay"); /* scaled_dy not > 255 here */
    __asm__("bra %g", set_inv_hgr_ptrs);

    no_resize:
    __asm__("ldy %v", dy); /* dy not > 255 here */
    set_inv_hgr_ptrs:
    __asm__("lda %v,y", div7_table); /* Not shifted, there* */
    __asm__("sta %v", cur_hgr_row);
    __asm__("lda %v,y", mod7_table);
    __asm__("sta %v", cur_hgr_mod);

    dither_line:
    __asm__("ldy %v", off_x);
    __asm__("sty %v", dx);

    shifted_mod7_load:
    __asm__("lda $FFFF,y"); /* Patched at start with *(shifted_mod7_table) */
    __asm__("sta %v", pixel);
#endif

#ifndef __CC65__
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
#else
    prepare_dither:
    __asm__("jmp $FFFF"); /* PATCHED to prepare_dither_{bayer,sierra,none} */

    prepare_dither_bayer:
      __asm__("lda %v", bayer_map_y);
      __asm__("sta %v", bayer_map_x);
      __asm__("ldx %v+1", bayer_map_y);
      __asm__("stx %v+1", bayer_map_x);

      __asm__("clc");
      __asm__("adc #8");
      __asm__("bcc %g", noof13);
      __asm__("inx");
      noof13:
      __asm__("sta %v", end_bayer_map_x);
      __asm__("stx %v+1", end_bayer_map_x);
      compute_pixel_coords_a:
      __asm__("jmp $FFFF");

    prepare_dither_sierra:
      /* reset err2 */
      __asm__("stz %v", err2);
#endif

    prepare_dither_none:

    /* Column loop */
#ifndef __CC65__
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
        ptr = hgr_baseaddr[scaled_dx] + cur_hgr_row;
        pixel = cur_hgr_mod;
      }
#else
      /* Get destination pixel */
      compute_pixel_coords_b:
      __asm__("jmp $FFFF"); /* PATCHED, will either go to invert_pixel_coords or dither_pixel */

      invert_pixel_coords:
      __asm__("ldy %v", resize);
      __asm__("beq %g", no_resize_pixel);

      //scaled_dx = (dx + (dx << 1)) >> 2;  /* *3/4 */
      __asm__("lda %v", dx);
      __asm__("stz tmp1");
      __asm__("asl a");
      __asm__("rol tmp1");

      __asm__("adc %v", dx);
      __asm__("tay"); /* backup low byte */

      __asm__("lda #0");
      __asm__("adc tmp1");
      __asm__("lsr a");
      __asm__("sta tmp1");

      __asm__("tya");
      __asm__("ror a");
      __asm__("lsr tmp1");
      __asm__("ror a");

      __asm__("cmp %v", prev_scaled_dx);
      __asm__("beq %g", next_pixel);

      __asm__("sta %v", prev_scaled_dx);

      __asm__("bra %g", inv_pixel_coords);

      no_resize_pixel:
      __asm__("lda %v", dx);
      inv_pixel_coords:

      //ptr = hgr_baseaddr[scaled_dx] + cur_hgr_row;
      __asm__("ldx #>(%v)", hgr_baseaddr);
      __asm__("asl a");
      __asm__("tay");
      __asm__("bcc %g", noof21);
      __asm__("inx");
      __asm__("clc");
      noof21:
      __asm__("lda #<(%v)", hgr_baseaddr);
      __asm__("stx ptr1+1");
      __asm__("sta ptr1");

      __asm__("lda (ptr1),y");
      __asm__("adc %v", cur_hgr_row);
      __asm__("sta %v", ptr);

      __asm__("iny");
      __asm__("lda (ptr1),y");
      __asm__("adc #0");
      __asm__("sta %v+1", ptr);

      //pixel = cur_hgr_mod;
      __asm__("lda %v", cur_hgr_mod);
      __asm__("sta %v", pixel);

      dither_pixel:
#endif

#ifndef __CC65__
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
        opt_val += *bayer_map_x;

        if (opt_val < DITHER_THRESHOLD) {
          x86_64_tgi_set(dx, y, TGI_COLOR_BLACK);
        } else {
          *ptr |= pixel;
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
          *ptr |= pixel;
          x86_64_tgi_set(dx, y, TGI_COLOR_WHITE);
        }
        goto next_pixel;

      dither_sierra:
        buf_plus_err = opt_val + err2;
        buf_plus_err += *(sierra_err+dx);
        if (buf_plus_err < DITHER_THRESHOLD) {
          /* pixel's already black */
          x86_64_tgi_set(dx, y, TGI_COLOR_BLACK);
        } else {
          *ptr |= pixel;
          x86_64_tgi_set(dx, y, TGI_COLOR_WHITE);
        }
        err2 = ((int8)buf_plus_err) >> 1; /* cur_err * 2 / 4 */
        err1 = err2 >> 1;    /* cur_err * 1 / 4 */

        *(sierra_err+dx)   = err1;
        if (dx > 0) {
          *(sierra_err+dx-1)    += err1;
        }

#else
      //opt_val = opt_histogram[*buf_ptr];
      buf_ptr_load:
      __asm__("ldy $FFFF"); /* Patched by buf_ptr address */
      __asm__("lda %v,y",   opt_histogram);
      __asm__("sta %v",     opt_val);

      //if (brighten) {
      brighten_branch:
      __asm__("jmp $FFFF"); /* Patched with do_brighten of dither_* */
      // __asm__("ldy %v", brighten);
      // __asm__("bne %g", do_brighten);

      do_brighten:
      __asm__("ldx #0");
      __asm__("lda %v", brighten);
      __asm__("bpl %g", brighten_pos);
      __asm__("dex");
      brighten_pos:
      __asm__("adc %v", opt_val);
      __asm__("bcc %g", noof18);
      __asm__("inx");
      __asm__("clc");
      noof18:
      __asm__("cpx #0");
      __asm__("beq %g", store_opt);
      __asm__("bpl %g", pos_opt);
      __asm__("lda #0");
      __asm__("bra %g", store_opt);
      pos_opt:
      __asm__("lda #$FF");
      store_opt:
      __asm__("sta %v", opt_val);

      do_dither:
      __asm__("jmp $FFFF"); /* PATCHED, will go to dither_{bayer,sierra,none} */

      dither_bayer:
      __asm__("ldx #$00");
      __asm__("lda (%v)", bayer_map_x);
      __asm__("bpl %g", positive_b);
      __asm__("dex");
      positive_b:
      __asm__("adc %v", opt_val);
      __asm__("tay"); /* Backup low byte */
      __asm__("txa");
      __asm__("adc #0");

      __asm__("bmi %g", black_pix_bayer);
      __asm__("bne %g", white_pix_bayer);
      __asm__("cpy #<(%b)", DITHER_THRESHOLD);
      __asm__("bcc %g", black_pix_bayer);
      white_pix_bayer:
      __asm__("lda (%v)", ptr);
      __asm__("ora %v", pixel);
      __asm__("sta (%v)", ptr);

      black_pix_bayer:
      /* Advance Bayer X */
      __asm__("lda %v", bayer_map_x);
      __asm__("inc a");
      __asm__("bne %g", noof5);
      __asm__("inc %v+1", bayer_map_x);
      noof5:
      __asm__("cmp %v", end_bayer_map_x);
      __asm__("bne %g", bayer_map_x_done); /* No need to check high byte */

      __asm__("lda %v", bayer_map_y);
      __asm__("ldx %v+1", bayer_map_y);
      __asm__("stx %v+1", bayer_map_x);
      bayer_map_x_done:
      __asm__("sta %v", bayer_map_x);

      __asm__("bra %g", next_pixel);

      dither_none:
      __asm__("lda %v", opt_val);
      __asm__("cmp #<(%b)", DITHER_THRESHOLD);
      __asm__("bcc %g", next_pixel);

      __asm__("lda (%v)", ptr);
      __asm__("ora %v", pixel);
      __asm__("sta (%v)", ptr);

      __asm__("bra %g", next_pixel);

      dither_sierra:
      /* Add the two errors (the one from previous pixel and the one
        from previous line). As they're max 128/2 and 128/4, don't bother
        about overflows. */
      __asm__("ldx #$00");
      __asm__("ldy %v", dx);
      __asm__("lda (%v),y", sierra_err);
      __asm__("adc %v", err2);
      __asm__("bpl %g", err_pos);
      __asm__("dex");

      err_pos:
      /* Add current pixel value */
      __asm__("clc");
      __asm__("adc %v", opt_val);
      __asm__("tay"); /* Backup low byte */
      __asm__("bcc %g", noof20);
      __asm__("inx");
      /* High byte not zero? - don't check for neg here, can't happen, it's either 0 or 1 */
      __asm__("bne %g", white_pix);

      noof20:
      /* is high byte negative? (don't check for positive here, can't happen, it's either $FF or 0)*/
      __asm__("cpx #0");
      __asm__("bmi %g", forward_err);

      /* Must check low byte */
      __asm__("cmp #<(%b)", DITHER_THRESHOLD);
      __asm__("bcc %g", forward_err_direct);
      white_pix:
      __asm__("lda (%v)", ptr);
      __asm__("ora %v", pixel);
      __asm__("sta (%v)", ptr);

      forward_err:
      __asm__("tya");       /* Restore low byte */
      __asm__("cmp #$80");  /* Keep sign for >> (no need to do it where coming from low byte check path, DITHER_THRESHOLD is $80)*/
      forward_err_direct:
      __asm__("ror a");
      __asm__("sta %v", err2);
      __asm__("cmp #$80");
      __asm__("ror a");

      // *(cur_err_x_yplus1+dx)   = err1;
      __asm__("ldy %v", dx);
      __asm__("sta (%v),y", sierra_err);

      // if (dx > 0) {
      //   *(cur_err_x_yplus1+dx-1)    += err1;
      // }

      __asm__("beq %g", next_pixel); /* Is dx == 0 */
      sierra_err_forward:
      __asm__("dey"); /* Patched with iny at setup, if xdir < 0 */
      __asm__("clc"); /* May be set by ror */
      __asm__("adc (%v),y", sierra_err);
      __asm__("sta (%v),y", sierra_err);
#endif

next_pixel:
#ifndef __CC65__
      if (xdir < 0) {
        dx--;
        pixel >>= 1;
        if (pixel == 0x00) {
          pixel = 0x40;
          ptr--;
        }
      } else {
        dx++;
        pixel <<= 1;
        if (pixel == 0x80) {
          pixel = 0x01;
          ptr++;
        }
      }
      buf_ptr++;

    if (dx != end_dx)
      goto next_x;

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

#else
      __asm__("lda %v", dx); /* Between next pixel and xcounters_dir to avoid optimizer optimizing branches */
      xcounters_dir:
      __asm__("jmp $FFFF"); /* PATCHED, to {dec,inc}_xcounters */

      dec_xcounters:
      // if (--dx = end_dx)
      //   goto line_done;
      __asm__("dec a");
      x_loop_bound_a:
      __asm__("cmp #$FF"); /* PATCHED with end_dx */
      __asm__("beq %g", line_done);
      __asm__("sta %v", dx);

      __asm__("lsr %v", pixel); /* Update pixel mask */
      __asm__("bne %g", inc_buf_ptr);
      __asm__("lda #$40");      /* We did seven pixels */
      __asm__("sta %v", pixel);

      __asm__("lda %v", ptr);   /* We did seven pixels, update ptr to next HGR byte */
      __asm__("bne %g", dec_base);
      __asm__("dec %v+1", ptr);
      dec_base:
      __asm__("dec %v", ptr);
      __asm__("bra %g", inc_buf_ptr);

      inc_xcounters:
      // if (++dx = end_dx)
      //   goto line_done;
      __asm__("inc a");
      x_loop_bound_b:
      __asm__("cmp #$FF"); /* PATCHED with end_dx */
      __asm__("beq %g", line_done);
      __asm__("sta %v", dx);

      __asm__("asl %v", pixel);
      __asm__("bpl %g", inc_buf_ptr); /* Are we on bit eight ? */

      __asm__("lda #1");        /* Yes - We did seven pixels */
      __asm__("sta %v", pixel);
      __asm__("inc %v", ptr);   /* Update HGR byte pointer */
      __asm__("bne %g", inc_buf_ptr);
      __asm__("inc %v+1", ptr);

      inc_buf_ptr:
      __asm__("inc %g+1", buf_ptr_load);
      __asm__("beq %g", inc_buf_ptr_high);
      compute_pixel_coords_c:
      __asm__("jmp $FFFF");

      inc_buf_ptr_high:
      __asm__("inc %g+2", buf_ptr_load);
      compute_pixel_coords_d:
      __asm__("jmp $FFFF");

      line_done:
      __asm__("lda %v", y);
      __asm__("and #15");

      __asm__("bne %g", line_wrapup);

      progress_bar(-1, -1, scrw, y, file_height);
      if (kbhit()) {
        if (cgetc() == CH_ESC)
          goto stop;
      }

      line_wrapup:
      __asm__("jmp $FFFF"); /* PATCHED with either advance_bayer_y or next_line */

      advance_bayer_y:
      /* Advance Bayer Y */
      __asm__("lda %v", bayer_map_y);
      __asm__("ldx %v+1", bayer_map_y);
      __asm__("clc");
      __asm__("adc #8");
      __asm__("bcc %g", noof14);
      __asm__("inx");
      noof14:
      __asm__("cmp %v", end_bayer_map_y);
      __asm__("bne %g", store_bayer_y);
      __asm__("cpx %v+1", end_bayer_map_y);
      __asm__("bne %g", store_bayer_y);

      __asm__("lda #<(%v)", bayer_map);
      __asm__("ldx #>(%v)", bayer_map);

      store_bayer_y:
      __asm__("sta %v", bayer_map_y);
      __asm__("stx %v+1", bayer_map_y);

#endif

next_line:
#ifndef __CC65__
    if (ydir < 0) {
      cur_hgr_baseaddr_ptr--;
      dy--;
    } else {
      cur_hgr_baseaddr_ptr++;
      dy++;
    }
    ptr = *cur_hgr_baseaddr_ptr + d7;

    y++;
    if (y != file_height) {
      goto next_y;
    }
#else
    __asm__("lda %v", cur_hgr_baseaddr_ptr);
    ycounters_dir:
    __asm__("jmp $FFFF"); /* PATCHED, to {dec,inc}_ycounters */

    dec_ycounters:
      __asm__("sec");
      __asm__("sbc #2");
      __asm__("sta %v", cur_hgr_baseaddr_ptr);
      __asm__("bcs %g", nouf1);
      __asm__("dec %v+1", cur_hgr_baseaddr_ptr);
      nouf1:
      dy--;
      __asm__("bra %g", finish_inc);

    inc_ycounters:
      __asm__("clc");
      __asm__("adc #2");
      __asm__("sta %v", cur_hgr_baseaddr_ptr);
      __asm__("bcc %g", noof19);
      __asm__("inc %v+1", cur_hgr_baseaddr_ptr);
      noof19:
      dy++;

    finish_inc:
    __asm__("lda (%v)", cur_hgr_baseaddr_ptr);
    __asm__("clc");
    __asm__("adc %v", d7);
    __asm__("sta %v", ptr);
    __asm__("ldy #1");
    __asm__("lda (%v),y", cur_hgr_baseaddr_ptr);
    __asm__("adc #0");
    __asm__("sta %v+1", ptr);

    __asm__("inc %v", y);
    __asm__("lda %v", y);
    y_loop_bound:
    __asm__("cmp #$FF"); /* PATCHED with file_height */
    __asm__("bne %g", next_y);
#endif

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
