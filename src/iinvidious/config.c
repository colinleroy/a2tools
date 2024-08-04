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

extern unsigned char scrw;
char *translit_charset;
char video_size;
char tmp_buf[80];

#pragma code-name(push, "LOWCODE")

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

  r = fprintf(fp, "%s\n%d\n",
                  translit_charset,
                  video_size);

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

  clrzone(0, PAGE_BEGIN, scrw - 1, PAGE_BEGIN + PAGE_HEIGHT);

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
  video_size = get_bool('l', 's');

  save_config();
}

#pragma code-name(pop)
#pragma code-name(push, "RT_ONCE")

void load_config(void) {
  FILE *fp;

  translit_charset = US_CHARSET;
  video_size = 0;

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
#ifdef __APPLE2ENH__
    translit_charset = strdup(tmp_buf);
#endif

    fgets(tmp_buf, 16, fp);
#ifdef __APPLE2ENH__
    video_size = (tmp_buf[0]-'0');
#endif

    fclose(fp);
  }
}
#pragma code-name(pop)
