/*
 * Copyright (C) 2022 Colin Leroy-Mira <colin@colino.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CC65__
#define _GNU_SOURCE
#endif
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "stp_list.h"
#include "stp_cli.h"
#include "surl.h"
#include "simple_serial.h"
#include "extended_conio.h"
#include "dgets.h"
#include "dputc.h"
#include "dputs.h"
#include "clrzone.h"
#include "scroll.h"
#include "scrollwindow.h"
#include "strsplit.h"
#include "runtime_once_clean.h"

static char *url_enter(char *url, char *suffix);


static char login[32] = "";
static char password[32] = "";

extern char *translit_charset;

char **display_lines;

char tmp_buf[BUFSIZE];

char *stp_get_start_url(char *header, char *default_url, cmd_handler_func cmd_cb) {
  FILE *fp;
  char *start_url = NULL;
  char *tmp = NULL;
  int changed = 0;

#ifdef __APPLE2__
  _filetype = PRODOS_T_TXT;
#endif

  fp = fopen(STP_URL_FILE, "r");
  if (fp != NULL) {
    fgets(tmp_buf, BUFSIZE-1, fp);
    fgets(login, 31, fp);
    fgets(password, 31, fp);

    fclose(fp);
    if ((tmp = strchr(tmp_buf,'\n')))
      *tmp = '\0';
    if ((tmp = strchr(login,'\n')))
      *tmp = '\0';
    if ((tmp = strchr(password,'\n')))
      *tmp = '\0';

  } else {
    strcpy(tmp_buf, default_url);
  }

  start_url = strdup(tmp_buf);

  dputs(header);
  dputs("URL: ");

  dget_text(tmp_buf, BUFSIZE, cmd_cb, 0);
  if (cmd_cb) {
    cputs("\r\n");
  }

  changed = strcmp(tmp_buf, start_url);

  free(start_url);
  start_url = strdup(tmp_buf);

  dputs("Login: ");
  strcpy(tmp_buf, login);
  dget_text(login, 32, cmd_cb, 0);
  if (cmd_cb) {
    cputs("\r\n");
  }
  changed |= strcmp(tmp_buf, login);

  dputs("Password: ");
  echo(0);
  strcpy(tmp_buf, password);
  dget_text(password, 32, cmd_cb, 0);
  echo(1);
  if (cmd_cb) {
    cputs("\r\n");
  }
  changed |= strcmp(tmp_buf, password);

  if (changed) {
    fp = fopen(STP_URL_FILE, "w");
    if (fp != NULL) {
      fputs(start_url, fp);
      fputc('\n', fp);
      fputs(login, fp);
      fputc('\n', fp);
      fputs(password, fp);
      fputc('\n', fp);
      fclose(fp);
    }
  }

  tmp = start_url + strlen(start_url) - 1;
  if (*tmp == '/') {
    *tmp = '\0';
  }

  clrscr();
  return start_url;
}

char *stp_build_login_url(char *url) {
  char *host = strstr(url, "://");
  char *proto;
  char *full_url;
  char *tmp;

  full_url = malloc(BUFSIZE);

  if (host != NULL) {
    *host = '\0';
    /* url is now protocol */
    proto = url;
    host = host + 3;
  } else {
    proto = "ftp";
    host = url;
  }

  if (login[0] != '\0') {
    tmp = stpcpy(full_url, proto);
    tmp = stpcpy(tmp, "://");
    tmp = stpcpy(tmp, login);
    tmp = stpcpy(tmp, ":");
    tmp = stpcpy(tmp, password);
    tmp = stpcpy(tmp, "@");
    stpcpy(tmp, host);
  } else {
    tmp = stpcpy(full_url, proto);
    tmp = stpcpy(tmp, "://");
    stpcpy(tmp, host);
  }
  full_url = realloc(full_url, strlen(full_url) + 1);

  free(url);
  return full_url;
}

#ifdef __CC65__
#pragma code-name (push, "LOWCODE")
#endif

int num_lines = 0;
int cur_line = 0;
int cur_display_line = 0;

char stp_list_scroll(signed char shift) {
  signed char scroll_changed = 0, scroll_way = 0;
  char rollover = 0;

  /* Unscroll potentially long name */
  stp_animate_list(1);

  /* Handle list offset */
  if (shift < 0) {
    if (cur_line > 0) {
      cur_line--;
      scroll_way = -1;
    } else {
      rollover = 1;
      cur_line = num_lines - 1;
      scroll_way = +1;
    }
  } else if (shift > 0) {
    if (cur_line < num_lines - 1) {
      cur_line++;
      scroll_way = +1;
    } else {
      rollover = 1;
      cur_line = 0;
      scroll_way = -1;
    }
  } else {
    return 0;
  }

  /* Handle scroll */
  if (scroll_way < 0 && cur_line < cur_display_line) {
    cur_display_line = cur_line;
    scroll_changed = 1;
    if (!rollover) {
      set_scrollwindow(PAGE_BEGIN, PAGE_BEGIN + PAGE_HEIGHT + 1);
      scrolldown_one();
      set_scrollwindow(0, 24);
    }
  }

  if (scroll_way > 0 && cur_line - PAGE_HEIGHT > cur_display_line) {
    cur_display_line = cur_line - PAGE_HEIGHT;
    scroll_changed = 1;
    if (!rollover) {
      set_scrollwindow(PAGE_BEGIN, PAGE_BEGIN + PAGE_HEIGHT + 1);
      scrollup_one();
      set_scrollwindow(0, 24);
    }
  }
  return scroll_changed && rollover;
}

extern unsigned char scrw, scrh;
char search_buf[80] = "";
static int search_from = 0;

void stp_list_search(unsigned char new_search) {
  int i;

  if (new_search) {
    clrzone(0, 0, scrw - 1, 0);
    gotoxy(0, 0);
    dputs("Search: ");
    strcpy(tmp_buf, search_buf);
    dget_text(tmp_buf, 79, NULL, 0);
    if (tmp_buf[0] == '\0') {
      return;
    }
    strcpy(search_buf, tmp_buf);
  }

  search_from = cur_line + 1;

  /* Do the actual search */
search_from_start:
  for (i = search_from; i < num_lines; i++) {
    if (strcasestr(display_lines[i], search_buf) != NULL) {
      search_from = i;
      cur_display_line = search_from;
      cur_line = cur_display_line;
      stp_update_list(1);
      break;
    }
  }
  if (i == num_lines) {
    if (search_from != 0) {
      search_from = 0;
      goto search_from_start;
    } else {
      beep();
    }
  }
}

static int hscroll_off = 0;
static int hscroll_dir = 1;
void stp_animate_list(char reset) {
  int line_off;
  int cur_line_len;
  if (num_lines == 0) {
    return;
  }

  line_off = cur_line - cur_display_line;
  if (reset) {
    hscroll_off = 0;
    goto reprint;
  }

  cur_line_len = strlen(display_lines[cur_line]);
  if (cur_line_len > scrw - 3) {
    if (hscroll_dir == 1 && cur_line_len - hscroll_off == scrw - 3) {
      hscroll_dir = -1;
      platform_msleep(500);
    } else if (hscroll_off == 0) {
      hscroll_dir = 1;
      platform_msleep(500);
    }

    hscroll_off += hscroll_dir;

reprint:
    gotoxy(2, PAGE_BEGIN + line_off);
    strncpy(tmp_buf, display_lines[cur_line] + hscroll_off, scrw - 3);
    tmp_buf[scrw-3] = '\0';
    dputs(tmp_buf);
    if (!reset) {
      platform_msleep(100);
    }
  }
}

void stp_update_list(char full_update) {
  int i;

  hscroll_off = 0;
  hscroll_dir = 1;

  if (full_update) {
    clrzone(0, PAGE_BEGIN, scrw - 1, PAGE_BEGIN + PAGE_HEIGHT);
    for (i = 0; i + cur_display_line < num_lines && i <= PAGE_HEIGHT; i++) {
      gotoxy(2, i + PAGE_BEGIN);
      strncpy(tmp_buf, display_lines[i + cur_display_line], scrw - 3);
      tmp_buf[scrw-3] = '\0';
      dputs(tmp_buf);
    }
  } else if (cur_line < num_lines) {
    gotoxy (2, PAGE_BEGIN + cur_line - cur_display_line);
    strncpy(tmp_buf, display_lines[cur_line], scrw - 3);
    tmp_buf[scrw-3] = '\0';
    dputs(tmp_buf);

    clrzone(0, PAGE_BEGIN, 1, PAGE_BEGIN + PAGE_HEIGHT);
  }
  gotoxy(0, PAGE_BEGIN + cur_line - cur_display_line);
  dputc('>');
}

extern char center_x;
int stp_get_data(char *url, const surl_response **resp) {
  *resp = NULL;
  num_lines = 0;
  cur_line = 0;
  cur_display_line = 0;

  if (lines)
    free(lines);
  lines = NULL;
  if (nat_lines)
    free(nat_lines);
  if (nat_data)
    free(nat_data);
  lines = NULL;
  nat_lines = NULL;
  nat_data = NULL;

  clrzone(0, PAGE_BEGIN, scrw - 1, PAGE_BEGIN + PAGE_HEIGHT);
  gotoxy(center_x, 12);
  dputs("Loading...   ");

  *resp = surl_start_request(SURL_METHOD_GET, url, NULL, 0);

  stp_print_result(*resp);

  gotoxy(0, 2);

  if ((*resp)->size == 0) {
    gotoxy(center_x, 12);
    if (surl_response_ok()) {
      dputs("Empty.       ");
    } else {
      dputs("Bad response.");
    }
    return KEYBOARD_INPUT;
  }

  if ((*resp)->content_type && strcmp((*resp)->content_type, "directory")) {
    return SAVE_DIALOG;
  } else {
    if ((*resp)->size > STP_DATA_SIZE-1) {
      gotoxy(center_x, 18);
      dputs("Not enough memory :-(");
      return KEYBOARD_INPUT;
    }
    surl_receive_data(data, (*resp)->size);

    surl_translit(translit_charset);
    if ((*resp)->code / 100 == 2) {
      nat_data = malloc((*resp)->size + 1);
      if (nat_data) {
        surl_receive_data(nat_data, (*resp)->size);
      }
    }
  }

  num_lines = strsplit_in_place(data, '\n', &lines);
  if (nat_data && strsplit_in_place(nat_data, '\n', &nat_lines) > 0) {
    display_lines = nat_lines;
  } else {
    nat_lines = NULL;
    display_lines = lines;
  }
  return UPDATE_LIST;
}

char *stp_url_up(char *url) {
  int url_len = strlen(url);
  char *last_slash;

  while (url_len > 1 && url[url_len - 1] == '/') {
    url[url_len - 1] = '\0';
    url_len = strlen(url);
  }

  last_slash = strrchr(url, '/');
  if (last_slash && last_slash - url > strlen("sftp://")) {
    *(last_slash + 1) = '\0';
  }

  search_buf[0] = '\0';

  return url;
}

char *stp_url_enter(char *url, char *suffix) {
  int url_len = strlen(url);
  int suffix_len = strlen(suffix);
  int url_ends_slash = url[url_len - 1] == '/';
  char *tmp = realloc(url, url_len + suffix_len + (url_ends_slash ? 1 : 2));
  if (!tmp) {
    gotoxy(center_x, 12);
    dputs("Not enough memory :-(");
  }
  url = tmp;
  if (!url_ends_slash) {
    url[url_len] = '/';
    url_len++;
  }
  strcpy(url + url_len, suffix);
  search_buf[0] = '\0';

  return url;
}

void stp_print_header(const char *url, enum HeaderUrlAction action) {
  char *no_pass_url = NULL, *host;
  static char *header_url = NULL;

  if (action == URL_SET) {
    if (header_url != NULL)
      free(header_url);
    header_url = strdup(url);
  }

  no_pass_url = strdup(header_url);
  switch(action) {
    case URL_RESTORE:
    case URL_SET:
      break;
    case URL_ADD:
      no_pass_url = realloc(no_pass_url, strlen(header_url) + strlen(url) + 3);
      if (header_url[strlen(header_url)-1] != '/')
        strcat(no_pass_url, "/");
      strcat(no_pass_url, url);
      break;
    case URL_UP:
      if (strchr(no_pass_url + 8 /*strlen("sftp://")+1*/, '/'))
        *(strrchr(no_pass_url, '/')) = '\0';
      break;
  }

  free(header_url);
  header_url = strdup(no_pass_url);

  host = strstr(no_pass_url, "://");
  if (host) {
    host += 3;
    if (strchr(host, ':') && strchr(host, ':') < strchr(host, '@')) {
      /* Means there's a login */
      char *t = strchr(host, ':') + 1;
      while(*t != '@') {
        *t = '*';
        t++;
      }
    }
  }
  clrzone(0, 0, scrw - 1, 0);
  gotoxy(0, 0);

  if (strlen(no_pass_url) > scrw - 3) {
    dputs("...");
    strncpy(tmp_buf, no_pass_url + strlen(no_pass_url) - scrw + 3, scrw - 3);
    tmp_buf[scrw - 3] = '\0';
    dputs(tmp_buf);
  } else {
    dputs(no_pass_url);
  }
  free(no_pass_url);
  gotoxy(0, 1);
  chline(scrw);
}
