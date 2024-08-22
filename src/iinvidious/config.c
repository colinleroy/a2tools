#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "malloc0.h"
#include "platform.h"
#include "surl.h"
#include "extended_conio.h"
#include "charsets.h"
#include "clrzone.h"
#include "config.h"
#include "dgets.h"

extern unsigned char scrw;
char *translit_charset;
char video_size;
char enable_subtitles;
char sub_language[3] = "en";
char tmp_buf[80];

static FILE *open_config(char *mode) {
  FILE *fp;
  #ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
  #endif
  fp = fopen("clisettings", mode);
  if (fp == NULL && mode[0] == 'w') {
    cputs("Could not open settings file.\r\n");
  }
  return fp;
}

static int save_config(void) {
  FILE *fp;
  int r;

  cputs("Saving config...\r\n");
  fp = open_config("w");
  if (fp == NULL) {
    return -1;
  }

  r = fprintf(fp, "%s\n%d\n%d\n%s\n",
                  translit_charset,
                  video_size,
                  enable_subtitles,
                  sub_language);

  if (r < 0 || fclose(fp) != 0) {
    cputs("Could not save settings file.\r\n");
    return -1;
  }
  return 0;
}

extern char tmp_buf[80];

static char get_bool(char one, char zero) {
  char c;
again:
  c = tolower(cgetc());
  if (c == zero)
    return 0;
  if (c == one)
    return 1;
  goto again;
}

void config(void) {
#ifdef __APPLE2ENH__
  char c;
#endif

#ifdef __APPLE2ENH__
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
      translit_charset = US_CHARSET;
      break;
    case '1':
      translit_charset = FR_CHARSET;
      break;
    case '2':
      translit_charset = ES_CHARSET;
      break;
    case '3':
      translit_charset = IT_CHARSET;
      break;
    case '4':
      translit_charset = DE_CHARSET;
      break;
    default:
      goto charset_again;
  }
#else
  translit_charset = US_CHARSET;
#endif

  cputs("\r\nVideo size (Small - more FPS / Large - less FPS)? (s/l)\r\n");
  video_size = get_bool('s' /* HGR_SCALE_HALF */, 'l' /* HGR_SCALE_FULL */);

  cputs("\r\nEnable subtitles? (y/n)\r\n");
  enable_subtitles = get_bool('y', 'n');

  if (enable_subtitles) {
    cputs("\r\nPreferred subtitles language code (two lowercase letters)? ");
    dget_text(sub_language, 3, NULL, 0);
    if (sub_language[0] == '\0')
      strcpy(sub_language, "en");
  }
  save_config();
}

#pragma code-name(push, "RT_ONCE")

void load_config(void) {
  FILE *fp;

  translit_charset = US_CHARSET;
  video_size = 0;
  enable_subtitles = 1;
  strcpy(sub_language, "en");

  cputs("Loading config...\r\n");
  fp = open_config("r");
  if (fp == NULL) {
    return;
  }

  if (fp != NULL) {
    fgets(tmp_buf, 16, fp);
    if (strchr(tmp_buf, '\n')) {
      *strchr(tmp_buf, '\n') = '\0';
    }
    translit_charset = strdup(tmp_buf);

    fgets(tmp_buf, 16, fp);
    video_size = (tmp_buf[0]-'0');

    fgets(tmp_buf, 16, fp);
    enable_subtitles = (tmp_buf[0] != '0');

    fgets(sub_language, 3, fp);

    fclose(fp);
  }
}
#pragma code-name(pop)
