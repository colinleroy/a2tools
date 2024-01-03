#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "extended_conio.h"
#include "platform.h"
#include "file_select.h"
#include "hgr.h"
#include "hgr_addrs.h"
#include "path_helper.h"
#include "progress_bar.h"
#include "scrollwindow.h"
#include "simple_serial.h"

uint8 scrw, scrh;
#ifndef __CC65__
char HGR_PAGE[HGR_LEN];
#endif

static uint8 serial_opened = 0;

/* Check for XON/XOFF */
uint8 wait_imagewriter_ready(void) {
#ifdef __CC65__
  #define XOFF 0x13
  #define XON  0x11
  int c;

  if ((c = simple_serial_getc_immediate()) != EOF) {
    if (c == XOFF) {
      uint8 tries = 5;
      /* Wait for XON */
      while (tries && (c = simple_serial_getc_with_timeout()) == EOF)
        tries--;
      if (c == XON) {
        return 0;
      }
      return -1;
    }
  }
#endif
  return 0;
}

void hgr_print(void) {
  uint16 x;
  uint8 y, cy, ey, bit;
  uint8 c;
  char setup_binary_print_cmd[] = {CH_ESC, 'n', CH_ESC, 'T', '1', '6'};
  char send_chars_cmd[8]; // = {CH_ESC, 'G', '0', '0', '0', '0'};
#ifdef __CC65__
  #define cur_d7 zp6p
  #define cur_m7 zp8p
#else
  uint8 *cur_d7, *cur_m7;
#endif
  uint16 sx, ex;
  uint8 scale = 1;

  init_hgr_base_addrs();

  hgr_mixon();
  clrscr();
  gotoxy(0, 20);

  cputs("Printout scale: \r\n"
        "1. 72dpi (9x6.7cm)\r\n"
        "2. 36dpi (18x13cm)\r\n"
        "Escape: cancel printing");
scale_again:
  scale = cgetc();
  if (scale == CH_ESC) {
    goto out;
  }
  scale -= '0';
  if (scale != 1 && scale != 2) {
    goto scale_again;
  }
  clrscr(); gotoxy(0, 20);
  cputs("Please connect ImageWriter II to the printer port and turn it on.\r\n"
        "Press a key when ready...\r\n");
  cgetc();

  if (!serial_opened) {
    serial_opened = (simple_serial_open_slot(PRINTER_SER_SLOT) == 0);
  }
  if (!serial_opened) {
    cputs("Could not open serial port.\r\n");
    cgetc();
    goto out;
  }
#ifdef __CC65__
  simple_serial_set_speed(SER_BAUD_9600);
  simple_serial_set_flow_control(SER_HS_NONE);
#else
  simple_serial_set_speed(B9600);
#endif

  /* Calculate X boundaries */
  for (sx = 0; sx < 100; sx++) {
    cur_d7 = div7_table + sx;
    cur_m7 = mod7_table + sx;
    for (y = 0; y < HGR_HEIGHT; y++) {
      /* Do we have a white pixel? */
      if ((*(hgr_baseaddr[y] + *cur_d7) & *cur_m7) != 0)
        goto found_start;

    }
  }
  found_start:
  if (sx == 100)
    sx = 0;

  for (ex = HGR_WIDTH-1; ex > 180; ex--) {
    cur_d7 = div7_table + ex;
    cur_m7 = mod7_table + ex;
    for (y = 0; y < HGR_HEIGHT; y++) {
      /* Do we have a white pixel? */
      if ((*(hgr_baseaddr[y] + *cur_d7) & *cur_m7) != 0)
        goto found_end;

    }
  }
  found_end:
  if (ex == 180)
    ex = HGR_WIDTH;

  cprintf("Printing region %d-%d\r\n", sx, ex);

  /* Set line width */
  sprintf(send_chars_cmd, "%cG%04d", CH_ESC, (ex-sx) * scale);

  for (y = 0; y < HGR_HEIGHT; y += (8/scale)) {
    cur_d7 = div7_table + sx;
    cur_m7 = mod7_table + sx;
    ey = y + (8/scale);
    /* Set printer to binary mode */
    simple_serial_write(setup_binary_print_cmd, sizeof(setup_binary_print_cmd));
    simple_serial_write(send_chars_cmd, 6);
    for (x = sx; x < ex; x++) {
      c = 0;
      bit = (scale == 1) ? 0x1 : 0x3;
      for (cy = y; cy < ey; cy++) {
        if ((*(hgr_baseaddr[cy] + *cur_d7) & *cur_m7) == 0) {
          c |= bit;
        }
        bit <<= scale;
      }
      cur_d7++;
      cur_m7++;
      /* We only check if the printer is ready inside the main loop,
       * because we can send the few setup extra bytes in any case.
       */
      if (wait_imagewriter_ready() != 0) {
        goto err_out;
      }
      simple_serial_write(&c, 1);
      if (scale == 2)
        simple_serial_write(&c, 1);
    }
    simple_serial_write("\r\n", 2);
  }
err_out:
  clrscr(); gotoxy(0, 20);
  cputs("Done.\r\n"
        "Press a key to continue.\r\n");
  cgetc();
out:
  hgr_mixoff();
}

int main(int argc, char *argv[]) {
  FILE *fp = NULL;
  static char imgname[FILENAME_MAX];
  uint16 len;
  uint8 i;
  static char cmdline[127];
  #define BLOCK_SIZE 512
  const char *filename = NULL;
  register_start_device();

  videomode(VIDEOMODE_80COL);
  screensize(&scrw, &scrh);
#ifndef __CC65__
  scrw = 80; scrh = 24;
#endif

  set_scrollwindow(0, scrh);
  clrscr();

  if (argc > 1) {
    if (strcmp(argv[1], "___SEL___"))
      filename = argv[1];
  }

  cputs("Image view\r\n\r\n");
  if (!filename) {
    char *tmp;
    cputs("Image (HGR): ");
    tmp = file_select(wherex(), wherey(), scrw - wherex(), wherey() + 10, 0, "Select an HGR file");
    if (tmp == NULL)
      return -1;
    strcpy(imgname, tmp);
    free(tmp);
  } else {
    strcpy(imgname, filename);
  }

  fp = fopen(imgname, "rb");
  if (fp == NULL) {
    cputs("Can not open image.\r\n");
    cgetc();
    return -1;
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

  fclose(fp);

again:
  i = tolower(cgetc());
  if (i == 'p') {
    hgr_print();
    goto again;
  }

  init_text();

  if (argc > 2) {
    cmdline[0] = '\0';
    for (i = 3; i < argc; i++) {
      strcat(cmdline, argv[i]);
      strcat(cmdline, " ");
    }
    exec(argv[2], cmdline);
  }
  return 0;
}
