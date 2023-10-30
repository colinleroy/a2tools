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

int main (void)
{
  uint8 num_pics, left_pics, mode, choice;
  char *name;
  struct tm time;

  register_start_device();

#ifdef __CC65__
  videomode(VIDEOMODE_80COL);
  screensize(&scrw, &scrh);
#endif

  while (qt_serial_connect(19200) != 0) {
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
