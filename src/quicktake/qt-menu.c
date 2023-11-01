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

#include "qt-conv.h"
#include "qt-serial.h"

uint8 scrw, scrh;

#ifdef __CC65__
  #pragma static-locals(push, on)
#endif

#define BUF_SIZE 64
char magic[5] = "????";

#define DITHER_NONE   0
#define DITHER_BURKES 1
#define DITHER_BAYER  2
#define DEFAULT_DITHER_THRESHOLD 128

int16 angle = 0;
uint8 auto_level = 1;
uint8 dither_alg = DITHER_BURKES;
uint8 resize = 1;
uint8 mix_is_on = 0;
uint8 dither_threshold = DEFAULT_DITHER_THRESHOLD;

static void convert_temp_to_hgr(const char *ofname);

static void convert_image(const char *filename) {
  static char imgname[BUF_SIZE];
  if (!filename) {
    char *tmp = file_select(wherex(), wherey(), scrw - wherex(), wherey() + 10, 0, "Image to convert: ");
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
    if (!strcmp(magic, QT100_MAGIC)) {
      exec("qt100conv", imgname);
    } else if (!strcmp(magic, QT150_MAGIC)) {
      exec("qt150conv", imgname);
    } else {
      cputs("Unknown file type.\r\n");
    }
  }
}

static void view_image(const char *filename) {
  FILE *fp = NULL;
  static char imgname[BUF_SIZE];
  uint16 len;
  #define BLOCK_SIZE 512

  if (!filename) {
    char *tmp;
    dputs("Image to view: ");
    tmp = file_select(wherex(), wherey(), scrw - wherex(), wherey() + 10, 0, "Please select an HGR file");
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
  gotoxy(0, 22);
  cputs("Loading image...");

  progress_bar(0, 23, scrw, 0, HGR_LEN);

  len = 0;
  while (len < HGR_LEN) {
#ifdef __CC65__
    fread((char *)(HGR_PAGE + len), 1, BLOCK_SIZE, fp);
#endif
    progress_bar(0, 23, scrw, len, HGR_LEN);
    len += BLOCK_SIZE;
  }

  init_hgr();
  hgr_mixoff();
  mix_is_on = 0;

  fclose(fp);

  cgetc();
  init_text();

  while (reopen_start_device() != 0) {
    clrscr();
    gotoxy(13, 12);
    printf("Please reinsert the program disk, then press any key.");
    cgetc();
  }
  clrscr();
  return;
}

static unsigned baseaddr[192];
static void init_base_addrs (void)
{
  uint16 i, group_of_eight, line_of_eight, group_of_sixtyfour;

  for (i = 0; i < HGR_HEIGHT; ++i)
  {
    line_of_eight = i % 8;
    group_of_eight = (i % 64) / 8;
    group_of_sixtyfour = i / 64;

    baseaddr[i] = line_of_eight * 1024 + group_of_eight * 128 + group_of_sixtyfour * 40;
  }
}

#define FILE_WIDTH 256
#define FILE_HEIGHT HGR_HEIGHT
#define X_OFFSET ((HGR_WIDTH - FILE_WIDTH) / 2)

#ifndef __CC65__
char HGR_PAGE[HGR_LEN];
#endif

static uint8 edit_image(const char *ofname) {
  char c;

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
    printf("H: Auto-level %s - B: Brighten - D: Darken (Threshold %d)\n",
           auto_level ? "off":"on", dither_threshold);
    printf("Dither with K: Burkes / Y: Bayer / N: Don't dither (Current: %s)\n"
           "S: Save - Escape: Exit without saving - Any other key: Hide help",
           dither_alg == DITHER_BURKES ? "Burkes"
            : dither_alg == DITHER_BAYER ? "Bayer" : "None");

    c = tolower(cgetc());
    if (!mix_is_on) {
      hgr_mixon();
      mix_is_on = 1;
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
          printf("Once saved, image edition will not be possible anymore.\n"
                 "Save? (Y/n) ");
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
          return 1;
        case 'c':
          resize = !resize;
          return 1;
        case 'k':
          dither_alg = DITHER_BURKES;
          dither_threshold = DEFAULT_DITHER_THRESHOLD;
          return 1;
        case 'y':
          dither_alg = DITHER_BAYER;
          dither_threshold = DEFAULT_DITHER_THRESHOLD;
          return 1;
        case 'n':
          dither_alg = DITHER_NONE;
          dither_threshold = DEFAULT_DITHER_THRESHOLD;
          return 1;
        case 'b':
          dither_threshold -= 10;
          return 1;
        case 'd':
          dither_threshold += 10;
          return 1;
        default:
          hgr_mixoff();
          mix_is_on = 0;
      }
    }
  } while (1);

save:
  ofp = fopen(ofname, "w");
  if (ofp == NULL) {
    printf("Can't open %s\n", ofname);
    fclose(ifp);
    return 0;
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

static uint8 buf[256];
static uint8 err[512];
static uint16 histogram[256];
static uint8 opt_histogram[256];
FILE *ifp, *ofp;

#define NUM_PIXELS 49152U //256*192

static void convert_temp_to_hgr(const char *ofname) {
  /* Rotation/cropping variables */
  uint16 x, off_x, start_x, end_x;
  uint16 y, off_y;
  uint16 dx, dy, scaled_dx, scaled_dy;
  int8 xdir, ydir;
  int16 cur_err;
  uint8 *ptr;
  uint8 pixel;
  uint8 invert_coords;

  /* Burkes variables */
  uint8 buf_plus_err;
  uint16 x_plus1;
  uint16 x_plus2;
  int16 x_minus1;
  int16 x_minus2;
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

  /* General variables */
  uint16 curr_hist = 0;
  uint8 *err_line_2 = err + FILE_WIDTH;
  uint16 h_plus1 = FILE_HEIGHT + 1;
  unsigned char dhbmono[] = {0x7e,0x7d,0x7b,0x77,0x6f,0x5f,0x3f};
  unsigned char dhwmono[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40};

  init_base_addrs();

  printf("Finishing %s conversion...\n", ofname);

  if (auto_level) {
    ifp = fopen(HIST_NAME, "r");
    if (ifp != NULL) {
      fread(histogram, sizeof(uint16), 256, ifp);
      fclose(ifp);

      printf("Histogram equalization...\n");
      for (x = 0; x < 256; x++) {
        curr_hist += histogram[x];
        opt_histogram[x] = (uint8)((((uint32)curr_hist * 255)) / NUM_PIXELS);
      }
    } else {
      printf("Can't open "HIST_NAME"\n");
    }
  } else {
    for (x = 0; x < 256; x++) {
      opt_histogram[x] = x;
    }
  }

  ifp = fopen(TMP_NAME, "r");
  if (ifp == NULL) {
    printf("Can't open "TMP_NAME"\n");
    return;
  }

  printf("Dithering...");
  memset(err, 0, sizeof err);
  memset((char *)HGR_PAGE, 0, HGR_LEN);

  start_x = 0;
  end_x = FILE_WIDTH;
  switch (angle) {
    case 0:
      off_x = X_OFFSET;
      off_y = 0;
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
        end_x = FILE_WIDTH - 33;
      }
      xdir = +1;
      ydir = -1;
      invert_coords = 1;
      break;
    case 270:
      if (resize) {
        off_x = FILE_WIDTH - 1;
        off_y = 68 * 4 / 3;
      } else {
        off_x = HGR_HEIGHT - 1;
        off_y = 44;
        start_x = 32;
        end_x = FILE_WIDTH - 33;
      }
      xdir = -1;
      ydir = +1;
      invert_coords = 1;
      break;
    case 180:
      off_x = HGR_WIDTH - X_OFFSET;
      off_y = FILE_HEIGHT - 1;
      xdir = -1;
      ydir = -1;
      invert_coords = 0;
      break;
  }

  for(y = 0, dy = off_y; y != FILE_HEIGHT; y++, dy+= ydir) {
    fread(buf, 1, FILE_WIDTH, ifp);

    /* Rollover next error line */
    if (dither_alg == DITHER_BURKES) {
      memcpy(err, err_line_2, FILE_WIDTH);
      memset(err_line_2, 0, FILE_WIDTH);
    }

    for(x = start_x, dx = off_x; x != end_x; x++, dx += xdir) {
      //printf("y %d dy %d x %d dx %d\n", y, dy, x, dx);
      /* Get destination pixel */
      if (invert_coords) {
        if (resize) {
          scaled_dy = dy * 3 / 4;
          scaled_dx = dx * 3 / 4;
        } else {
          scaled_dy = dy;
          scaled_dx = dx;
        }
        ptr = (char *)HGR_PAGE + baseaddr[scaled_dx] + scaled_dy / 7;
        pixel = scaled_dy % 7;
      } else {
        ptr = (char *)HGR_PAGE + baseaddr[dy] + dx / 7;
        pixel = dx % 7;
      }

      /* Dither */
      if (dither_alg == DITHER_BURKES) {
        buf_plus_err = opt_histogram[buf[x]] + err[x];
        x_plus1 = x + 1;
        x_plus2 = x + 2;
        x_minus1 = x - 1;
        x_minus2 = x - 2;

        if (dither_threshold > buf_plus_err) {
          cur_err = buf_plus_err;
          ptr[0] &= dhbmono[pixel];
        } else {
          cur_err = buf_plus_err - 255;
          ptr[0] |= dhwmono[pixel];
        }
        err8 = cur_err >> 2; /* cur_err * 8 / 32 */
        err4 = err8 >> 1;    /* cur_err * 4 / 32 */
        err2 = err4 >> 1;    /* cur_err * 2 / 32 */

        if (x_plus1 < FILE_WIDTH) {
          err[x_plus1]          += err8;
          err_line_2[x_plus1]   += err4;
          if (x_plus2 < FILE_WIDTH) {
            err[x_plus2]        += err4;
            err_line_2[x_plus2] += err2;
          }
        }
        if (x_minus1 > 0) {
          err_line_2[x_minus1]   += err4;
          if (x_minus2 > 0) {
            err_line_2[x_minus2] += err2;
          }
        }
        err_line_2[x]            += err8;
      } else if (dither_alg == DITHER_BAYER) {
        uint16 val = opt_histogram[buf[x]];

        val += val * map[y % 8][x % 8] / 63;
        if (dither_threshold > val) {
          ptr[0] &= dhbmono[pixel];
        } else {
          ptr[0] |= dhwmono[pixel];
        }
      } else if (dither_alg == DITHER_NONE) {
        if (dither_threshold > opt_histogram[buf[x]]) {
          ptr[0] &= dhbmono[pixel];
        } else {
          ptr[0] |= dhwmono[pixel];
        }      
      }
    }
  }
  fclose(ifp);
}

static void print_header(uint8 num_pics, uint8 left_pics, uint8 mode, const char *name, struct tm *time) {
  gotoxy(0, 0);
  printf("%s connected - %02d/%02d/%04d %02d:%02d\n%d photos taken, %d left, %s mode\n",
         name, time->tm_mday, time->tm_mon, time->tm_year, time->tm_hour, time->tm_min,
         num_pics, left_pics, qt_get_mode_str(mode));
  chline(scrw);
}

uint8 auto_convert = 1;

static uint8 print_menu(void) {
  gotoxy(0, 3);
  printf("1. Get one picture\n"
         "2. Get all pictures\n"
         "3. Delete all pictures\n"
         "4. Take a picture\n"
         "5. Convert a picture on floppy\n"
         "6. View a picture\n"
         "7. Set camera name\n"
         "8. Set camera time\n"
         "9. %s auto-conversion to HGR\n"
         "0. Exit\n\n",

          auto_convert ? "Disable":"Enable");
  return cgetc();
}

static void save_picture(uint8 n_pic, uint8 full) {
  char filename[64];
  char *dirname;

  filename[0] = '\0';
#ifdef __CC65__
  clrscr();
  dputs("Make sure to save the picture to a floppy with\r\n"
        "at least 118480 + 8192 (124kB) free. Basically,\r\n"
        "use one floppy per picture.\r\n"
        "Please swap disks if needed and press a key.\r\n\r\n");
  cgetc();

  dirname = file_select(wherex(), wherey(), scrw - wherex(), wherey() + 10, 1, "Filename: ");
  if (dirname == NULL) {
    return;
  }
  gotox(0);
  strcpy(filename, dirname);
  strcat(filename, "/");
  free(dirname);

  dget_text(filename, 60, NULL, 0);
#else
  sprintf(filename, "image%02d.qtk", n_pic);
#endif
  if (!strchr(filename, '.')) {
    strcat(filename, ".QTK");
  }

  qt_get_picture(n_pic, filename, full);

  if (!auto_convert) {
    char c;
    dputs("Convert to HGR? (Y/n) ");
    c = tolower(cgetc());
    if (c != 'n') {
      convert_image(filename);
    }
  } else {
    convert_image(filename);
  }
}

static void get_all_pictures(uint8 num_pics, uint8 full) {
  uint8 n_pic;
  for (n_pic = 1; n_pic <= num_pics; n_pic++) {
    save_picture(n_pic, full);
  }
}

static void get_one_picture(uint8 num_pics, uint8 full) {
  char buf[3];
  int8 n_pic;

again:
  dputs("Picture number? ");
  buf[0] = '\0';
  dget_text(buf, 3, NULL, 0);
  n_pic = atoi(buf);
  if (n_pic < 1 || n_pic > num_pics) {
    dputc(0x07);
    goto again;
  }
  save_picture(n_pic, full);
}

static void set_camera_name(const char *name) {
  char buf[31];

  strncpy(buf, name, 31);

  dputs("Name: ");
  dget_text(buf, 31, NULL, 0);

  qt_set_camera_name(buf);
}

static void set_camera_time(void) {
  char *names[5] = {"Day","Month","Year","Hour","Minute"};
  char buf[5];
  uint8 vals[5];
  uint8 i;

  for (i = 0; i < 5; i++) {
    buf[0] = '\0';
    dputs(names[i]);
    dputs(": ");
    dget_text(buf, 5, NULL, 0);
    vals[i] = (uint8)(atoi(buf) % 100);
  }

  qt_set_camera_time(vals[0], vals[1], vals[2], vals[3], vals[4], 0);
}

static void delete_pictures(void) {
  clrscr();
  dputs("Delete all pictures on camera? (y/N) ");
  if (tolower(cgetc()) == 'y') {
    qt_delete_pictures();
  }
  dputs("\r\nPlease wait...\r\n");
}

int main (int argc, char *argv[])
{
  uint8 num_pics, left_pics, mode, choice;
  char *name;
  struct tm time;
#ifndef __CC65__
  int target_speed = 57600;
#else
  int target_speed = 9600;

  register_start_device();

  videomode(VIDEOMODE_80COL);
  printf("Free memory: %zu/%zuB\n", _heapmaxavail(), _heapmemavail());
  screensize(&scrw, &scrh);
#endif

  if (argc > 1) {
    init_hgr();
    do {
      clrscr();
      if (angle >= 360)
        angle -= 360;
      if (angle < 0)
        angle += 360;
      convert_temp_to_hgr(argv[1]);
    } while (edit_image(argv[1]));
  } else {
    /* For testing */
    //exec("qt100conv","/QT100/TEST100.qtk");
  }

  /* Remove temporary files */
  unlink(HIST_NAME);
  unlink(TMP_NAME);

  while (qt_serial_connect(target_speed) != 0) {
    char c;
    dputs("Try again? (Y/n) ");
    c = tolower(cgetc());
    if (c == 'n')
      goto out;
  }

again:
  qt_get_information(&num_pics, &left_pics, &mode, &name, &time);

  clrscr();
  print_header(num_pics, left_pics, mode, name, &time);

  choice = print_menu();
  switch(choice) {
    case '1':
      get_one_picture(num_pics, 1);
      break;
    case '2':
      get_all_pictures(num_pics, 1);
      break;
    case '3':
      delete_pictures();
      break;
    case '4':
      qt_take_picture();
      break;
    case '5':
      clrscr();
      convert_image(NULL);
      break;
    case '6':
      clrscr();
      view_image(NULL);
      break;
    case '7':
      set_camera_name(name);
      break;
    case '8':
      set_camera_time();
      break;
    case '9':
      auto_convert = !auto_convert;
      break;
    case '0':
      goto out;
    default:
      break;
  }
  goto again;

out:
  free(name);
  return 0;
}

#ifdef __CC65__
  #pragma static-locals(pop)
#endif
