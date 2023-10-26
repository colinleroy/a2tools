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

static void print_header(uint8 num_pics, uint8 left_pics, uint8 mode, const char *name) {
  gotoxy(0, 0);
  printf("%s connected\n%d photos taken, %d left, %s mode\n",
         name, num_pics, left_pics, qt_get_mode_str(mode));
  chline(scrw);
}

uint8 auto_convert = 1;

static uint8 print_menu(void) {
  gotoxy(0, 3);
  printf("1. Get one picture\n"
         "2. Get all pictures\n"
         "3. Convert a picture on floppy\n"
         "\n"
         "9. %s auto-conversion to HGR\n"
         "0. Exit\n\n",
       
          auto_convert ? "Disable":"Enable");
  return cgetc();
}

char filename[64] = "/";

static void save_picture(uint8 n_pic) {
  printf("Filename: ");
  if (strchr(filename, '/')) {
    *(strrchr(filename, '/') + 1) = '\0';
  }
  dget_text(filename, 60, NULL, 0);

  if (!strchr(filename, '.')) {
    strcat(filename, ".QTK");
  }

  qt_get_picture(n_pic, filename);

  if (!auto_convert) {
    char c;
    printf("Convert to HGR? [Y/n] ");
    c = tolower(cgetc());
    if (c != 'n') {
      convert_image(filename);
    }
  }
}

static void get_all_pictures(uint8 num_pics) {
  uint8 n_pic;
  for (n_pic = 1; n_pic <= num_pics; n_pic++) {
    save_picture(n_pic);
  }
}

static void get_one_picture(uint8 num_pics) {
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
  save_picture(n_pic);
}

int main (void)
{
  uint8 num_pics, left_pics, mode, choice;
  char *name;

  register_start_device();

#ifdef __CC65__
  videomode(VIDEOMODE_80COL);
  screensize(&scrh, &scrw);
#endif

  qt_serial_connect();

  qt_get_information(&num_pics, &left_pics, &mode, &name);

again:
  clrscr();
  print_header(num_pics, left_pics, mode, name);

  choice = print_menu();
  switch(choice) {
    case '1':
      get_one_picture(num_pics);
      break;
    case '2':
      get_all_pictures(num_pics);
      break;
    case '3':
      printf("Not implemented\n");
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
