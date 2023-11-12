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

#include "splash.h"
#include "qt-conv.h"
#include "qt-edit-image.h"
#include "qt-serial.h"

#pragma code-name(push, "LOWCODE")

uint8 scrw, scrh;
uint8 camera_connected;
uint8 current_quality, current_flash_mode;

#ifdef __CC65__
  #pragma static-locals(push, on)
#endif

char magic[5] = "????";

static void print_header(uint8 num_pics, uint8 left_pics, uint8 mode, uint8 flash_mode, uint8 battery_level, const char *name, struct tm *time) {
  gotoxy(0, 0);
  if (camera_connected) {
    printf("%s connected - %d%% battery - %02d/%02d/%04d %02d:%02d\n"
           "%d photos taken, %d left, %s mode, %s flash\n",
          name, battery_level, time->tm_mday, time->tm_mon, time->tm_year, time->tm_hour, time->tm_min,
          num_pics, left_pics, qt_get_mode_str(mode), qt_get_flash_str(flash_mode));
  } else {
    printf("No camera connected\n\n");
  }
  chline(scrw);
}

static uint8 print_menu(void) {
  printf("Menu\n\n");
  if (camera_connected) {
    printf(" G. Get one picture\n"
           " P. Preview pictures\n"
           " D. Delete all pictures\n"
           " S. Snap a picture\n");
  } else {
    printf(" C. Connect camera\n");
  }
  printf(  " R. Re-edit a raw picture from floppy\n"
           " V. View a converted picture from floppy\n");
  if (camera_connected) {
    printf(" N. Set camera name\n"
           " T. Set camera time\n"
           " Q. Set quality to %s\n"
           " F. Set flash to %s\n",
           qt_get_mode_str((current_quality == QUALITY_HIGH) ? QUALITY_STANDARD:QUALITY_HIGH),
           qt_get_flash_str((current_flash_mode + 1) % 3));
  }
  printf(  "\n"
           " 0. Exit\n\n");
  return cgetc();
}

static void save_picture(uint8 n_pic) {
  char filename[64];
  char *dirname;

  filename[0] = '\0';
#ifdef __CC65__
  clrscr();
  printf("Saving picture %d\n\n"

        "Make sure to save the picture to a floppy with\n"
        "at least 118480 + 8192 (124kB) free. Basically,\n"
        "use one floppy per picture.\n"
        "Do not use /RAM, which will be used for temporary storage.\n\n"
        "Please swap disks if needed and press a key.\n\n",
      n_pic);
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

  if (filename[strlen(filename) - 1] == '/')
    return;

  if (!strchr(filename, '.')) {
    strcat(filename, ".QTK");
  }

  if (qt_get_picture(n_pic, filename) == 0) {
    qt_convert_image(filename);
  }
}

static void get_one_picture(uint8 num_pics) {
  char buf[3];
  int8 n_pic;

  clrscr();
  dputs("Get a picture from the camera\r\n\r\n"

        "Picture number? ");

  buf[0] = '\0';
  dget_text(buf, 3, NULL, 0);

  if (buf[0] == '\0')
    return;

  n_pic = atoi(buf);
  if (n_pic < 1 || n_pic > num_pics) {
    dputc(0x07);
    printf("No image %d in camera.\n"
           "Please press a key...", n_pic);
    cgetc();
    return;
  }
  save_picture(n_pic);
}

static void set_camera_name(const char *name) {
  char buf[31];

  if (name == NULL) {
    return;
  }

  clrscr();
  dputs("Camera renaming\r\n\r\n"

        "New camera name: ");

  strncpy(buf, name, 31);
  dget_text(buf, 31, NULL, 0);

  qt_set_camera_name(buf);
}

static void set_camera_time(void) {
  char *names[5] = {"Day","Month","Year","Hour","Minute"};
  char buf[5];
  uint8 vals[5];
  uint8 i;

  clrscr();
  dputs("Camera time setting\r\n\r\n"

        "Please enter the current date and time:\r\n");

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
  dputs("Pictures deletion\r\n\r\n"

        "Delete all pictures on camera? (y/N)\r\n");

  if (tolower(cgetc()) == 'y') {
    dputs("Deleting pictures, please wait...\r\n");
    qt_delete_pictures();
  }
  dputs("Done!...\r\n");
}

static void take_picture(void) {
  clrscr();
  dputs("Taking a picture...\r\n\r\n");
  qt_take_picture();
  dputs("Done!...\r\n");
}

static void show_thumbnails(uint8 num_pics) {
  uint8 i = 0;
  char c;
  char thumb_buf[32];
  if (num_pics == 0) {
    return;
  }

  set_scrollwindow(0, scrh);
  init_hgr(1);
  hgr_mixon();

  do {
    i++;
    if (i > num_pics) {
      i = 1;
    }

    clrscr();
    gotoxy(0,20);
    if (qt_get_thumbnail(i) != 0) {
      break;
    }

    sprintf(thumb_buf, "Thumbnail %d", i);
    convert_temp_to_hgr(THUMBNAIL_NAME, thumb_buf, 80, 60);

    clrscr();
    gotoxy(0,20);
    printf("%s - G to get full picture\n"
           "Escape to exit, any other key to continue",
           thumb_buf);
    c = tolower(cgetc());
  } while (c != CH_ESC && c != 'g');
  unlink(THUMBNAIL_NAME);

  if (c == 'g') {
    init_text();
    save_picture(i);
  }
}

int main(int argc, char *argv[])
{
  uint8 num_pics, left_pics, mode, choice, flash, batt;
  char *name;
  struct tm time;
#ifndef __CC65__
  uint16 target_speed = 57600U;
  scrw = 80; scrh = 24;

#else
#ifndef IIGS
  uint16 target_speed = 19200U;
#else
  uint16 target_speed = 57600U;
#endif

  register_start_device();

  videomode(VIDEOMODE_80COL);
  screensize(&scrw, &scrh);
  init_hgr(1);
  hgr_mixon();
  clrscr();
  gotoxy(0,20);
  printf("Welcome to Quicktake for Apple II - (c) Colin Leroy-Mira, https://colino.net\n");
  printf("Free memory: %zuB - ", _heapmemavail());
#endif

  if (argc > 1) {
    qt_edit_image(argv[1]);
  } else {
    set_scrollwindow(21, scrh);
  }

  /* Remove temporary files */
  unlink(HIST_NAME);
  unlink(TMP_NAME);

  camera_connected = 0;
connect:
  while (qt_serial_connect(target_speed) != 0) {
    char c;

    if (target_speed != 9600)
      printf("Please turn the Quicktake off and on. Try again at %u or at 9600bps? (Y/n/9)\n", target_speed);
    else
      printf("Please turn the Quicktake off and on. Try again? (Y/n)\n");

    c = tolower(cgetc());
    if (c == 'n')
      goto menu;
    if(c == '9')
      target_speed = 9600;
  }
  camera_connected = 1;
menu:
  init_text();
  set_scrollwindow(0, scrh);
  clrscr();
  gotoxy(0, 0);
  if (camera_connected) {
    if (qt_get_information(&num_pics, &left_pics, &mode, &flash, &batt, &name, &time) != 0)
      camera_connected = 0;
    current_quality = mode;
    current_flash_mode = flash;
  }

  print_header(num_pics, left_pics, mode, flash, batt, name, &time);

  set_scrollwindow(4, scrh);
  gotoxy(0, 0);

  choice = tolower(print_menu());
  switch(choice) {
    case 'p':
      if (camera_connected) {
        show_thumbnails(num_pics);
      }
      break;
    case 'g':
      if (camera_connected) {
        get_one_picture(num_pics);
      }
      break;
    case 'c':
      if (!camera_connected) {
        goto connect;
      }
      break;
    case 'd':
      if (camera_connected) {
        delete_pictures();
      }
      break;
    case 's':
      if (camera_connected) {
        take_picture();
      }
      break;
    case 'r':
      qt_convert_image(NULL);
      break;
    case 'v':
      qt_view_image(NULL);
      break;
    case 'n':
      if (camera_connected) {
        set_camera_name(name);
      }
      break;
    case 't':
      if (camera_connected) {
        set_camera_time();
      }
      break;
    case 'q':
      if (camera_connected) {
        qt_set_quality((current_quality == QUALITY_HIGH) ? QUALITY_STANDARD:QUALITY_HIGH);
      }
      break;
    case 'f':
      if (camera_connected) {
        qt_set_flash((current_flash_mode + 1) % 3);
      }
      break;
    case '0':
      goto out;
    default:
      break;
  }
  goto menu;

out:
  free(name);
  return 0;
}

#ifdef __CC65__
  #pragma static-locals(pop)
#endif
#pragma code-name(pop)
