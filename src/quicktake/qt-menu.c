#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dgets.h"
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

#include "runtime_once_clean.h"

#pragma code-name(push, "LOWCODE")

uint8 scrw, scrh;
uint8 camera_connected;
char *camera_name = NULL;
struct tm camera_time;
uint8 num_pics, left_pics, choice, battery_level, charging, current_quality, current_flash_mode;

#ifdef __CC65__
  #pragma static-locals(push, on)
#endif

char magic[5] = "????";

static void print_header(void) {
  gotoxy(0, 0);
  if (camera_connected) {
    printf("%s connected - %d%% battery%s - %02d/%02d/%04d %02d:%02d\n"
           "%d photos taken, %d left, %s mode, %s flash - ",
          camera_name, battery_level, charging? " (charging)":"",
          camera_time.tm_mday, camera_time.tm_mon, camera_time.tm_year,
          camera_time.tm_hour, camera_time.tm_min,
          num_pics, left_pics, qt_get_mode_str(current_quality), qt_get_flash_str(current_flash_mode));
  } else {
    printf("No camera connected\n");
  }
#ifdef __CC65__
  printf("Free RAM: %zuB\n", _heapmemavail());
#endif
  chline(scrw);
}

static uint8 print_menu(void) {
  printf("Menu\n\n");
  if (camera_connected) {
    if (serial_model != QT_MODEL_200) {
      printf(" G. Get one picture\n"
           " P. Preview pictures\n"
           " D. Delete all pictures\n"
           " S. Snap a picture\n");
    } else {
      printf(" G. Get one picture\n");
    }
  } else {
    printf(" C. Connect camera\n");
  }
  printf(  " R. Re-edit a raw picture from floppy\n"
           " V. View a converted picture from floppy\n");
  if (camera_connected  && serial_model != QT_MODEL_200) {
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

  dirname = file_select(wherex(), wherey(), scrw - wherex(), wherey() + 10, 1, "Select directory");
  if (dirname == NULL) {
    return;
  }
  gotox(0);
  strcpy(filename, dirname);
  strcat(filename, "/");
  free(dirname);

  dget_text(filename, 60, NULL, 0);
#else
  if (serial_model == QT_MODEL_200)
    sprintf(filename, "image%02d.jpg", n_pic);
  else
    sprintf(filename, "image%02d.qtk", n_pic);
#endif

  if (filename[strlen(filename) - 1] == '/')
    return;

  if (qt_get_picture(n_pic, filename) == 0) {
    qt_convert_image(filename);
  } else {
    printf("Error saving picture.\n");
    cgetc();
  }
}

static void get_one_picture(uint8 num_pics) {
  char buf[3];
  int8 n_pic;

  clrscr();
  cputs("Get a picture from the camera\r\n\r\n"

        "Picture number? ");

  buf[0] = '\0';
  dget_text(buf, 3, NULL, 0);

  if (buf[0] == '\0')
    return; 

  n_pic = atoi(buf);
  if (n_pic < 1 || n_pic > num_pics) {
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
  cputs("Camera renaming\r\n\r\n"

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
  cputs("Camera time setting\r\n\r\n"

        "Please enter the current date and time:\r\n");

  for (i = 0; i < 5; i++) {
    buf[0] = '\0';
    cputs(names[i]);
    cputs(": ");
    dget_text(buf, 5, NULL, 0);
    vals[i] = (uint8)(atoi(buf) % 100);
  }

  qt_set_camera_time(vals[0], vals[1], vals[2], vals[3], vals[4], 0);
}

static void delete_pictures(void) {
  clrscr();
  cputs("Pictures deletion\r\n\r\n"

        "Delete all pictures on camera? (y/N)\r\n");

  if (tolower(cgetc()) == 'y') {
    cputs("Deleting pictures, please wait...\r\n");
    qt_delete_pictures();
  }
  cputs("Done!...\r\n");
}

static void take_picture(void) {
  clrscr();
  cputs("Taking a picture...\r\n\r\n");
  qt_take_picture();
  cputs("Done!...\r\n");
}

static void show_thumbnails(uint8 num_pics) {
  uint8 i = 0;
  uint8 quality, flash, year, month, day, hour, minute;

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
    if (qt_get_thumbnail(i, &quality, &flash, &year, &month, &day, &hour, &minute) != 0) {
      printf("Error getting thumbnail.\n");
      cgetc();
      break;
    }

    sprintf(thumb_buf, "Thumbnail %d", i);
    convert_temp_to_hgr(THUMBNAIL_NAME, thumb_buf, THUMB_WIDTH*2, THUMB_HEIGHT*2, serial_model);

    clrscr();
    gotoxy(0,20);

    printf("%s (%s, flash %s, %02d/%02d/%04d %02d:%02d)\n"
           "G to get full picture, Escape to exit, any other key to continue",
           thumb_buf, qt_get_mode_str(quality), flash?"on":"off", day, month,
           year + 2000, hour, minute);
    c = tolower(cgetc());
  } while (c != CH_ESC && c != 'g');
  unlink(THUMBNAIL_NAME);

  if (c == 'g') {
    init_text();
    save_picture(i);
  }
}

#pragma code-name(pop)
#pragma code-name(push, "RT_ONCE")

static uint8 setup(int argc, char *argv[]) {
#ifndef __CC65__
  uint16 target_speed = 57600U;
  scrw = 80; scrh = 24;

#else
#ifndef IIGS
  uint16 target_speed = 19200U;
#else
  uint16 target_speed = 57600U;
#endif
#endif
  register_start_device();

  videomode(VIDEOMODE_80COL);
  if (argc == 1) {
  // exec("QTKTCONV","/QT100/TEST100.QTK 0 0 640 480");
  // exec("QTKNCONV","/QT150/TEST150.QTK 0 0 640 480");
  // exec("JPEGCONV","/QT150/TEST200.JPG 0 0 640 480");
  }
  screensize(&scrw, &scrh);
  init_hgr(1);
  hgr_mixon();
  clrscr();
  gotoxy(0,20);
  printf("Welcome to Quicktake for Apple II - (c) Colin Leroy-Mira, https://colino.net\n");

  if (argc > 2) {
    qt_edit_image(argv[1], atoi(argv[2]));
  } else {
    set_scrollwindow(21, scrh);
  }

  /* Remove temporary files */
  unlink(HIST_NAME);
  unlink(TMP_NAME);

  while (qt_serial_connect(target_speed) != 0) {
    char c;

    printf("Please turn the Quicktake off and on. Try again");
    if (target_speed != 9600)
      printf(" at %u or at 9600bps? (Y/n/9)\n", target_speed);
    else
      printf("? (Y/n)\n");

    c = tolower(cgetc());
    if (c == 'n')
      return 0;
    if(c == '9')
      target_speed = 9600;
  }
  return 1;
}

#pragma code-name(pop)
#pragma code-name(push, "LOWCODE")

int main(int argc, char *argv[])
{
  uint8 choice;

  camera_connected = setup(argc, argv);
menu:
  init_text();
  set_scrollwindow(0, scrh);
  clrscr();
  gotoxy(0, 0);

  if (camera_connected) {
    free(camera_name);
    camera_name = NULL;
    if (qt_get_information(&num_pics, &left_pics, &current_quality,
                           &current_flash_mode, &battery_level, &charging,
                           &camera_name, &camera_time) != 0) {
      camera_connected = 0;
    }
  }

  runtime_once_clean();

  print_header();

  set_scrollwindow(4, scrh);
  gotoxy(0, 0);

  choice = tolower(print_menu());

  /* Choices available with or without a connected camera */
  switch(choice) {
    case 'r':
      qt_convert_image(NULL);
      goto menu;
    case 'v':
      qt_view_image(NULL);
      goto menu;
    case '0':
      goto out;
    default:
      break;
  }

  /* Choices available only with a camera connected */
  if (camera_connected) {
    /* The only possible choice for QT200 */
    if (choice == 'g') {
      get_one_picture(num_pics);
    }
    /* Choices for QT1x0 */
    if (serial_model != QT_MODEL_200) {
      switch(choice) {
        case 'p':
          show_thumbnails(num_pics);
          break;
        case 'd':
          delete_pictures();
          break;
        case 's':
          take_picture();
          break;
        case 'n':
          set_camera_name(camera_name);
          break;
        case 't':
          set_camera_time();
          break;
        case 'q':
          qt_set_quality((current_quality == QUALITY_HIGH) ? QUALITY_STANDARD:QUALITY_HIGH);
          break;
        case 'f':
          qt_set_flash((current_flash_mode + 1) % 3);
          break;
        default:
          break;
      }
    }
  } else if (choice == 'c') {
    exec("slowtake", NULL);
  }

  goto menu;

out:
  free(camera_name);
  return 0;
}

#ifdef __CC65__
  #pragma static-locals(pop)
#endif
#pragma code-name(pop)
