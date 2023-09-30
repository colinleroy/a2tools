#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#ifdef __APPLE2ENH__
#include <apple2enh.h>
#else
  #ifdef __APPLE2__
  #include <apple2.h>
  #endif
#endif
#include "surl.h"
#include "extended_conio.h"
#include "strsplit.h"
#include "api.h"
#include "logo.h"

char *instance_url;
char *oauth_token;
static unsigned char scrw, scrh;

static int save_config(char *charset, char monochrome) {
  FILE *fp;
  int r;

#ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
#endif

  cputs("Saving config...\r\n");

  fp = fopen("clisettings", "w");
  if (fp == NULL) {
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

static void cli() {
  char c, monochrome;
  char *charset;

  clrscr();
  gotoxy(0, 0);

  print_logo(scrw);

  cputs("Please choose your keyboard layout:\r\n");
  cputs("0. US      ("US_CHARSET" charset)\r\n");
  cputs("1. French  ("FR_CHARSET" charset)\r\n");
  cputs("2. Spanish ("ES_CHARSET" charset)\r\n");
  cputs("3. Italian ("IT_CHARSET" charset)\r\n");
  cputs("4. German  ("DE_CHARSET" charset)\r\n");
  
charset_again:
  c = cgetc();
  switch(c) {
    case '0':
      charset = US_CHARSET;
      break;
    case '1':
      charset = FR_CHARSET;
      break;
    case '2':
      charset = ES_CHARSET;
      break;
    case '3':
      charset = IT_CHARSET;
      break;
    case '4':
      charset = DE_CHARSET;
      break;
    default:
      goto charset_again;
  }
  
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

int main(int argc, char **argv) {
  char *params = malloc(BUF_SIZE);

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

  snprintf(params, BUF_SIZE, "%s %s", instance_url, oauth_token);

#ifdef __CC65__
  exec("mastocli", params);
#else
  printf("exec(mastocli %s)\n",params);
#endif
  exit(0);
}
