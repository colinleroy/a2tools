#include <ctype.h>

#include "extended_conio.h"
#include "progress_bar.h"
#include "hgr.h"
#include "hgr_addrs.h"
#include "iw_print.h"
#include "simple_serial.h"
#include "vsdrive.h"

static uint8 serial_opened = 0, printer_opened = 0;
extern uint8 scrw;
/* As long as I can't cleanly handle both ports at once, toggle the opened
 * port depending on whether we want to print or access vsdrive. */
int iw_print_open_port(char port_num) {
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
  simple_serial_set_irq(0);
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

unsigned char print_setup_msg(void) {
  clrscr(); gotoxy(0, 20);
  cprintf("Please set your printer to %sbps, XON/XOFF, connect it to the\r\n"
          "printer port (slot %u) and turn it on.\r\n"
          "Press a key when ready (Esc to cancel)\r\n",
          tty_speed_to_str(ser_params.printer_baudrate), ser_params.printer_slot);
  if (cgetc() == CH_ESC)
    return -1;

  clrscr(); gotoxy(0, 20);
  cprintf("Printing\n");
  progress_bar(0, wherey(), scrw, 0, HGR_HEIGHT);

  return 0;
}

static char init_print(void) {
  init_hgr_base_addrs();
  if (iw_print_open_port(ser_params.printer_slot) != 0) {
    return -1;
  }
  hgr_mixon();
  return 0;
}

static void print_done(void) {
  clrscr();
  gotoxy(0, 20);
  cputs("Done.\r\n"
        "Press a key to continue.\r\n");
  cgetc();
}

void hgr_print(void) {
  uint8 scale = 1;

  if (init_print() != 0) {
    goto out;
  }

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
  print_done();

out:
  hgr_mixoff();
}

void dhgr_print(void) {
  if (init_print() != 0 || print_setup_msg() != 0)
    goto out;

  /* Send data */
  dhgr_to_iw();
  print_done();

out:
  hgr_mixoff();
}
