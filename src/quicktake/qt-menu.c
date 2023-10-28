#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dgets.h"
#include "dputc.h"
#include "extended_conio.h"
#include "path_helper.h"

#include "qt-conv.h"
#include "qt-serial.h"

#ifdef __CC65__
  #pragma static-locals(push, on)
#endif

uint8 scrw, scrh;

#define BUF_SIZE 64
static char imgname[BUF_SIZE];
char magic[5] = "????";

static void convert_image(const char *filename) {
  if (!filename) {
    cputs("Enter image to convert: ");
    dget_text(imgname, BUF_SIZE, NULL, 0);
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
         "3. Get one thumbnail\n"
         "4. Get all thumbnails\n"
         "5. Convert a picture on floppy\n"
         "\n"
         "7. Set camera name\n"
         "8. Set camera time\n"
         "9. %s auto-conversion to HGR\n"
         "0. Exit\n\n",
       
          auto_convert ? "Disable":"Enable");
  return cgetc();
}

char filename[64] = "/";

static void save_picture(uint8 n_pic, uint8 full) {
  printf("Filename: ");
  if (strchr(filename, '/')) {
    *(strrchr(filename, '/') + 1) = '\0';
  }
  dget_text(filename, 60, NULL, 0);

  if (!strchr(filename, '.')) {
    strcat(filename, ".QTK");
  }

  qt_get_picture(n_pic, filename, full);

  if (!auto_convert) {
    char c;
    printf("Convert to HGR? [Y/n] ");
    c = tolower(cgetc());
    if (c != 'n') {
      convert_image(filename);
    }
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
  printf("Picture number (1-%d)? ", num_pics);
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

  printf("Name: ");
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
    printf("%s: ", names[i]);
    dget_text(buf, 5, NULL, 0);
    vals[i] = (uint8)(atoi(buf) % 100);
  }
  printf("set time to %d/%d/%d %d:%d\n", vals[0], vals[1], vals[2], vals[3], vals[4]);

  qt_set_camera_time(vals[0], vals[1], vals[2], vals[3], vals[4], 0);
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

  while (qt_serial_connect() != 0) {
    char c;
    printf("Try again? [Y/n] ");
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
      get_one_picture(num_pics, 0);
      break;
    case '4':
      get_all_pictures(num_pics, 0);
      break;
    case '5':
      printf("Not implemented\n");
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
