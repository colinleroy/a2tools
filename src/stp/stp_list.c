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
#include "dget_text.h"
#include "clrzone.h"
#include "scroll.h"
#include "scrollwindow.h"
#include "strsplit.h"
#include "runtime_once_clean.h"
#include "malloc0.h"
#include "platform.h"

static char *url_enter(char *url, char *suffix);

static char login[32] = "";
static char password[32] = "";

extern char *translit_charset;

char **display_lines;

#ifndef __CC65__
char tmp_buf[BUFSIZE];
char search_buf[SEARCH_BUF_SIZE];
#endif

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
  if (IS_NOT_NULL(fp)) {
    fgets(tmp_buf, BUFSIZE-1, fp);
    fgets(login, 31, fp);
    fgets(password, 31, fp);

    fclose(fp);
    if (IS_NOT_NULL(tmp = strchr(tmp_buf,'\n')))
      *tmp = '\0';
    if (IS_NOT_NULL(tmp = strchr(login,'\n')))
      *tmp = '\0';
    if (IS_NOT_NULL(tmp = strchr(password,'\n')))
      *tmp = '\0';

  } else {
    strcpy(tmp_buf, default_url);
  }

  start_url = strdup(tmp_buf);

  orig_x = wherex();
  orig_y = wherey();
  cputs(header);
  cputs("URL: ");

  dget_text_single(tmp_buf, BUFSIZE, cmd_cb);
  if (IS_NOT_NULL(cmd_cb)) {
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
  dget_text_single(login, 32, cmd_cb);
  if (IS_NOT_NULL(cmd_cb)) {
    cputs("\r\n");
  }
  if (cmd_cb_handled) {
    return NULL;
  }
  changed |= strcmp(tmp_buf, login);

  cputs("Password: ");
  dgets_echo_on = 0;
  strcpy(tmp_buf, password);
  dget_text_single(password, 32, cmd_cb);
  dgets_echo_on = 1;

  if (IS_NOT_NULL(cmd_cb)) {
    cputs("\r\n");
  }
  if (cmd_cb_handled) {
    return NULL;
  }
  changed |= strcmp(tmp_buf, password);

  if (changed) {
    fp = fopen(STP_URL_FILE, "w");
    if (IS_NOT_NULL(fp)) {
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

  if (IS_NOT_NULL(host)) {
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

static int search_from = 0;

void stp_list_search(unsigned char new_search) {
  int i;

  if (new_search) {
    clrzone(0, 0, NUMCOLS - 1, 0);
    cputs("Search: ");
    dget_text_single(search_buf, 40, NULL);

    stp_print_footer();

    if (search_buf[0] == '\0') {
      return;
    }
  }

  search_from = cur_line + 1;

  /* Do the actual search */
search_from_start:
  for (i = search_from; i < num_lines; i++) {
    if (IS_NOT_NULL(strcasestr(display_lines[i], search_buf))) {
      unsigned char full_update = 1;
      cur_line = search_from = i;

      if (cur_line < cur_display_line) {
        /* Result is before current offset: display it at top */
        cur_display_line = search_from;
      } else if (cur_line > cur_display_line + PAGE_HEIGHT) {
        /* Result is after current offset: display it at bottom */
        cur_display_line = search_from - PAGE_HEIGHT;
      } else {
        /* Result is on current page, no scroll to do */
        full_update = 0;
      }
      stp_update_list(full_update);
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
  clreol();
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
      platform_interruptible_msleep(500);
    } else if (hscroll_off == 0) {
      hscroll_dir = 1;
      platform_interruptible_msleep(500);
    }

    hscroll_off += hscroll_dir;

reprint:
    gotoxy(2, PAGE_BEGIN + line_off);
    cnputs_nowrap(display_lines[cur_line] + hscroll_off);
    if (!reset) {
      platform_interruptible_msleep(100);
    }
  }
}

void stp_update_list(char full_update) {
  unsigned char i;
  int j;

  hscroll_off = 0;
  hscroll_dir = 1;

  if (full_update) {
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
  free(lines);
  lines = NULL;
  free(nat_lines);
  nat_lines = NULL;
  if (!nat_data_static) {
    free(nat_data);
    nat_data = NULL;
  }
}

extern char center_x;
static char center_y = 11;
unsigned char stp_get_data(char *url) {
  extern surl_response resp;

  num_lines = 0;
  cur_line = 0;
  cur_display_line = 0;

  stp_free_data();

  stp_clr_page();
  gotoxy(center_x, center_y);
  cputs("Loading...   ");

  surl_start_request(NULL, 0, url, SURL_METHOD_GET);

  stp_print_result();

  gotoxy(center_x, center_y);
  if (!surl_response_ok()) {
    cputs("Bad response.");
    stp_print_footer();
    return KEYBOARD_INPUT;
  } else if (resp.size == 0) {
    cputs("Empty.       ");
    stp_print_footer();
    return KEYBOARD_INPUT;
  }

  gotoxy(0, 2);

  if (IS_NOT_NULL(resp.content_type) && strcmp(resp.content_type, "directory")) {
    return SAVE_DIALOG;
  } else {
    if (resp.size > STP_DATA_SIZE-1) {
      gotoxy(center_x, 18);
      cputs("Not enough memory :-(");
      return KEYBOARD_INPUT;
    }
    surl_receive_data(data, resp.size);
    num_lines = strsplit_in_place(data, '\n', &lines);

    if (nat_data_static || resp.size < _heapmaxavail() - 1024) {
      char *tmp;
      surl_translit(translit_charset);
      if (surl_response_ok()) {
        if (nat_data_static && resp.size < max_nat_data_size) {
          surl_receive_data(nat_data, resp.size);
        } else if (!nat_data_static) {
          nat_data = malloc(resp.size + 1);
          if (IS_NOT_NULL(nat_data)) {
            surl_receive_data(nat_data, resp.size);
          }
        }
        if (IS_NOT_NULL(nat_data)) {
          while ((tmp = strchr(nat_data, '/'))) {
            /* we're not supposed to have slashes in filenames, but transliteration
             * can put some. Change them. */
            *tmp = '_';
          }
        }
      }
    }
  }

  if (IS_NOT_NULL(nat_data) && strsplit_in_place(nat_data, '\n', &nat_lines) > 0) {
    display_lines = nat_lines;
  } else {
    nat_lines = NULL;
    display_lines = lines;
  }

  stp_print_footer();
  stp_clr_page();
  return UPDATE_LIST;
}

char *stp_url_up(char *url) {
  char *last_slash;

  last_slash = strrchr(url, '/');
  if (IS_NOT_NULL(last_slash) && last_slash - url > 7 /*strlen("sftp://") */) {
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

  if (IS_NULL(tmp)) {
    gotoxy(center_x, center_y);
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

#pragma code-name(pop)

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
      header_url = realloc_safe(header_url, header_url_len + url_len + 2);
      if (header_url[header_url_len-1] != '/')
        strcat(header_url, "/");
      strcat(header_url, url);
      break;
    case URL_UP:
      if (IS_NOT_NULL(tmp = strrchr(header_url + 7 /*strlen("sftp://")*/, '/')))
        *tmp = '\0';
      break;
  }

  host = strstr(header_url, "://");
  if (IS_NOT_NULL(host)) {
    char *pass_sep;

    host += 3;
    pass_sep = strchr(host, ':');

    if (IS_NOT_NULL(pass_sep) && pass_sep < strchr(host, '@')) {
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

  gotoxy(0, 20);
  chline(NUMCOLS);
  gotoxy(0, 1);
  chline(NUMCOLS);
}
