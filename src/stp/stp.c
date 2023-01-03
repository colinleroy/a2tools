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
#ifdef __CC65__
#include <apple2enh.h>
#endif
#include "stp.h"
#include "stp_cli.h"
#include "surl.h"
#include "simple_serial.h"
#include "extended_conio.h"
#include "extended_string.h"

static char *url_go_up(char *url);
static char *url_enter(char *url, char *suffix);

char *get_start_url(void) {
  FILE *fp;
  char *start_url = NULL;
  char *last_start_url = NULL;

#ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
#endif

  last_start_url = malloc(BUFSIZE);
  fp = fopen(STP_URL_FILE,"r");
  if (fp != NULL) {
    fgets(last_start_url, BUFSIZE, fp);
    fclose(fp);
    *strchr(last_start_url,'\n') = '\0';
  } else {
    last_start_url = strdup("ftp://ftp.apple.asimov.net");
  }

  gotoxy(0, 9);
  printf("Please enter the server's root URL,\n");
  printf("or Enter to reuse the last one:\n\n");
  printf("%s\n", last_start_url);
  printf("\n\nURL: ");

  start_url = malloc(BUFSIZE);
  cgets(start_url, BUFSIZE);

  *strchr(start_url,'\n') = '\0';
  if (*start_url == '\0') {
    free(start_url);
    start_url = last_start_url;
  } else {
    free(last_start_url);
  }

  fp = fopen(STP_URL_FILE, "wb");
  if (fp != NULL) {
    fprintf(fp, "%s\n", start_url);
    fclose(fp);
  } else {
    printf("Can't save URL: %s\n", strerror(errno));
    exit(1);
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

int main(int argc, char **argv) {
  char *url;
  char c;
  int full_update = 1;

  clrscr();
  screensize(&scrw, &scrh);

  url = get_start_url();
  clrscr();

  stp_print_footer();
  while(1) {
    surl_response *resp = NULL;
    char *data = NULL, **lines = NULL;
    int i;

    num_lines = 0;
    cur_line = 0;
    cur_display_line = 0;

    stp_print_header(url);
    
    resp = surl_start_request("GET", url, NULL, 0);
    
    stp_print_result(resp);
    
    gotoxy(0, 2);

    if (resp != NULL && resp->code >= 200 && resp->code < 300 && resp->size > 0) {
      data = malloc(resp->size + 1);
    }
    if (data == NULL || surl_receive_data(resp, data, resp->size) < resp->size) {
      printf("Can not display request results.");
      goto keyb_input;
    }

    num_lines = strsplit_in_place(data, '\n', &lines);

update_list:
    if (full_update) {
      clrzone(0, 2, scrw, 2 + PAGE_HEIGHT);
      for (i = 0; i + cur_display_line < num_lines && i < PAGE_HEIGHT; i++) {
        if (i + cur_display_line == num_lines - 1
         && *lines[i] == '\0') {
           break;
        }
        gotoxy(0, i + 2);
        if (i + cur_display_line == cur_line) {
          printf("> %s", lines[i]);
        } else {
          printf("  %s", lines[i]);
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
      case CH_ESC: case 'z':
        url = url_go_up(url);
        full_update = 1;
        break;
      case CH_ENTER: case 'e':
        url = url_enter(url, lines[cur_line]);
        full_update = 1;
        break;
      case CH_CURS_UP: case 'i':
        full_update = scroll(-1);
        goto update_list;
      case CH_CURS_DOWN: case 'k':
        full_update = scroll(+1);
        goto update_list;
      default: 
        goto update_list;
    }
    surl_response_free(resp);
    resp = NULL;
    free(data);
    data = NULL;
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
