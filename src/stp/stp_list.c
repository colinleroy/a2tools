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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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
#include "strcasestr.h"

#ifdef __CC65__
#pragma code-name (push, "LOWCODE")
#endif

static char *url_enter(char *url, char *suffix);

static char *login = NULL;
static char *password = NULL;

extern char *welcome_header;

char *stp_get_start_url(char *header, char *default_url) {
  FILE *fp;
  char *start_url = NULL;
  char *last_start_url = NULL;
  char *last_login = NULL;
  char *last_password = NULL;
  int changed = 0;

#ifdef __APPLE2ENH__
  _filetype = PRODOS_T_TXT;
#endif

  fp = fopen(STP_URL_FILE,"r");
  if (fp != NULL) {
    last_start_url = malloc(BUFSIZE + 1);
    last_login = malloc(BUFSIZE + 1);
    last_password = malloc(BUFSIZE + 1);
    fgets(last_start_url, BUFSIZE, fp);
    fgets(last_login, BUFSIZE, fp);
    fgets(last_password, BUFSIZE, fp);
    fclose(fp);
    if (strchr(last_start_url,'\n'))
      *strchr(last_start_url,'\n') = '\0';
    if (strchr(last_login,'\n'))
      *strchr(last_login,'\n') = '\0';
    if (strchr(last_password,'\n'))
    *strchr(last_password,'\n') = '\0';
  } else {
    last_start_url = strdup(default_url);
    last_login = strdup("");
    last_password = strdup("");
  }

  clrscr();
  gotoxy(0, 1);
  if (welcome_header) {
    dputs(welcome_header);
  }

  gotoxy(0, 14);
  dputs(header);
  dputs("\r\nHit Enter to reuse the last one:\r\n\r\n"
        "'");
  dputs(last_start_url);
  dputs("'\r\n\r\n\r\n"
        "URL: ");

  start_url = malloc(BUFSIZE + 1);
  start_url[0] = '\0';
  dget_text(start_url, BUFSIZE, NULL, 0);

  if (*start_url == '\0') {
    free(start_url);
    start_url = last_start_url;
  } else {
    free(last_start_url);
    changed = 1;

    /* Forget login and password too */
    free(last_login);
    free(last_password);
    last_login = strdup("");
    last_password = strdup("");
  }

  if (*last_login != '\0') {
    dputs("Login (");
    dputs(last_login);
    dputs("): ");
  } else {
    dputs("Login (anonymous): ");
  }

  login = malloc(BUFSIZE + 1);
  login[0] = '\0';
  dget_text(login, BUFSIZE, NULL, 0);

  if (*login == '\0') {
    password = last_password;
  } else {
    password = malloc(BUFSIZE + 1);
    password[0] = '\0';
    dputs("Password: ");
    echo(0);
    dget_text(password, BUFSIZE, NULL, 0);
    echo(1);
    free(last_password);
    changed = 1;
  }

  if (strchr(password,'\n'))
    *strchr(password,'\n') = '\0';

  if (*login == '\0') {
    free(login);
    login = last_login;
  } else {
    free(last_login);
    changed = 1;
  }

  if (changed) {
    fp = fopen(STP_URL_FILE, "w");
    if (fp != NULL) {
      fprintf(fp, "%s\n", start_url);
      fprintf(fp, "%s\n", login);
      fprintf(fp, "%s\n", password);
      fclose(fp);
    } else {
      dputs("Can't save URL: ");
      dputs(strerror(errno));
      cgetc();
      exit(1);
    }
  }
  if (*login == '\0') {
    free(login);
    login = NULL;
  }
  if (*password == '\0') {
    free(password);
    password = NULL;
  }

  clrscr();
  return start_url;
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
char search_buf[80] = { 0 };
char tmp_buf[80];
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
    if (strcasestr(lines[i], search_buf) != NULL) {
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



char *stp_build_login_url(char *url) {
  char *host = strstr(url, "://");
  char *proto;
  char *full_url;

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

  if (login)
    snprintf(full_url, BUFSIZE, "%s://%s:%s@%s", proto, login, password, host);
  else
    snprintf(full_url, BUFSIZE, "%s://%s", proto, host);
  free(url);
  return full_url;
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

  cur_line_len = strlen(lines[cur_line]);
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
    strncpy(tmp_buf, lines[cur_line] + hscroll_off, scrw - 3);
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
      strncpy(tmp_buf, lines[i + cur_display_line], scrw - 3);
      tmp_buf[scrw-3] = '\0';
      dputs(tmp_buf);
    }
  } else if (cur_line < num_lines) {
    gotoxy (2, PAGE_BEGIN + cur_line - cur_display_line);
    strncpy(tmp_buf, lines[cur_line], scrw - 3);
    tmp_buf[scrw-3] = '\0';
    dputs(tmp_buf);

    clrzone(0, PAGE_BEGIN, 1, PAGE_BEGIN + PAGE_HEIGHT);
  }
  gotoxy(0, PAGE_BEGIN + cur_line - cur_display_line);
  dputc('>');
}

extern char center_x;
int stp_get_data(char *url, const surl_response **resp) {
  size_t r;

  *resp = NULL;
  num_lines = 0;
  cur_line = 0;
  cur_display_line = 0;

  if (lines)
    free(lines);
  lines = NULL;

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
    if ((*resp)->size > STP_DATA_SIZE) {
      gotoxy(center_x, 18);
      dputs("Not enough memory :-(");
      return KEYBOARD_INPUT;
    }
    r = surl_receive_data(data, (*resp)->size);
  }

  stp_print_header(url);

  if (r < (*resp)->size) {
    gotoxy(center_x - 7, 12);
    dputs("Can not load response.");
    return KEYBOARD_INPUT;
  }

  num_lines = strsplit_in_place(data, '\n', &lines);

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
  if (last_slash && last_slash - url > 6) {
    *(last_slash + 1) = '\0';
  }

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

  return url;
}

#ifdef __CC65__
#pragma code-name (pop)
#endif

void stp_print_header(char *url) {
  char *no_pass_url = strdup(url);

  if (strchr(no_pass_url, ':') != strrchr(no_pass_url,':')) {
    /* Means there's a login */
    char *t = strrchr(no_pass_url, ':') + 1;
    while(*t != '@') {
      *t = '*';
      t++;
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
