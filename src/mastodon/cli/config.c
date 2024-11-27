#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include "malloc0.h"
#include "platform.h"
#include "surl.h"
#include "extended_conio.h"
#include "api.h"
#include "logo.h"
#include "charsets.h"
#include "citoa.h"
#include "login_data.h"

void config_cli(void) {
  char c;

#ifdef __APPLE2ENH__
  clrscr();
  cputs("Please choose your keyboard layout:");
  for (c = 0; c < N_CHARSETS; c++) {
    cputs("\r\n"); cutoa(c); cputs(". ");cputs(charsets[c]);
  }
  cputs("\r\n");
  
charset_again:
  c = cgetc();
  if (c >= '0' && c < '0'+N_CHARSETS) {
    strcpy(login_data.charset, charsets[c-'0']);
  } else {
    goto charset_again;
  }
#else
  strcpy(login_data.charset, US_CHARSET);
#endif

  clrscr();
  cputs("Is your monitor monochrome? (y/n)\r\n");
monochrome_again:
  c = cgetc();
  switch(tolower(c)) {
    case 'y':
      login_data.monochrome = 1;
      break;
    case 'n':
      login_data.monochrome = 0;
      break;
    default:
      goto monochrome_again;
  }
}
