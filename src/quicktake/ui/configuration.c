#include <conio.h>
#include <ctype.h>
#include <string.h>

#include "a2_features.h"
#include "simple_serial.h"
#include "path_helper.h"
#include "platform.h"
#include "scrollwindow.h"
#include "zexec.h"

typedef struct _cam_driver {
  char *driver_name;
  char *camera_name;
} cam_driver;

static cam_driver cameras[] = {
  {"QT1X0.ZX", "Apple Quicktake 100"},
  {"QT1X0.ZX", "Apple Quicktake 150"},

  {"FUJI.ZX",  "Apple Quicktake 200"},
  {"FUJI.ZX",  "Fujifilm DS-7"},
  {"FUJI.ZX",  "Fujifilm DX-8"},

  {"DC50.ZX",  "Kodak DC50 Zoom"},

  {"SRRA.ZX",  "Epson PhotoPC PCDC001"},
  {"SRRA.ZX",  "Sanyo VPC-G1"},
  {"SRRA.ZX",  "Sanyo VPC-G200"},
  {"SRRA.ZX",  "Sierra Imaging SD640"},

  {NULL, NULL}
};

static void camera_configure(void) {
  int8 i;
  int8 max_driver_num = 0;
  unsigned char c;

  clrscr();
  for (i = 0; cameras[i].driver_name; i++) {
    gotoxy(2, i); cputs(cameras[i].camera_name);
  }
  max_driver_num = i - 1;

  i = 0;
  goto select;

  while ((c = tolower(cgetc())) != CH_ENTER) {
    gotoxy(0, i);
    cputc(' ');
    switch(c) {
      case CH_CURS_DOWN: i++; break;
      case CH_CURS_UP:   i--; break;
    }
    if (i < 0) {
      i = max_driver_num;
    }
    if (i > max_driver_num) {
      i = 0;
    }
select:
    gotoxy(0, i);
    cputc('>');
  }
  /* Copy the selected driver name to the ser_params struct */
  strcpy(ser_params.extra_parameters, cameras[i].driver_name);
}

void main(int argc, char *argv) {
  if (!has_128k) {
    cputs("This program requires 128kB of memory.");
    cgetc();
    exit(1);
  }

  register_start_device();

#ifdef __CC65__
  try_videomode(VIDEOMODE_80COL);
  if (!has_80cols) {
    cputs("This program requires 80 columns.");
    cgetc();
    exit(1);
  }
#endif

  clrscr();

  simple_serial_read_config();

  /* Configure when forced (by param) or no config exists */
  if (argc > 1 || ser_params.extra_parameters[0] == 0) {
    cputs("Quicktake for Apple II - configuration\r\n");
    chline(80);
    cputs("Please select your camera:\r\n");
    set_scrollwindow(3, 24);

    /* Camera selector */
    camera_configure();
    /* Serial ports configuration */
    simple_serial_configure();
    /* And save ! */
    simple_serial_write_config();
  }
  zxloader_name = "CONFIG.SYSTEM";
  zexec("SLOWTAKE");
}
