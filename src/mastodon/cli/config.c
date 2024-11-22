#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "malloc0.h"
#include "platform.h"
#include "surl.h"
#include "extended_conio.h"
#include "api.h"
#include "logo.h"
#include "charsets.h"
#include "citoa.h"

extern char *instance_url;
extern char *oauth_token;
static unsigned char scrw, scrh;

static int save_config(char *charset, char monochrome) {
  FILE *fp;
  int r;

#ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
#endif

  cputs("Saving config...\r\n");

  fp = fopen("clisettings", "w");
  if (IS_NULL(fp)) {
    cputs("Could not open settings file.\r\n");
    return -1;
  }

  r = fprintf(fp, "%s\n%d\n",
                  charset, monochrome);

  if (r < 0 || fclose(fp) != 0) {
    cputs("Could not save settings file.\r\n");
    return -1;
  }
  return 0;
}

static void put_logo (void) {
  clrscr();
  gotoxy(0, 0);

  print_logo(scrw);
}

static void cli() {
  char c, monochrome;
  char *charset;

  put_logo();

#ifdef __APPLE2ENH__
  cputs("Please choose your keyboard layout:");
  for (c = 0; c < N_CHARSETS; c++) {
    cputs("\r\n"); cutoa(c); cputs(". ");cputs(charsets[c]);
  }
  cputs("\r\n");
  
charset_again:
  c = cgetc();
  if (c >= '0' && c < '0'+N_CHARSETS) {
    charset = charsets[c-'0'];
  } else {
    goto charset_again;
  }
#else
  charset = US_CHARSET;
#endif
  put_logo();
  cputs("\r\nIs your monitor monochrome? (y/n)\r\n");
monochrome_again:
  c = cgetc();
  switch(tolower(c)) {
    case 'y':
      monochrome = 1;
      break;
    case 'n':
      monochrome = 0;
      break;
    default:
      goto monochrome_again;
  }

  save_config(charset, monochrome);
}

int conf_main(int argc, char **argv) {
  char *params = malloc0(127);

  if (argc < 3) {
    cputs("Missing instance_url and/or oauth_token parameters.\n");
  }

#ifdef __APPLE2ENH__
  videomode(VIDEOMODE_80COL);
#endif
  screensize(&scrw, &scrh);

  instance_url = argv[1];
  oauth_token = argv[2];
  cli();

  snprintf(params, 127, "%s %s", instance_url, oauth_token);

#ifdef __CC65__
  exec("mastocli", params);
#else
  printf("exec(mastocli %s)\n",params);
#endif
  exit(0);
}
