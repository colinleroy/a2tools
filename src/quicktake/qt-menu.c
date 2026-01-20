#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/statvfs.h>

#include "dget_text.h"
#include "extended_conio.h"
#include "file_select.h"
#include "hgr.h"
#include "path_helper.h"
#include "progress_bar.h"
#include "scrollwindow.h"
#include "simple_serial.h"
#include "clrzone.h"
#include "splash.h"
#include "qt-conv.h"
#include "qt-edit-image.h"
#include "qt-serial.h"
#include "qt-state.h"
#include "runtime_once_clean.h"
#include "a2_features.h"

uint8 scrw, scrh;
uint8 camera_connected;
camera_info cam_info;
static unsigned char exec_pass = 0;

#ifdef __CC65__
  #pragma static-locals(push, on)
#endif

char magic[5] = "????";

#define WELCOME_STR "Welcome to Quicktake for Apple II - (c) Colin Leroy-Mira, https://colino.net\r\n"

static void print_header(void) {
  gotoxy(0, 0);
  if (camera_connected) {
    cprintf("%s connected - %d%% battery%s - %02d/%02d/%04d %02d:%02d\r\n"
           "%d photos taken, %d left, %s, %s flash - ",
          cam_info.name, cam_info.battery_level, cam_info.charging? " (charging)":"",
          cam_info.date.day, cam_info.date.month, cam_info.date.year,
          cam_info.date.hour, cam_info.date.minute,
          cam_info.num_pics, cam_info.left_pics, qt_get_quality_str(cam_info.quality_mode),
          qt_get_flash_str(cam_info.flash_mode));
  } else {
    cputs("No camera connected\r\n");
  }
#ifdef __CC65__
  cprintf("Free RAM: %zuB\r\n", _heapmemavail());
#endif
  chline(scrw);
}

static uint8 print_menu(void) {
  cputs("Menu\r\n\r\n");
  if (camera_connected) {
    cputs(" G. Get one picture\r\n");
    if (serial_model != QT_MODEL_200) {
      cputs(" P. Preview pictures\r\n"
            " D. Delete all pictures\r\n"
            " S. Snap a picture\r\n");
    }
  } else {
    cputs(" C. Connect camera\r\n");
  }
  cputs(  " R. Re-edit a raw picture from floppy\r\n"
           " V. View a converted picture from floppy\r\n");
  if (camera_connected  && serial_model != QT_MODEL_200) {
    cprintf(" N. Set camera name\r\n"
           " T. Set camera time\r\n"
           " Q. Set quality to %s\r\n"
           " F. Set flash to %s\r\n",
           qt_get_quality_str((cam_info.quality_mode == QUALITY_HIGH) ? QUALITY_STANDARD:QUALITY_HIGH),
           qt_get_flash_str((cam_info.flash_mode + 1) % 3));
  }
  cputs(   "\r\n"
           " A. About this program\r\n"
           " 0. Exit\r\n\r\n");
  return cgetc();
}

#pragma code-name(push, "LOWCODE")

static void save_picture(uint8 n_pic) {
  char filename[64];
  struct statvfs sv;
  char *dirname;
  int fd, saved_errno;

  filename[0] = '\0';
#ifdef __CC65__
again:
  clrscr();
  cprintf("Saving picture %d\r\n\r\n"

        "Make sure to save the picture to a floppy with enough free space.\r\n"
        "\r\n"
        "Do not use /RAM, which will be used for temporary storage.\r\n\r\n"
        "Please swap disks if needed and press a key.\r\n\r\n",
      n_pic);
  cgetc();

  dirname = file_select(1, "Select directory");
  if (dirname == NULL) {
    return;
  }
  if (!strcmp(dirname, "/RAM") || !strncmp(dirname, "/RAM/", 5)) {
    cputs("\r\nNot enough space available.");
    cgetc();
    goto again;
  }
  gotox(0);
  cputs("Enter filename: ");
  strcpy(filename, dirname);
  strcat(filename, "/");
  free(dirname);

  dget_text_single(filename, 60, NULL);
#else
  if (serial_model == QT_MODEL_200)
    sprintf(filename, "image%02d.jpg", n_pic);
  else
    sprintf(filename, "image%02d.qtk", n_pic);
#endif

  if (filename[0] == '\0' || filename[strlen(filename) - 1] == '/')
    return;

  fd = open(filename, O_RDONLY);
  if (fd > 0) {
    char c;
    close(fd);
    cputs("File exists. Overwrite? (y/N)\r\n");
    c = tolower(cgetc());
    if (c != 'y') {
      return;
    }
    unlink(filename);
  }

#ifdef __CC65__
  if (statvfs(filename, &sv) != 0) {
    goto err_io;
  }
#else
  sv.f_bfree=1024;
  sv.f_bsize=512;
#endif

  fd = open(filename, O_WRONLY|O_CREAT);
  if (fd <= 0) {
    goto err_io;
  }

  if (qt_get_picture(n_pic, fd, sv.f_bfree * sv.f_bsize) == 0) {
    uint16 tmp = n_pic;
    close(fd);
    state_set(STATE_GET, tmp, NULL);
    exec_pass = 1;
    qt_convert_image(filename);
  } else {
    saved_errno = errno;
    close(fd);
    unlink(filename);
    errno = saved_errno;
err_io:
    cputs("  Error saving picture: ");
    cputs(strerror(errno));
    cputs("\r\n");
    cgetc();
  }
}

#pragma code-name(pop)

static void get_one_picture(uint8 num_pics) {
#ifdef __CC65__
  char buf[5];
#else
  char buf[20];
#endif
  uint8 n_pic;
  uint16 tmp;

  clrscr();

  cputs("Get a picture from the camera\r\n\r\n"

        "Picture number? ");

  buf[0] = '\0';

  if (state_load(STATE_GET, &tmp, NULL) == 0) {
    if (tmp < num_pics)
      sprintf(buf, "%u", tmp + 1);
  }
  dget_text_single(buf, 4, NULL);

  if (buf[0] == '\0')
    return;

  n_pic = atoi(buf);
  if (n_pic < 1 || n_pic > num_pics) {
    cprintf("No image %d in camera.\r\n"
           "Please press a key...", n_pic);
    cgetc();
    return;
  }
  save_picture(n_pic);
}

static void set_camera_name(const char *name) {
  char buf[32];

  if (name == NULL) {
    return;
  }

  clrscr();
  cputs("Camera renaming\r\n\r\n"

        "New camera name: ");

  strncpy(buf, name, 31);
  dget_text_single(buf, 31, NULL);

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
    dget_text_single(buf, 5, NULL);
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

void clear_dhgr(void);

static void show_thumbnails(uint8 num_pics) {
  int8 i = 1;
  uint16 tmp;
  thumb_info info;
  char c = 0;
  char thumb_buf[32];
  int fd;

  if (num_pics == 0) {
    return;
  }

  init_graphics(1, 0);
  hgr_mixon();

  if (state_load(STATE_PREVIEW, &tmp, NULL) == 0) {
    i = tmp;
  }

  do {
    set_scrollwindow(0, scrh);

    fd = open(THUMBNAIL_NAME, O_WRONLY|O_CREAT);
    if (fd <= 0) {
      goto err_thumb_io;
    }

    clrscr();
    gotoxy(0,20);

    c = qt_get_thumbnail(i, fd, &info);
    close(fd);

    if (c != 0) {
err_thumb_io:
      cputs("\r\nError getting thumbnail.\r\n");
      cgetc();
      break;
    }

    tmp = i;
    state_set(STATE_PREVIEW, tmp, NULL);
    sprintf(thumb_buf, "Thumbnail %d", i);
    clear_dhgr();
    qt_edit_image(thumb_buf, THUMB_WIDTH*2, serial_model);

    clrscr();
    gotoxy(0,20);

    cprintf("%s (%s, flash %s, %02d/%02d/%04d %02d:%02d)\r\n"
           "G: get full picture, Esc: exit, N: next thumbnail, P: previous thumbnail",
           thumb_buf, qt_get_quality_str(info.quality_mode),
           info.flash_mode ? "on":"off", info.date.day, info.date.month,
           info.date.year, info.date.hour, info.date.minute);

get_key:
    c = tolower(cgetc());
    switch (c) {
      case 'n':
        i++;
        if (i > num_pics) {
          i = 1;
        }
        break;
      case 'p':
        i--;
        if (i == 0) {
          i = num_pics;
        }
        break;
      case 'g':
      case CH_ESC:
        goto done;
      default:
        goto get_key;
    }
  } while (1);
done:
  unlink(THUMBNAIL_NAME);

  if (c == 'g') {
    init_text();
    set_scrollwindow(0, scrh);
    save_picture(i);
  }
}

static void print_welcome(void) {
  init_graphics(1, 0);
  set_scrollwindow(20, scrh);
  hgr_mixon();
  clrscr();
  cputs(WELCOME_STR);
  set_scrollwindow(21, scrh);
}

static void show_about(void) {
  int fd;
  size_t r;

  reopen_start_device();
  fd = open("about", O_RDONLY);
  if (fd <= 0) {
    return;
  }
  set_scrollwindow(0, scrh);
  clrscr();
  while((r = read(fd, (char *)buffer, sizeof(buffer) - 1))) {
    buffer[r] = '\0';
    cputs((char *)buffer);
  }
  close(fd);
  cgetc();
}

#pragma code-name(push, "RT_ONCE")

static uint8 setup(int argc, char *argv[]) {
  uint16 is_reedit = 0;
  char *reedit_name;

#ifndef __CC65__
  uint16 target_speed = 57600U;
  scrw = 80; scrh = 24;
#else
  uint16 target_speed = 19200U;
#endif
  register_start_device();

#ifdef __CC65__
  try_videomode(VIDEOMODE_80COL);
#endif
if (is_iigs) {
  target_speed = 57600U;
}
// Start decoding right away when debugging decoders
#ifdef DEBUG_HD
  if (argc == 1) {
  #if DEBUG_HD==100
  exec("QTKTCONV","/HD/TEST100.QTK 0 0 640 480");
  #endif
  #if DEBUG_HD==150
  exec("QTKNCONV","/HD/TEST150.QTK 0 0 640 480");
  #endif
  #if DEBUG_HD==200
  exec("JPEGCONV","/HD/TEST200.JPG 0 0 640 480");
  #endif
  }
#endif
#ifdef DEBUG_FLOPPY
  if (argc == 1) {
  #if DEBUG_FLOPPY==100
  exec("QTKTCONV","/QT100/TEST100.QTK 0 0 640 480");
  #endif
  #if DEBUG_FLOPPY==98
  exec("QTKTCONV","/QT100/TEST100.QTK 0 0 640 480");
  #endif
  #if DEBUG_FLOPPY==150
  exec("QTKNCONV","/QT150/TEST150.QTK 0 0 640 480");
  #endif
  #if DEBUG_FLOPPY==200
  exec("JPEGCONV","/QT200/TEST200.JPG 0 0 640 480");
  #endif
  }
#endif

  screensize(&scrw, &scrh);
  print_welcome();

  if (state_load(STATE_EDIT, &is_reedit, &reedit_name) == 0) {
      qt_edit_image(reedit_name, is_reedit, QT_MODEL_UNKNOWN);
      state_set(STATE_EDIT, 0, NULL);
  } else if (argc == 3) {
    is_reedit = atoi(argv[2]);
    if (is_reedit)
      qt_edit_image(argv[1], is_reedit, QT_MODEL_UNKNOWN);
  }

  /* Remove temporary files */
  unlink(TMP_NAME);

  while (qt_serial_connect(target_speed) != 0) {
    char c;

    cprintf("Please turn the Quicktake off and on. Try again?\r\n");
    if (target_speed != 9600)
      cprintf("Y: try at %ubps, 9: try at 9600bps, C: configure, N: don't try (Y/9/c/n)\r\n", target_speed);
    else
      cprintf("Y: try at %ubps, C: configure, N: don't try (Y/c/n)\r\n", target_speed);

    c = tolower(cgetc());
    if (c == 'n')
      return 0;
    print_welcome();
    if(c == '9')
      target_speed = 9600;
    if(c == 'c') {
      init_text();
      set_scrollwindow(0, scrh);
      clrscr();
      simple_serial_configure();
      print_welcome();
    }
  }
  return 1;
}

#pragma code-name(pop)

void unlink_temp_files(void) {
  unlink(TMP_NAME);
  unlink(THUMBNAIL_NAME);
  if (!exec_pass) {
    state_unlink();
  }
  /* Don't unlink AUXHGR, as we want *conv to start writing GREY
   * *after* that file. */
}

#pragma code-name(push, "LOWCODE")

int main(int argc, char *argv[])
{
  uint8 choice;

  if (!has_128k) {
    cputs("This program requires 128kB of memory.");
    cgetc();
    exit(1);
  }

  reserve_auxhgr_file();
  atexit(&unlink_temp_files);
  camera_connected = setup(argc, argv);
menu:
  init_text();
  set_scrollwindow(0, scrh);
  clrscr();

  if (camera_connected) {
    free(cam_info.name);
    cam_info.name = NULL;
    if (qt_get_information(&cam_info) != 0) {
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
    case 'a':
      show_about();
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
      get_one_picture(cam_info.num_pics);
    }
    /* Choices for QT1x0 */
    if (serial_model != QT_MODEL_200) {
      switch(choice) {
        case 'p':
          show_thumbnails(cam_info.num_pics);
          break;
        case 'd':
          delete_pictures();
          break;
        case 's':
          take_picture();
          break;
        case 'n':
          set_camera_name(cam_info.name);
          break;
        case 't':
          set_camera_time();
          break;
        case 'q':
          qt_set_quality((cam_info.quality_mode == QUALITY_HIGH) ? QUALITY_STANDARD:QUALITY_HIGH);
          break;
        case 'f':
          qt_set_flash((cam_info.flash_mode + 1) % 3);
          break;
        default:
          break;
      }
    }
  } else if (choice == 'c') {
    exec_pass = 1;
    exec("slowtake", NULL);
  }

  goto menu;

out:
  free(cam_info.name);
  cam_info.name = NULL;
  state_unlink();
  return 0;
}

#ifdef __CC65__
  #pragma static-locals(pop)
#endif
#pragma code-name(pop)
