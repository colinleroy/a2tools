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
#include "stp_list.h"
#include "clrzone.h"
#include "hgr.h"
#include "scrollwindow.h"
#include "citoa.h"
#include "a2_features.h"

char *translit_charset;
char monochrome;
char enable_video;
char enable_subtitles;
char video_size;
extern unsigned char NUMCOLS;

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

  cputs("Saving config...\r\n");
  fp = open_config("w");
  if (fp == NULL) {
    return -1;
  }

  fputs(translit_charset, fp);
  fputc('\n', fp);
  fputc(monochrome+'0', fp);
  fputc('\n', fp);
  fputc(enable_video+'0', fp);
  fputc('\n', fp);
  fputc(enable_subtitles+'0', fp);
  fputc('\n', fp);
  fputc(video_size+'0', fp);
  fputc('\n', fp);

  if (fclose(fp) != 0) {
    cputs("Could not save settings file.\r\n");
    return -1;
  }
  return 0;
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

  clrzone(0, PAGE_BEGIN, NUMCOLS - 1, PAGE_BEGIN + PAGE_HEIGHT);

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

  cputs("\r\nIs your monitor monochrome? (y/n)\r\n");
  monochrome = get_bool('y', 'n');

  if (!is_iigs) {
    cputs("\r\nEnable video playback? (y/n)\r\n");
    enable_video = get_bool('y', 'n');

    cputs("\r\nVideo size (Small - more FPS / Large - less FPS)? (s/l)\r\n");
    video_size = get_bool('s' /* HGR_SCALE_HALF */, 'l' /* HGR_SCALE_FULL */);

    cputs("\r\nEnable subtitles? (y/n)\r\n");
    enable_subtitles = get_bool('y', 'n');
  } else {
    enable_video = 0;
    video_size = 0;
    enable_subtitles = 0;
  }

  save_config();
}


#pragma code-name(push, "LC")

void text_config(void) {
  set_scrollwindow(0, NUMROWS);
  init_text();
  config();
  init_graphics(monochrome, 0);
  hgr_mixon();
  clrzone(0, 0, NUMCOLS, 19);
  set_scrollwindow(20, NUMROWS);
}

#pragma code-name(pop)

#pragma code-name(push, "RT_ONCE")

void load_config(void) {
  FILE *fp;

  translit_charset = US_CHARSET;
  monochrome = 0;

  enable_video = !is_iigs;
  enable_subtitles = SUBTITLES_AUTO;
  video_size = HGR_SCALE_HALF;

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
    monochrome = (tmp_buf[0] != '0');

    fgets(tmp_buf, 16, fp);
    enable_video = (tmp_buf[0] != '0');

    fgets(tmp_buf, 16, fp);
    enable_subtitles = (tmp_buf[0] != '0');

    fgets(tmp_buf, 16, fp);
    video_size = (tmp_buf[0]-'0');

    if (is_iigs) {
      enable_video = 0;
    }

    fclose(fp);
  }
}
#pragma code-name(pop)
