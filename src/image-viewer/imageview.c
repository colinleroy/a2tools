#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "extended_conio.h"
#include "platform.h"
#include "file_select.h"
#include "hgr.h"
#include "hgr_addrs.h"
#include "path_helper.h"
#include "progress_bar.h"
#include "scrollwindow.h"
#include "simple_serial.h"
#include "clrzone.h"
#include "vsdrive.h"
#include "a2_features.h"
#include "extrazp.h"

uint8 scrw, scrh;
#ifndef __CC65__
char HGR_PAGE[HGR_LEN];
#endif

static uint8 serial_opened = 0, printer_opened = 0;

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

/* As long as I can't cleanly handle both ports at once, toggle the opened
 * port depending on whether we want to print or access vsdrive. */
static int open_port(char port_num) {
  if (port_num == ser_params.printer_slot) {
    if (printer_opened) {
      return 0;
    }
    if (serial_opened) {
      simple_serial_close();
      vsdrive_uninstall();
      serial_opened = 0;
    }
  } else {
    if (serial_opened) {
      return 0;
    }
    if (printer_opened) {
      simple_serial_close();
      printer_opened = 0;
    }
  }

open_again:
  if (port_num == ser_params.printer_slot) {
    printer_opened = (simple_serial_open_printer() == 0);
    if (!printer_opened) {
      goto check_configure;
    }
  } else {
    serial_opened = (simple_serial_open() == 0);
    if (!serial_opened) {
      goto check_configure;
    } else {
      vsdrive_install();
    }
  }
  simple_serial_set_irq(1);
  return 0;

check_configure:
  printf("Could not open serial slot %d. Configure? (Y/n)\n", port_num);
  if (tolower(cgetc()) != 'n') {
    init_text();
    clrscr();
    simple_serial_configure();
    goto open_again;
  }
  return -1;
}

uint8 is_dhgr = 0;

void hgr_print(void) {
  uint16 x;
  uint8 y, cy, ey, bit;
  uint8 c;
  char setup_binary_print_cmd[] = {CH_ESC, 'n', CH_ESC, 'T', '1', '6'};
  char disable_auto_line_feed[] = {CH_ESC, 'Z', 0x80, 0x00};
#ifdef __CC65__
  char send_chars_cmd[8]; // = {CH_ESC, 'G', '0', '0', '0', '0'};
  #define cur_d7 zp6p
  #define cur_m7 zp8p
  #define line   zp10p
#else
  char send_chars_cmd[16];
  uint8 *cur_d7, *cur_m7, *line;
#endif
  uint16 sx, ex;
  uint8 scale = 1;

  init_hgr_base_addrs();

  if (open_port(ser_params.printer_slot) != 0) {
    goto out;
  }

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
  cprintf("Please set your ImageWriter II to %sbps, XON/XOFF, connect it to the \r\n"
          "printer port (slot %u) and turn it on.\r\n"
          "Press a key when ready or Escape to cancel...\r\n",
          tty_speed_to_str(ser_params.printer_baudrate), ser_params.printer_slot);
  if (cgetc() == CH_ESC)
    goto out;

  /* Calculate X boundaries */
  for (sx = 0; sx < 100; sx++) {
    cur_d7 = div7_table + sx;
    cur_m7 = mod7_table + sx;
    for (y = 0; y < HGR_HEIGHT; y++) {
      /* Do we have a white pixel? */
      line = (uint8 *)(hgr_baseaddr_l[y]|(hgr_baseaddr_h[y]<<8));
      if ((*(line + *cur_d7) & *cur_m7) != 0)
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
      line = (uint8 *)(hgr_baseaddr_l[y]|(hgr_baseaddr_h[y]<<8));
      if ((*(line + *cur_d7) & *cur_m7) != 0)
        goto found_end;

    }
  }
  found_end:
  if (ex == 180)
    ex = HGR_WIDTH;

  cprintf("Printing ");
  progress_bar(wherex(), wherey(), scrw, 0, HGR_HEIGHT);

  if (wait_imagewriter_ready() != 0) {
    goto err_out;
  }

  simple_serial_write(disable_auto_line_feed, sizeof(disable_auto_line_feed));

  /* Send blank lines as margin, because I'm tired of
   * waiting for https://github.com/OpenPrinting/libcupsfilters/pull/69 to
   * land in Raspbian. */
  simple_serial_write("\r\n\r\n\r\n", 6);

  /* Set line width; we'll send one byte per block of 1x8 pixels */
  sprintf(send_chars_cmd, "%cG%04d", CH_ESC, (ex-sx) * scale);

  /* Iterate on Y step 8, or 4 if we scale double */

  for (y = 0; y < HGR_HEIGHT; y += (8/scale)) {
    cur_d7 = div7_table + sx;
    cur_m7 = mod7_table + sx;
    ey = y + (8/scale);
    /* Set printer to binary mode */
    simple_serial_write(setup_binary_print_cmd, sizeof(setup_binary_print_cmd));
    simple_serial_write(send_chars_cmd, 6);

    /* Iterate on every X pixel (not HGR byte!) */
    for (x = sx; x < ex; x++) {
      c = 0;
      bit = (scale == 1) ? 0x1 : 0x3;
      /* Build the "vertical byte" from Y to Y+7 */
      for (cy = y; cy < ey; cy++) {
        line = (uint8 *)(hgr_baseaddr_l[cy]|(hgr_baseaddr_h[cy]<<8));
        if ((*(line + *cur_d7) & *cur_m7) == 0) {
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

      /* Send the vertical 1x8 block to IW */
      simple_serial_write((char *)&c, 1);
      /* If scaling double, send it twice */
      if (scale == 2)
        simple_serial_write((char *)&c, 1);
    }
    simple_serial_write("\r\n", 2);
    progress_bar(-1, -1, scrw, y, HGR_HEIGHT);
  }
err_out:
  clrscr();
  gotoxy(0, 20);
  cputs("Done.\r\n"
        "Press a key to continue.\r\n");
  cgetc();
out:
  hgr_mixoff();
}

void dhgr_print(void) {
  uint16 x;
  uint8 y, cy, ey, bit;
  char setup_binary_print_cmd[] = {CH_ESC, 'n', CH_ESC, 'T', '1', '6'};
  char disable_auto_line_feed[] = {CH_ESC, 'Z', 0x80, 0x00};
#ifdef __CC65__
  char send_chars_cmd[8]; // = {CH_ESC, 'G', '0', '0', '0', '0'};
  #define cur_d7 zp6p
  #define cur_m7 zp8p
  #define line   zp10p
#else
  char send_chars_cmd[16];
  uint8 *cur_d7, *cur_m7, *line;
#endif
  uint16 sx, ex;
  int fd;

  /* Copy back AUX DHGR page to main RAM for less awfulness
   * Drawback: requires 8kB RAM. */
  fd = open(hgr_auxfile, O_RDONLY);
  if (fd) {
    read(fd, (char *)0x4000, 0x2000);
    close(fd);
  } else {
    cputs("Error copying DHGR data.");
    cgetc();
    goto out;
  }

  init_hgr_base_addrs();

  if (open_port(ser_params.printer_slot) != 0) {
    goto out;
  }

  hgr_mixon();
  clrscr();
  gotoxy(0, 20);
  cprintf("Please set your ImageWriter II to %sbps, XON/XOFF, connect it to the \r\n"
          "printer port (slot %u) and turn it on.\r\n"
          "Press a key when ready or Escape to cancel...\r\n",
          tty_speed_to_str(ser_params.printer_baudrate), ser_params.printer_slot);
  if (cgetc() == CH_ESC)
    goto out;

  sx = 0;
  ex = HGR_WIDTH-1;

  cprintf("Printing ");
  progress_bar(wherex(), wherey(), scrw, 0, HGR_HEIGHT);

  if (wait_imagewriter_ready() != 0) {
    goto err_out;
  }

  simple_serial_write(disable_auto_line_feed, sizeof(disable_auto_line_feed));

  /* Send blank lines as margin, because I'm tired of
   * waiting for https://github.com/OpenPrinting/libcupsfilters/pull/69 to
   * land in Raspbian. */
  simple_serial_write("\r\n\r\n\r\n", 6);

  /* Set line width. We'll send 560 chars per line. */
  sprintf(send_chars_cmd, "%cG%04d", CH_ESC, 560);

  /* Send data */
  for (y = 0; y < HGR_HEIGHT; y += 4) {
    ey = y + 4;
    /* Set printer to binary mode */
    simple_serial_write(setup_binary_print_cmd, sizeof(setup_binary_print_cmd));
    simple_serial_write(send_chars_cmd, 6);
    cur_d7 = div7_table + sx;
    cur_m7 = mod7_table + sx;
    /* Here we iterate on blocks of 7x4 pixels, so that we
     * can fetch the pixel in main and aux a bit more easily */
    for (x = 0; x < 280; x+=7) {
      char main_block[7], aux_block[7];
      uint8 sub_byte;

      /* And get each HGR bit for this block */
      for (sub_byte = 0; sub_byte < 7; sub_byte++) {
        main_block[sub_byte] = aux_block[sub_byte] = 0;
        bit = 0x3; /* Make every pixel double height like on screen */
        for (cy = y; cy < ey; cy++) {
          /* Both in main */
          line = (uint8 *)(hgr_baseaddr_l[cy]|(hgr_baseaddr_h[cy]<<8));
          if ((*(line + *cur_d7) & *cur_m7) == 0) {
             main_block[sub_byte] |= bit;
          }
          /* And in aux */
          line += 0x2000;
          if ((*(line + *cur_d7) & *cur_m7) == 0) {
             aux_block[sub_byte] |= bit;
          }
          bit <<= 2;
        }
        cur_d7++;
        cur_m7++;
      }
      /* We only check if the printer is ready inside the main loop,
       * because we can send the few setup extra bytes in any case.
       */
      if (wait_imagewriter_ready() != 0) {
        goto err_out;
      }
      simple_serial_write(aux_block, 7);
      simple_serial_write(main_block, 7);
    }
    simple_serial_write("\r\n", 2);
    progress_bar(-1, -1, scrw, y, HGR_HEIGHT);
  }
err_out:
  clrscr();
  gotoxy(0, 20);
  cputs("Done.\r\n"
        "Press a key to continue.\r\n");
  cgetc();
out:
  hgr_mixoff();
}

static void get_next_image(char *imgname) {
#ifdef __CC65__
  char *filename = strrchr(imgname, '/');
  DIR *d;
  struct dirent *ent;
  uint8 found = 0, loop = 0;

  if (filename) {
    *filename = '\0';
  } else {
    return;
  }

  open_port(ser_params.data_slot);

  d = opendir(imgname);
  if (!d) {
    *filename = '/';
    return;
  }

start_from_first:
  while ((ent = readdir(d))) {
    if (found == 0 && !strcmp(ent->d_name, filename+1)) {
      found = 1;
      continue;
    }
    if (found == 1) {
      if (((ent->d_type == PRODOS_T_BIN && (ent->d_auxtype == 0x2000 || ent->d_auxtype == 0x0))
            || (ent->d_type == PRODOS_T_FOT && ent->d_auxtype < 0x4000))
        && ((ent->d_size <= 8192UL && ent->d_size >= 8184UL)
            || (ent->d_size <= 2*8192UL && ent->d_size >= 2*8184UL))) {
         /* this is, quite probably, an image */
         sprintf(filename, "/%s", ent->d_name);
         found = 2;
         break;
       }
    }
  }

  if (found < 2) {
    found = 1;
    rewinddir(d);
    loop++;
    if (loop == 1) {
      goto start_from_first;
    } else {
      /* Bail */
      *filename = '/';
    }
  }
  closedir(d);
#endif
}

static void print_help(void) {
  gotoxy(0, 20);
  cputs("P: Print to ImageWriter II - Space: view next image in directory\r\n"
        "Escape: exit - Any other key: toggle help.");
}

void unlink_page2(void) {
  unlink(hgr_auxfile);
}

int main(int argc, char *argv[]) {
  int fd, ramfd;
  static char imgname[FILENAME_MAX];
  uint16 len;
  uint8 i, tries = 0;
  static char cmdline[127];
  #define BLOCK_SIZE 512
  const char *filename = NULL;
  static char *rambuf[BLOCK_SIZE];
  register_start_device();

  try_videomode(VIDEOMODE_80COL);
  screensize(&scrw, &scrh);
#ifndef __CC65__
  scrw = 80; scrh = 24;
#endif

choose_again:
  init_text();
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

    open_port(ser_params.data_slot);
    tmp = file_select(0, "Select an HGR file");
    if (tmp == NULL)
      goto out;
    strcpy(imgname, tmp);
    free(tmp);
  } else {
    strcpy(imgname, filename);
  }

next_image:
  open_port(ser_params.data_slot);

  fd = open(imgname, O_RDONLY);
  if (fd <= 0) {
    init_text();
    cprintf("Can not open image %s (%s).\r\n", imgname, strerror(errno));
    cgetc();
    goto out;
  }

  memset((char *)HGR_PAGE, 0x0, HGR_LEN);
  init_text();
  gotoxy(0, 17);
  cprintf("Loading image %s...               ", imgname);
  print_help();
  progress_bar(0, 18, scrw, 0, HGR_LEN*2);

  len = 0;

  if (has_128k) {
    is_dhgr = (lseek(fd, 0, SEEK_END) == 2*HGR_LEN);
    lseek(fd, 0, SEEK_SET);
    if (is_dhgr && (ramfd = open(hgr_auxfile, O_WRONLY|O_CREAT)) > 0) {
      atexit(&unlink_page2);
      while (len < HGR_LEN) {
        read(fd, rambuf, BLOCK_SIZE);
        write(ramfd, rambuf, BLOCK_SIZE);

        len += BLOCK_SIZE;
        progress_bar(-1, -1, scrw, len, HGR_LEN*2);
      }
      close(ramfd);
    }
  }

  len = 0;
  while (len < HGR_LEN) {
    read(fd, (char *)(HGR_PAGE + len), BLOCK_SIZE);
    len += BLOCK_SIZE;
    progress_bar(-1, -1, scrw, len+HGR_LEN, HGR_LEN*2);
  }

  #ifdef __CC65__
  init_hgr(1);
  if (is_dhgr) {
    __asm__("sta $C05E"); //DHIRESON
  }
#endif

  close(fd);

again:
  i = tolower(cgetc());
  switch(i) {
    case 'p':
      if (is_dhgr)
        dhgr_print();
      else
        hgr_print();
      goto again;
    case CH_ESC:
      if (argc > 1 && strcmp(argv[1], "___SEL___"))
        goto out;
      else
        goto choose_again;
    case ' ':
      get_next_image(imgname);
      goto next_image;
    default:
      if (hgr_mix_is_on) {
        hgr_mixoff();
      } else{
        hgr_mixon();
        clrscr();
        print_help();
      }
      goto again;
  }

out:
  init_text();

  /* Pass back extra arguments received at startup */
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
