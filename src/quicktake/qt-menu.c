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

uint8 scrw, scrh;
uint8 camera_connected;

extern uint8 mix_is_on;

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
    printf(" 1. Get one picture\n"
           " 2. Delete all pictures\n"
           " 3. Take a picture\n");
  } else {
    printf(" 1. Connect camera\n");
  }
  printf(  " 4. Re-edit a raw picture from floppy\n"
           " 5. View a converted picture from floppy\n");
  if (camera_connected) {
    printf(" 6. Set camera name\n"
           " 7. Set camera time\n");
  }
  printf(  "\n"
           " 0. Exit\n\n");
  return cgetc();
}

static void save_picture(uint8 n_pic, uint8 full) {
  char filename[64];
  char *dirname;

  filename[0] = '\0';
#ifdef __CC65__
  clrscr();
  dputs("Saving picture\r\n\r\n"
  
        "Make sure to save the picture to a floppy with\r\n"
        "at least 118480 + 8192 (124kB) free. Basically,\r\n"
        "use one floppy per picture.\r\n"
        "Do not use /RAM, which will be used for temporary storage.\r\n\r\n"
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

  if (filename[strlen(filename) - 1] == '/')
    return;

  if (!strchr(filename, '.')) {
    strcat(filename, ".QTK");
  }

  if (qt_get_picture(n_pic, filename, full) == 0) {
    qt_convert_image(filename);
  }
}

static void get_one_picture(uint8 num_pics, uint8 full) {
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
  save_picture(n_pic, full);
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
  mix_is_on = 1;
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
  }

  print_header(num_pics, left_pics, mode, flash, batt, name, &time);

  set_scrollwindow(4, scrh);
  gotoxy(0, 0);

  choice = print_menu();
  switch(choice) {
    case '1':
      if (camera_connected) {
        get_one_picture(num_pics, 1); 
      } else {
        goto connect;
      }
      break;
    case '2':
      if (camera_connected) {
        delete_pictures();
      }
      break;
    case '3':
      if (camera_connected) {
        take_picture();
      }
      break;
    case '4':
      qt_convert_image(NULL);
      break;
    case '5':
      qt_view_image(NULL);
      break;
    case '6':
      if (camera_connected) {
        set_camera_name(name);
      }
      break;
    case '7':
      if (camera_connected) {
        set_camera_time();
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
