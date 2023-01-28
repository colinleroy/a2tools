#include <string.h>
#include "scrollwindow.h"
#include "logo.h"
#ifdef __CC65__
#include <conio.h>
#include "dputs.h"
#else
#include "extended_conio.h"
#endif

void print_logo(unsigned char scrw) {
  char *logo = 
    "   ***************   \r\n"
    " ***      *      *** \r\n"
    "***   ***   ***   ***\r\n"
    "***   ***   ***   ***\r\n"
    "***   *********   ***\r\n"
    " ******************* \r\n"
    "  ****************   \r\n"
    "   ****              \r\n"
    "     *********       \r\n"
    "                     \r\n"
    " WELCOME TO MASTODON.\r\n"
    "\r\n";

  set_hscrollwindow((scrw - 20) / 2, 25);
  gotoxy(0, 2);
  dputs(logo);
  set_hscrollwindow(0, scrw);
  dputs("\r\n");
}