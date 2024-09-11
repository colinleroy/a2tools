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
#include "clrzone.h"
#include "scroll.h"
#include "scrollwindow.h"
#include "strsplit.h"
#include "runtime_once_clean.h"
#include "malloc0.h"

static char *url_enter(char *url, char *suffix);

static char login[32] = "";
static char password[32] = "";

extern char *translit_charset;

char **display_lines;

char tmp_buf[BUFSIZE];

char cmd_cb_handled = 0;

void stp_clr_page(void) {
  clrzone(0, PAGE_BEGIN, NUMCOLS - 1, PAGE_BEGIN + PAGE_HEIGHT);
}

/* Wozamp hack to scroll */
char stp_list_scroll_after_url = 0;

char *stp_get_start_url(char *header, char *default_url, cmd_handler_func cmd_cb) {
  FILE *fp;
  char *start_url = NULL;
  char *tmp = NULL;
  int changed = 0;
  char orig_x, orig_y;

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

  orig_x = wherex();
  orig_y = wherey();
  cputs(header);
  cputs("URL: ");

  dget_text(tmp_buf, BUFSIZE, cmd_cb, 0);
  if (cmd_cb) {
    cputs("\r\n");
  }

  changed = strcmp(tmp_buf, start_url);

  free(start_url);
  if (cmd_cb_handled) {
    return NULL;
  }
  start_url = strdup(tmp_buf);

  if (stp_list_scroll_after_url) {
    clrzone(orig_x, orig_y, NUMCOLS - orig_x, orig_y + 1);
    cputs("URL: ");
    cputs(start_url);
    cputs("\r\n");
}

  cputs("Login: ");
  strcpy(tmp_buf, login);
  dget_text(login, 32, cmd_cb, 0);
  if (cmd_cb) {
    cputs("\r\n");
  }
  if (cmd_cb_handled) {
    return NULL;
  }
  changed |= strcmp(tmp_buf, login);

  cputs("Password: ");
  echo(0);
  strcpy(tmp_buf, password);
  dget_text(password, 32, cmd_cb, 0);
  echo(1);
  if (cmd_cb) {
    cputs("\r\n");
  }
  if (cmd_cb_handled) {
    return NULL;
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

#ifdef __CC65__
#pragma code-name (push, "LOWCODE")
#endif

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
  set_scrollwindow(PAGE_BEGIN, PAGE_BEGIN + PAGE_HEIGHT + 1);
  if (scroll_way < 0 && cur_line < cur_display_line) {
    cur_display_line = cur_line;
    scroll_changed = 1;
    if (!rollover) {
      scrolldown_one();
    }
  }

  if (scroll_way > 0 && cur_line - PAGE_HEIGHT > cur_display_line) {
    cur_display_line = cur_line - PAGE_HEIGHT;
    scroll_changed = 1;
    if (!rollover) {
      scrollup_one();
    }
  }
  set_scrollwindow(0, 24);

  return scroll_changed && rollover;
}

char search_buf[80] = "";
static int search_from = 0;

void stp_list_search(unsigned char new_search) {
  int i;

  if (new_search) {
    clrzone(0, 0, NUMCOLS - 1, 0);
    cputs("Search: ");
    dget_text(search_buf, BUFSIZE-1, NULL, 0);
    if (search_buf[0] == '\0') {
      return;
    }
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

static void cnputs_nowrap(const char *str) {
  strncpy(tmp_buf, str, NUMCOLS - 3);
  tmp_buf[NUMCOLS-3] = '\0';
  cputs(tmp_buf);
}

static unsigned char hscroll_off = 0;
static signed char hscroll_dir = 1;

void stp_animate_list(char reset) {
  unsigned char line_off;
  unsigned char cur_line_len;

  if (num_lines == 0) {
    return;
  }

  line_off = cur_line - cur_display_line;

  if (reset) {
    hscroll_off = 0;
    goto reprint;
  }

  cur_line_len = strlen(display_lines[cur_line]);
  if (cur_line_len > NUMCOLS - 3) {
    if (hscroll_dir == 1 && cur_line_len - hscroll_off == NUMCOLS - 3) {
      hscroll_dir = -1;
      platform_msleep(500);
    } else if (hscroll_off == 0) {
      hscroll_dir = 1;
      platform_msleep(500);
    }

    hscroll_off += hscroll_dir;

reprint:
    gotoxy(2, PAGE_BEGIN + line_off);
    cnputs_nowrap(display_lines[cur_line] + hscroll_off);
    if (!reset) {
      platform_msleep(100);
    }
  }
}

void stp_update_list(char full_update) {
  unsigned char i;
  int j;

  hscroll_off = 0;
  hscroll_dir = 1;

  if (full_update) {
    stp_clr_page();
    for (i = PAGE_BEGIN, j = cur_display_line; j < num_lines && i <= PAGE_BEGIN+PAGE_HEIGHT; i++, j++) {
      gotoxy(2, i);
      cnputs_nowrap(display_lines[j]);
    }
  } 
  i = PAGE_BEGIN + cur_line - cur_display_line;
  if (cur_line < num_lines) {
    gotoxy (2, i);
    cnputs_nowrap(display_lines[cur_line]);

    clrzone(0, PAGE_BEGIN, 1, PAGE_BEGIN + PAGE_HEIGHT);
  }
  gotoxy(0, i);
  cputc('>');
}

void stp_free_data(void) {
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
}

extern char center_x;
int stp_get_data(char *url, const surl_response **resp) {
  *resp = NULL;
  num_lines = 0;
  cur_line = 0;
  cur_display_line = 0;

  stp_free_data();

  stp_clr_page();
  gotoxy(center_x, 12);
  cputs("Loading...   ");

  *resp = surl_start_request(SURL_METHOD_GET, url, NULL, 0);

  stp_print_result(*resp);

  gotoxy(0, 2);

  if ((*resp)->size == 0) {
    gotoxy(center_x, 12);
    if (surl_response_ok()) {
      cputs("Empty.       ");
    } else {
      cputs("Bad response.");
    }
    return KEYBOARD_INPUT;
  }

  if ((*resp)->content_type && strcmp((*resp)->content_type, "directory")) {
    return SAVE_DIALOG;
  } else {
    if ((*resp)->size > STP_DATA_SIZE-1) {
      gotoxy(center_x, 18);
      cputs("Not enough memory :-(");
      return KEYBOARD_INPUT;
    }
    surl_receive_data(data, (*resp)->size);

    surl_translit(translit_charset);
    if (surl_response_ok()) {
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
  char *last_slash;

  last_slash = strrchr(url, '/');
  if (last_slash && last_slash - url > 7 /*strlen("sftp://") */) {
    *(last_slash) = '\0';
  }

  search_buf[0] = '\0';

  return url;
}

char *stp_url_enter(char *url, char *suffix) {
  int url_len = strlen(url);
  int suffix_len = strlen(suffix);
  int url_ends_slash = url[url_len - 1] == '/';
  char *tmp = realloc(url, url_len + suffix_len + 2);

  if (!tmp) {
    gotoxy(center_x, 12);
    cputs("Not enough memory :-(");
    return url;
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

static char *header_url = NULL;
void stp_print_header(const char *url, enum HeaderUrlAction action) {
  char *host, *tmp;
  int header_url_len, url_len = strlen(url);

  if (action == URL_SET) {
    header_url = realloc_safe(header_url, url_len + 1);
    strcpy(header_url, url);
  }

  switch(action) {
    case URL_RESTORE:
    case URL_SET:
      break;
    case URL_ADD:
      header_url_len = strlen(header_url);
      header_url = realloc_safe(header_url, header_url_len + url_len + 1);
      if (header_url[header_url_len-1] != '/')
        strcat(header_url, "/");
      strcat(header_url, url);
      break;
    case URL_UP:
      if ((tmp = strrchr(header_url + 7 /*strlen("sftp://")*/, '/')))
        *tmp = '\0';
      break;
  }

  host = strstr(header_url, "://");
  if (host) {
    char *pass_sep;

    host += 3;
    pass_sep = strchr(host, ':');

    if (pass_sep && pass_sep < strchr(host, '@')) {
      /* Means there's a login */
      pass_sep++;
      while(*pass_sep != '@') {
        *pass_sep = '*';
        pass_sep++;
      }
    }
  }
  clrzone(0, 0, NUMCOLS - 1, 0);

  if (strlen(header_url) > NUMCOLS - 3) {
    cputs("...");
    cnputs_nowrap(header_url + strlen(header_url) - NUMCOLS + 3);
  } else {
    cputs(header_url);
  }
  gotoxy(0, 1);
  chline(NUMCOLS);
}
