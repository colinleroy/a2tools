#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dgets.h"
#include "dputc.h"
#include "dputs.h"
#include "extended_conio.h"
#include "file_select.h"
#include "hgr.h"
#include "path_helper.h"
#include "progress_bar.h"
#include "scrollwindow.h"

#include "qt-conv.h"

extern uint8 scrw, scrh;

#ifdef __CC65__
  #pragma static-locals(push, on)
#endif

#define BUF_SIZE 64

#define DITHER_NONE   0
#define DITHER_BURKES 1
#define DITHER_BAYER  2
#define DEFAULT_DITHER_THRESHOLD 128
#define DEFAULT_BRIGHTEN 0

FILE *ifp, *ofp;

int16 angle = 0;
uint8 auto_level = 1;
uint8 dither_alg = DITHER_BURKES;
uint8 resize = 1;
uint8 dither_threshold = DEFAULT_DITHER_THRESHOLD;
int8 brighten = DEFAULT_BRIGHTEN;

void get_program_disk(void) {
  while (reopen_start_device() != 0) {
    clrscr();
    gotoxy(13, 12);
    printf("Please reinsert the program disk, then press any key.");
    cgetc();
  }
}

void qt_convert_image(const char *filename) {
  static char imgname[BUF_SIZE];

  clrscr();
  dputs("Image conversion\r\n\r\n");
  if (!filename) {
    char *tmp;
    
    dputs("Image (QTK): ");
    tmp = file_select(wherex(), wherey(), scrw - wherex(), wherey() + 10, 0, "Select a QTK file");
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

    if (!strcmp(magic, QT100_MAGIC)) {
      exec("qt100conv", imgname);
    } else if (!strcmp(magic, QT150_MAGIC)) {
      exec("qt150conv", imgname);
    } else {
      cputs("Unknown file type.\r\n");
    }
  }
}

static uint8 *baseaddr[HGR_HEIGHT];
static uint8 div7_table[HGR_WIDTH];
static uint8 mod7_table[HGR_WIDTH];
static uint16 histogram[256];
static uint8 opt_histogram[256];
#define NUM_PIXELS 49152U //256*192

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

    printf("Histogram equalization...\n");
    do {
      curr_hist += histogram[x];
      opt_histogram[x] = (uint8)((((uint32)curr_hist * 255)) / NUM_PIXELS);
    } while (++x);
  } else {
fallback_std:
    do {
      opt_histogram[x] = x;
    } while (++x);
  }
}

static void init_base_addrs (void)
{
  static uint8 y, base_init_done = 0;
  uint16 x, group_of_eight, line_of_eight, group_of_sixtyfour;
  if (base_init_done) {
    return;
  }

  for (y = 0; y < HGR_HEIGHT; ++y)
  {
    line_of_eight = y % 8;
    group_of_eight = (y % 64) / 8;
    group_of_sixtyfour = y / 64;

    baseaddr[y] = (uint8 *)HGR_PAGE + line_of_eight * 1024 + group_of_eight * 128 + group_of_sixtyfour * 40;
  }
  for (x = 0; x < HGR_WIDTH; x++) {
    div7_table[x] = x / 7;
    mod7_table[x] = x % 7;
  }
  histogram_equalize();
  base_init_done = 1;
}

#define X_OFFSET ((HGR_WIDTH - file_width) / 2)

#ifndef __CC65__
char HGR_PAGE[HGR_LEN];
#endif

static uint8 reedit_image(const char *ofname) {
  char c;

start_edit:
  do {
    clrscr();
    gotoxy(0, 20);
    printf("L: Rotate left - U: Rotate upside-down - R: Rotate right (Angle %d)",
           angle);
    if (angle == 90 || angle == 270) {
      if (resize)
        printf(" - C: Crop\n");
      else
        printf(" - C: Fit\n");
    } else {
      printf("\n");
    }
    printf("H: Auto-level %s - B: Brighten - D: Darken (Current %s%d)\n",
           auto_level ? "off":"on", 
           brighten > 0 ? "+":"",
           brighten);
    printf("Dither with K: Burkes / Y: Bayer / N: Don't dither (Current: %s)\n"
           "S: Save - Escape: Exit without saving - Any other key: Hide help",
           dither_alg == DITHER_BURKES ? "Burkes"
            : dither_alg == DITHER_BAYER ? "Bayer" : "None");

    c = tolower(cgetc());
    if (!hgr_mix_is_on()) {
      hgr_mixon();
    } else {
      switch(c) {
        case CH_ESC:
          clrscr();
          gotoxy(0, 20);
          printf("Exit without saving? (y/N) ");
          c = tolower(cgetc());
          if (c == 'y')
            goto done;
          break;
        case 's':
          clrscr();
          gotoxy(0, 20);
          printf("Once saved, image edition will require a new full conversion.\n"
                 "Save? (Y/n)\n");
          c = tolower(cgetc());
          if (c != 'n')
            goto save;
          break;
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
          resize = !resize;
          return 1;
        case 'k':
          dither_alg = DITHER_BURKES;
          return 1;
        case 'y':
          dither_alg = DITHER_BAYER;
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
    }
  } while (1);

save:
  ofp = fopen(ofname, "w");
  if (ofp == NULL) {
    printf("Please insert image floppy for %s, or Escape to return\n", ofname);
    if (cgetc() != CH_ESC)
      goto save;
    goto start_edit;
  }
  printf("\nSaving %s...\n", ofname);
  fseek(ofp, 0, SEEK_SET);
  fwrite((char *)HGR_PAGE, 1, HGR_LEN, ofp);
  fclose(ofp);
done:
  hgr_mixoff();
  init_text();
  return 0;
}

static uint8 buf[FILE_WIDTH];
static uint8 err[FILE_WIDTH * 2];

#pragma inline-stdfuncs(push, on)
#pragma allow-eager-inline(push, on)
#pragma codesize(push, 200)
#pragma register-vars(push, on)

void convert_temp_to_hgr(const char *ifname, const char *ofname, uint16 p_width, uint16 p_height) {
  /* Rotation/cropping variables */
  uint8 start_x, i;
  register uint8 x, end_x;
  register uint16 dx, dy;
  uint16 off_x, y, off_y;
  uint16 file_width;
#if SCALE
  uint8 scaled_dx, scaled_dy;
  uint8 file_height;
#else
  uint16 scaled_dx, scaled_dy;
  uint16 file_height;
#endif
  int8 xdir, ydir;
  int16 cur_err;
  uint8 *ptr;
  uint8 pixel, color;
  uint8 invert_coords;

  /* Burkes variables */
  uint8 buf_plus_err;
  uint8 x_plus1;
  uint8 x_plus2;
  uint8 x_minus1;
  uint8 x_minus2;
  int16 err8, err4, err2;

  /* Bayer variables */
  uint8 map[8][8] = {
    { 1, 49, 13, 61, 4, 52, 16, 64 },
    { 33, 17, 45, 29, 36, 20, 48, 32 },
    { 9, 57, 5, 53, 12, 60, 8, 56 },
    { 41, 25, 37, 21, 44, 28, 40, 24 },
    { 3, 51, 15, 63, 2, 50, 14, 62 },
    { 25, 19, 47, 31, 34, 18, 46, 30 },
    { 11, 59, 7, 55, 10, 58, 6, 54 },
    { 43, 27, 39, 23, 42, 26, 38, 22 }
  };
  uint8 y_mod8;

  /* General variables */
  uint8 *cur_err_line = err;
  uint8 *next_err_line;
#if SCALE
  uint8 h_plus1;
#else
  uint16 h_plus1;
#endif
  uint8 dhbmono[] = {0x7e,0x7d,0x7b,0x77,0x6f,0x5f,0x3f};
  uint8 dhwmono[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40};
  uint8 *cur_hgr_line;
  uint8 cur_hgr_row;
  uint8 cur_hgr_mod;
  uint8 opt_val;

  file_width = p_width;
  file_height = p_height;
  h_plus1 = file_height + 1;
  
  next_err_line = err + file_width;
  init_base_addrs();

  clrscr();
  gotoxy(0, 20);
  printf("Converting %s (Esc to stop)...\n", ofname);

  ifp = fopen(ifname, "r");
  if (ifp == NULL) {
    printf("Can't open %s\n", ifname);
    return;
  }

  printf("Dithering...\n");
  memset(err, 0, sizeof err);
  memset((char *)HGR_PAGE, 0, HGR_LEN);

  progress_bar(wherex(), wherey(), scrw, 0, file_height);

  start_x = 0;
#if SCALE
  end_x = file_width == 256 ? 0 : file_width;
#else
  end_x = file_width;
#endif

  switch (angle) {
    case 0:
      off_x = X_OFFSET;
      off_y = (HGR_HEIGHT - file_height) / 2;
      xdir = +1;
      ydir = +1;
      invert_coords = 0;
      break;
    case 90:
      if (resize) {
        off_x = 0;
        off_y = 212 * 4 / 3;
      } else {
        off_x = 0;
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
      xdir = -1;
      ydir = -1;
      invert_coords = 0;
      break;
  }

  for(y = 0, dy = off_y; y != file_height; y++, dy += ydir) {
    if (file_width == 80) {
      /* assume thumbnail at 4bpp */
      fread(buf, 1, 40, ifp);
      /* Unpack */
      i = 39;
      do {
        int c = buf[i];
        int a = (((c>>4) & 0b00001111) << 4);
        int b = (((c)    & 0b00001111) << 4);
        buf[i*2] = a;
        buf[(i*2) + 1] = b;
      } while (i--);
    } else {
      fread(buf, 1, file_width, ifp);
    }

    if (kbhit()) {
      if (cgetc() == CH_ESC)
        goto stop;
    }

    if (dither_alg == DITHER_BURKES) {
      /* Rollover next error line */
      uint8 *tmp = cur_err_line;
      cur_err_line = next_err_line;
      next_err_line = tmp;
      memset(next_err_line, 0, file_width);
    } else {
      /* Precompute y modulo for the line */
      y_mod8 = y % 8;
    }

    /* Calculate hgr base coordinates for the line */
    if (invert_coords) {
      if (resize) {
        scaled_dy = dy * 3 / 4;
        cur_hgr_row = div7_table[scaled_dy];
        cur_hgr_mod = mod7_table[scaled_dy];
      } else {
        cur_hgr_row = div7_table[dy];
        cur_hgr_mod = mod7_table[dy];
      }
    } else {
      cur_hgr_line = baseaddr[dy];
    }

    x = start_x;
    dx = off_x;
    do {
      /* Get destination pixel */
      if (invert_coords) {
        if (resize) {
          scaled_dx = dx * 3 / 4;
          ptr = baseaddr[scaled_dx] + cur_hgr_row;
          pixel = cur_hgr_mod;
        } else {
          ptr = baseaddr[dx] + cur_hgr_row;
          pixel = cur_hgr_mod;
        }
      } else {
        ptr = cur_hgr_line + div7_table[dx];
        pixel = mod7_table[dx];
      }

      opt_val = buf[x];
      opt_val = opt_histogram[opt_val];
      if (brighten) {
        int16 t = opt_val + brighten;
        if (t < 0)
          opt_val = 0;
        else if (t > 255)
          opt_val = 255;
        else
          opt_val = t;
      }

      /* Dither */
      if (dither_alg == DITHER_BURKES) {
        buf_plus_err = opt_val + cur_err_line[x];
        x_plus1 = x + 1;
        x_plus2 = x_plus1 + 1;
        x_minus1 = x - 1;
        x_minus2 = x_minus1 - 1;

        if (buf_plus_err < dither_threshold) {
          cur_err = buf_plus_err;
          color = 0;
        } else {
          cur_err = buf_plus_err - 255;
          color = 1;
        }
        err8 = cur_err >> 2; /* cur_err * 8 / 32 */
        err4 = err8 >> 1;    /* cur_err * 4 / 32 */
        err2 = err4 >> 1;    /* cur_err * 2 / 32 */

        if (x < file_width - 1) {
          cur_err_line[x_plus1]    += err8;
          next_err_line[x_plus1]   += err4;
          if (x < file_width - 2) {
            cur_err_line[x_plus2]  += err4;
            next_err_line[x_plus2] += err2;
          }
        }
        if (x > 0) {
          next_err_line[x_minus1]  += err4;
          if (x > 1) {
            next_err_line[x_minus2]+= err2;
          }
        }
        next_err_line[x]           += err8;
      } else if (dither_alg == DITHER_BAYER) {
        uint16 val = opt_val;
        val += val * map[y_mod8][x % 8] / 63;
        if (val < dither_threshold) {
          color = 0;
        } else {
          color = 1;
        }
      } else if (dither_alg == DITHER_NONE) {
        if (opt_val < dither_threshold) {
          color = 0;
        } else {
          color = 1;
        }      
      }
      if (color) {
        *ptr |= dhwmono[pixel];
      } else {
        *ptr &= dhbmono[pixel];
      }
      x++;
      dx += xdir;
    } while (x != end_x);
    if (y % 8 == 0)
      progress_bar(-1, -1, scrw, y, file_height);
  }
  progress_bar(-1, -1, scrw, file_height, file_height);
stop:
  fclose(ifp);
}

#pragma register-vars(pop)
#pragma codesize(pop)
#pragma allow-eager-inline(pop)
#pragma inline-stdfuncs(pop)

void qt_edit_image(const char *ofname) {
  do {
    if (angle >= 360)
      angle -= 360;
    if (angle < 0)
      angle += 360;
    convert_temp_to_hgr(TMP_NAME, ofname, FILE_WIDTH, FILE_HEIGHT);
  } while (reedit_image(ofname));
}

void qt_view_image(const char *filename) {
  FILE *fp = NULL;
  static char imgname[BUF_SIZE];
  uint16 len;
  #define BLOCK_SIZE 512

  clrscr();

  dputs("Image view\r\n\r\n");
  if (!filename) {
    char *tmp;
    dputs("Image (HGR): ");
    tmp = file_select(wherex(), wherey(), scrw - wherex(), wherey() + 10, 0, "Select an HGR file");
    if (tmp == NULL)
      return;
    strcpy(imgname, tmp);
    free(tmp);
  } else {
    strcpy(imgname, filename);
  }

  fp = fopen(imgname, "rb");
  if (fp == NULL) {
    dputs("Can not open image.\r\n");
    cgetc();
    return;
  }

  memset((char *)HGR_PAGE, 0x00, HGR_LEN);
  init_text();
  gotoxy(0, 18);
  cputs("Loading image...");

  progress_bar(0, 19, scrw, 0, HGR_LEN);

  len = 0;
  while (len < HGR_LEN) {
#ifdef __CC65__
    fread((char *)(HGR_PAGE + len), 1, BLOCK_SIZE, fp);
#endif
    progress_bar(-1, -1, scrw, len, HGR_LEN);
    len += BLOCK_SIZE;
  }

  init_hgr(1);
  hgr_mixoff();

  fclose(fp);

  cgetc();
  init_text();

  get_program_disk();
  clrscr();
  return;
}

#ifdef __CC65__
  #pragma static-locals(pop)
#endif
