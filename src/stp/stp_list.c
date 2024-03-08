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
#include "malloc0.h"

#ifdef __CC65__
#pragma code-name (push, "LOWCODE")
#endif

static char *url_enter(char *url, char *suffix);

static char *login = NULL;
static char *password = NULL;

char *stp_get_start_url(void) {
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
    last_start_url = strdup("ftp://ftp.apple.asimov.net/");
    last_login = strdup("");
    last_password = strdup("");
  }

  gotoxy(0, 9);
  dputs("Please enter the server's root URL,\r\n"
        "or Enter to reuse the last one:\r\n\r\n"
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

  return start_url;
}

char **lines = NULL;
int num_lines = 0;
int cur_line = 0;
int cur_display_line = 0;
char *data = NULL;

char stp_list_scroll(signed char shift) {
  signed char scroll_changed = 0, scroll_way = 0;
  char rollover = 0;

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

void stp_update_list(char full_update) {
  int i;
  if (full_update) {
    clrzone(0, PAGE_BEGIN, scrw - 1, PAGE_BEGIN + PAGE_HEIGHT);
    for (i = 0; i + cur_display_line < num_lines && i <= PAGE_HEIGHT; i++) {
      gotoxy(2, i + PAGE_BEGIN);
      dputs(lines[i + cur_display_line]);
    }
  } else if (cur_line < num_lines) {
    gotoxy (2, PAGE_BEGIN + cur_line - cur_display_line);
    dputs(lines[cur_line]);

    clrzone(0, PAGE_BEGIN, 1, PAGE_BEGIN + PAGE_HEIGHT);
  }
  gotoxy(0, PAGE_BEGIN + cur_line - cur_display_line);
  dputc('>');
}

int stp_get_data(char *url, const surl_response **resp) {
  size_t r;
  char center_x = 30; /* 12 in 40COLS */

  *resp = NULL;
  num_lines = 0;
  cur_line = 0;
  cur_display_line = 0;


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
    data = malloc0((*resp)->size + 1);
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

  url = realloc(url, url_len + suffix_len + (url_ends_slash ? 1 : 2));
  if (!url_ends_slash) {
    url[url_len] = '/';
    url_len++;
  }
  strcpy(url + url_len, suffix);

  return url;
}

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
  if (strlen(no_pass_url) > scrw - 10) {
    char *tmp = strdup(no_pass_url + strlen(no_pass_url) - scrw + 5);
    dputs("...");
    dputs(tmp);
    free(tmp);
  } else {
    dputs(no_pass_url);
  }
  free(no_pass_url);
#ifdef __CC65__
  gotoxy(69, 0);
  printf("%zub free", _heapmemavail());
#endif
  gotoxy(0, 1);
  chline(scrw);
}

void stp_print_result(const surl_response *response) {
  gotoxy(0, 20);
  chline(scrw);
  clrzone(0, 21, scrw - 1, 21);
  gotoxy(0, 21);
  if (response == NULL) {
    dputs("Unknown request error.");
  } else {
    cprintf("Response code %d - %lu bytes, %s",
            response->code,
            response->size,
            response->content_type != NULL ? response->content_type : "");
  }
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
