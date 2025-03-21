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

char *translit_charset;
char monochrome;
char enable_video;
char enable_subtitles;
char video_size;

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

#pragma code-name(push, "LOWCODE")

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

#ifdef __APPLE2ENH__
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
#else
  translit_charset = US_CHARSET;
#endif

  cputs("\r\nIs your monitor monochrome? (y/n)\r\n");
  monochrome = get_bool('y', 'n');

#ifndef IIGS
  cputs("\r\nEnable video playback? (y/n)\r\n");
  enable_video = get_bool('y', 'n');

  cputs("\r\nVideo size (Small - more FPS / Large - less FPS)? (s/l)\r\n");
  video_size = get_bool('s' /* HGR_SCALE_HALF */, 'l' /* HGR_SCALE_FULL */);

  cputs("\r\nEnable subtitles? (y/n)\r\n");
  enable_subtitles = get_bool('y', 'n');
#else
  enable_video = 0;
  video_size = 0;
  enable_subtitles = 0;
#endif

  save_config();
}

#pragma code-name(pop)

void text_config(void) {
  set_scrollwindow(0, NUMROWS);
  init_text();
  config();
  init_hgr(1);
  hgr_mixon();
  clrzone(0, 0, NUMCOLS, 19);
  set_scrollwindow(20, NUMROWS);
}

#pragma code-name(push, "RT_ONCE")

void load_config(void) {
  FILE *fp;

  translit_charset = US_CHARSET;
  monochrome = 0;
  video_size = HGR_SCALE_HALF;
#ifndef IIGS
  enable_video = 1;
#else
  enable_video = 0;
#endif
  enable_subtitles = SUBTITLES_AUTO;

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
    monochrome = (tmp_buf[0] != '0');

    fgets(tmp_buf, 16, fp);
#ifndef IIGS
    enable_video = (tmp_buf[0] != '0');
#endif

    fgets(tmp_buf, 16, fp);
#ifndef IIGS
    enable_subtitles = (tmp_buf[0] != '0');
#endif
    fgets(tmp_buf, 16, fp);
#ifndef IIGS
    video_size = (tmp_buf[0]-'0');
#endif

    fclose(fp);
  }
}
#pragma code-name(pop)
