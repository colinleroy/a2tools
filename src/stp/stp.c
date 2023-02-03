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
#include "stp.h"
#include "stp_cli.h"
#include "stp_save.h"
#include "stp_send_file.h"
#include "stp_delete.h"
#include "surl.h"
#include "simple_serial.h"
#include "extended_conio.h"
#include "dgets.h"
#include "clrzone.h"
#include "strsplit.h"

static char *url_go_up(char *url);
static char *url_enter(char *url, char *suffix);

static char *login = NULL;
static char *password = NULL;
char *get_start_url(void) {
  FILE *fp;
  char *start_url = NULL;
  char *last_start_url = NULL;
  char *last_login = NULL;
  char *last_password = NULL;
  int changed = 0;
#ifdef PRODOS_T_TXT
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
    *strchr(last_start_url,'\n') = '\0';
    *strchr(last_login,'\n') = '\0';
    *strchr(last_password,'\n') = '\0';
  } else {
    last_start_url = strdup("ftp://ftp.apple.asimov.net");
    last_login = strdup("");
    last_password = strdup("");
  }

  gotoxy(0, 9);
  printf("Please enter the server's root URL,\n");
  printf("or Enter to reuse the last one:\n\n");
  printf("'%s'\n", last_start_url);
  printf("\n\nURL: ");

  start_url = malloc(BUFSIZE + 1);
  dget_text(start_url, BUFSIZE, NULL);
  if (strchr(start_url,'\n'))
    *strchr(start_url,'\n') = '\0';

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
    printf("Login (%s): ", last_login);
  } else {
    printf("Login (anonymous): ");
  }
  
  login = malloc(BUFSIZE + 1);
  dget_text(login, BUFSIZE, NULL);
  if (strchr(login,'\n'))
    *strchr(login,'\n') = '\0';

  if (*login == '\0') {
    password = last_password;
  } else {
    password = malloc(BUFSIZE + 1);
    printf("Password: ");
    echo(0);
    dget_text(password, BUFSIZE, NULL);
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
      printf("Can't save URL: %s\n", strerror(errno));
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

static int num_lines = 0;
static int cur_line = 0;
static int cur_display_line = 0;

static int scroll(int shift) {
  int scroll_changed = 0, scroll_way = 0;
  /* Handle list offset */
  if (shift < 0) {
    if (cur_line > 0) {
      cur_line--;
      scroll_way = -1;
    } else {
      cur_line = num_lines - 1;
      scroll_way = +1;
    }
  } else if (shift > 0) {
    if (cur_line < num_lines - 1) {
      cur_line++;
      scroll_way = +1;
    } else {
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
  }
  if (scroll_way > 0 && cur_line > cur_display_line + PAGE_HEIGHT - 1) {
    cur_display_line = cur_line - PAGE_HEIGHT + 1;
    scroll_changed = 1;
  }
  return scroll_changed;
}

static unsigned char scrw = 255, scrh = 255;

static char *build_login_url(char *url) {
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

int main(int argc, char **argv) {
  char *url;
  char c;
  int full_update = 1;

  clrscr();
  screensize(&scrw, &scrh);

  url = get_start_url();
  url = build_login_url(url);
  //clrscr();

  stp_print_footer();
  while(1) {
    surl_response *resp = NULL;
    char *data = NULL, **lines = NULL;
    int i;
    size_t r;

    num_lines = 0;
    cur_line = 0;
    cur_display_line = 0;

    stp_print_header(url);
    
    clrzone(0, 2, scrw - 1, 2 + PAGE_HEIGHT);
    gotoxy(12, 12);
    printf("Loading...   ");

    simple_serial_set_activity_indicator(1, 39, 0);
    resp = surl_start_request(SURL_METHOD_GET, url, NULL, 0);
    simple_serial_set_activity_indicator(0, 0, 0);
    
    stp_print_result(resp);
    
    gotoxy(0, 2);

    if (resp == NULL || resp->size == 0) {
      gotoxy(12, 12);
      if (resp && resp->code >= 200 && resp->code < 300) {
        printf("Empty.       ");
      } else {
        printf("Bad response.");
      }
      goto keyb_input;
    }

    if (resp->content_type && strcmp(resp->content_type, "directory")) {
      stp_save_dialog(url, resp);
      clrzone(0, 2, scrw - 1, 2 + PAGE_HEIGHT);
      stp_print_result(resp);
      goto up_dir;

    } else {
      data = malloc(resp->size + 1);
      simple_serial_set_activity_indicator(1, 39, 0);
      r = surl_receive_data(resp, data, resp->size);
      simple_serial_set_activity_indicator(0, 0, 0);
    }
    if (r < resp->size) {
      gotoxy(5, 12);
      printf("Can not load response.");
      goto keyb_input;
    }

    num_lines = strsplit_in_place(data, '\n', &lines);

update_list:
    if (full_update) {
      clrzone(0, 2, scrw - 1, 2 + PAGE_HEIGHT);
      for (i = 0; i + cur_display_line < num_lines && i < PAGE_HEIGHT; i++) {
        gotoxy(0, i + 2);
        if (i + cur_display_line == cur_line) {
          printf("> %s", lines[i + cur_display_line]);
        } else {
          printf("  %s", lines[i + cur_display_line]);
        }
      }
    } else {
      clrzone(0, 2, 1, 2 + PAGE_HEIGHT);
      gotoxy(0, 2 + cur_line - cur_display_line);
      cputc('>');
    }

keyb_input:
    c = cgetc();
    switch(c) {
      case CH_ESC:
up_dir:
        url = url_go_up(url);
        full_update = 1;
        break;
      case CH_ENTER:
        if (lines)
          url = url_enter(url, lines[cur_line]);
        full_update = 1;
        break;
      case CH_CURS_UP:
        full_update = scroll(-1);
        goto update_list;
      case CH_CURS_DOWN:
        full_update = scroll(+1);
        goto update_list;
      case 's':
      case 'S':
        stp_send_file(url);
        full_update = 1;
        break;
      case 'd':
      case 'D':
        if (lines)
          stp_delete_dialog(url, lines[cur_line]);
        full_update = 1;
        break;
      default: 
        goto update_list;
    }
    surl_response_free(resp);
    resp = NULL;
    free(data);
    data = NULL;
    free(lines);
    lines = NULL;
  }

  exit(0);
}

static char *url_go_up(char *url) {
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

static char *url_enter(char *url, char *suffix) {
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
