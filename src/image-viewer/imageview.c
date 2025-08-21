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
#include "iw_print.h"

uint8 scrw, scrh;
#ifndef __CC65__
char HGR_PAGE[HGR_LEN];
#endif

static uint8 serial_opened = 0, printer_opened = 0;

/* As long as I can't cleanly handle both ports at once, toggle the opened
 * port depending on whether we want to print or access vsdrive. */
static int open_port(char port_num) {
  static uint8 already_asked = 0;
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
    if (!serial_opened && !already_asked) {
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
  already_asked = 1;
  return -1;
}

uint8 is_dhgr = 0;

unsigned char print_setup_msg(void) {
  clrscr(); gotoxy(0, 20);
  cprintf("Please set your printer to %sbps, XON/XOFF, connect it to the\r\n"
          "printer port (slot %u) and turn it on.\r\n"
          "Press a key when ready (Esc to cancel)\r\n",
          tty_speed_to_str(ser_params.printer_baudrate), ser_params.printer_slot);
  if (cgetc() == CH_ESC)
    return -1;

  clrscr(); gotoxy(0, 20);
  cprintf("Printing ");
  progress_bar(wherex(), wherey(), scrw, 0, HGR_HEIGHT);

  return 0;
}

void hgr_print(void) {
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

  if (print_setup_msg() != 0)
    goto out;

  hgr_to_iw(scale);

  clrscr(); gotoxy(0, 20);
  cputs("Done.\r\n"
        "Press a key to continue.\r\n");
  cgetc();
out:
  hgr_mixoff();
}

void dhgr_print(void) {
  init_hgr_base_addrs();

  if (open_port(ser_params.printer_slot) != 0) {
    goto out;
  }

  hgr_mixon();

  if (print_setup_msg() != 0)
    goto out;

  /* Send data */
  dhgr_to_iw();

  clrscr(); gotoxy(0, 20);
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
  cputs("P: Print to IW II - Space: view next\r\n"
        "Esc: exit - Any other key: toggle help.");
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
  uint8 monochrome = 0;

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
  close(fd);

redisplay:
  #ifdef __CC65__
  init_graphics(monochrome, is_dhgr);
#endif
again:
  i = tolower(cgetc());
  switch(i) {
    case 'c':
      monochrome = !monochrome;
      goto redisplay;
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
