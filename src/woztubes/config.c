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
#include "dget_text.h"
#include "citoa.h"

extern unsigned char scrw;
char *translit_charset;
char video_size;
char enable_subtitles;
char sub_language[3] = "en";
char tmp_buf[TMP_BUF_SIZE];

#pragma code-name (push, "LOWCODE")

static FILE *open_config(char *mode) {
  FILE *fp;
  #ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
  #endif
  fp = fopen("clisettings", mode);
  if (IS_NULL(fp) && mode[0] == 'w') {
    cputs("Could not open settings file.\r\n");
  }
  return fp;
}

#pragma code-name (pop)

static void save_config(void) {
  FILE *fp;

  cputs("Saving config...\r\n");
  fp = open_config("w");
  if (IS_NULL(fp)) {
    return;
  }

  fputs(translit_charset, fp);
  fputc('\n', fp);
  fputc(video_size+'0', fp);
  fputc('\n', fp);
  fputc(enable_subtitles+'0', fp);
  fputc('\n', fp);
  fputs(sub_language, fp);
  fputc('\n', fp);

  if (fclose(fp) != 0) {
    cputs("Could not save settings file.\r\n");
  }
}

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
  char c;

  cputs("Please choose your keyboard layout:");
  for (c = 0; c < N_CHARSETS; c++) {
    cputs("\r\n"); cutoa(c); cputs(". ");cputs(charsets[c]);
  }
  cputs("\r\n");
  
charset_again:
  c = cgetc();
  if (c >= '0' && c < '0'+N_CHARSETS) {
    translit_charset = (char *)charsets[c-'0'];
  } else {
    goto charset_again;
  }

  cputs("\r\nVideo size (Small - more FPS / Large - less FPS)? (s/l)\r\n");
  video_size = get_bool('s' /* HGR_SCALE_HALF */, 'l' /* HGR_SCALE_FULL */);

  cputs("\r\nEnable subtitles? (y/n)\r\n");
  enable_subtitles = get_bool('y', 'n');

  if (enable_subtitles) {
    cputs("\r\nPreferred subtitles language code (two letters)? ");
    dget_text_single(sub_language, 3, NULL);
    if (sub_language[0] == '\0')
      strcpy(sub_language, "en");
    sub_language[0] = tolower(sub_language[0]);
    sub_language[1] = tolower(sub_language[1]);
  }
  save_config();
}

#pragma code-name(push, "RT_ONCE")

void load_config(void) {
  FILE *fp;

  translit_charset = US_CHARSET;
  video_size = HGR_SCALE_HALF;
  enable_subtitles = SUBTITLES_AUTO;
  strcpy(sub_language, "en");

  cputs("Loading config...\r\n");
  fp = open_config("r");
  if (IS_NULL(fp)) {
    return;
  }

  if (IS_NOT_NULL(fp)) {
    fgets(tmp_buf, 16, fp);
    if (IS_NOT_NULL(strchr(tmp_buf, '\n'))) {
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
