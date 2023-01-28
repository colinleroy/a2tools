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
#include "surl.h"
#include "simple_serial.h"
#include "scrollwindow.h"
#include "strsplit.h"
#include "cgets.h"
#ifdef __CC65__
#include "dputs.h"
#include "dputc.h"
#include "scroll.h"
#include <conio.h>
#else
#include "extended_conio.h"
#endif
#include "math.h"
#include "api.h"
#include "oauth.h"
#include "logo.h"

#define BUF_SIZE 255

static unsigned char scrw, scrh;
char *instance_url = NULL;
char *client_id = NULL;
char *client_secret = NULL;
char *login = NULL;
char *password = NULL;
char *oauth_code = NULL;
char *oauth_token = NULL;

static int save_settings(void) {
  FILE *fp;
  int r;

  fp = fopen("mastsettings", "w");
  if (fp == NULL) {
    dputs("Could not open settings file.\r\n");
    return -1;
  }

  r = fprintf(fp, "%s\n"
                  "%s\n"
                  "%s\n"
                  "%s\n"
                  "%s\n"
                  "%s\n",
                  instance_url,
                  client_id,
                  client_secret,
                  login,
                  oauth_code,
                  oauth_token);

  if (r < 0 || fclose(fp) != 0) {
    dputs("Could not save settings file.\r\n");
    return -1;
  }
  return 0;
}

static int load_settings(void) {
  FILE *fp;
  char c;

#ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
#endif

  fp = fopen("mastsettings", "r");

  instance_url  = malloc(BUF_SIZE);
  client_id     = malloc(50);
  client_secret = malloc(50);
  login         = malloc(50);
  oauth_code    = malloc(50);
  oauth_token   = malloc(50);

  instance_url[0] = '\0';
  client_id[0] = '\0';
  client_secret[0] = '\0';
  login[0] = '\0';
  oauth_code[0] = '\0';
  oauth_token[0] = '\0';
  
  if (fp != NULL) {
    if (fgets(instance_url, BUF_SIZE, fp) > 0)
      *strchr(instance_url, '\n') = '\0';

    if (fgets(client_id, BUF_SIZE, fp) > 0)
      *strchr(client_id, '\n') = '\0';

    if (fgets(client_secret, BUF_SIZE, fp) > 0)
      *strchr(client_secret, '\n') = '\0';

    if (fgets(login, BUF_SIZE, fp) > 0)
      *strchr(login, '\n') = '\0';

    if (fgets(oauth_code, BUF_SIZE, fp) > 0)
      *strchr(oauth_code, '\n') = '\0';

    if (fgets(oauth_token, BUF_SIZE, fp) > 0)
      *strchr(oauth_token, '\n') = '\0';

    fclose(fp);

    cprintf("Login as %s on %s (Y/n)? ", login, instance_url);
    c = cgetc();
    dputs("\r\n");
    if (c == 'n' || c == 'N') {
      goto reenter_settings;
    }

    return 0;
  } else {
reenter_settings:
    /* Invalidate oauth token */
    oauth_token[0] = '\0';

    dputs("Your instance: ");
    cgets(instance_url, BUF_SIZE);
    *strchr(instance_url, '\n') = '\0';

    if (register_app() < 0) {
      return -1;
    }

    dputs("If on a non-US keyboard, use @ instead of arobase.\r\n");
    dputs("Your login: ");
    cgets(login, BUF_SIZE);
    *strchr(login, '\n') = '\0';
    
    return 0;
  }
  return -1;
}

int main(int argc, char **argv) {
  surl_response *response = NULL;
  char *params = malloc(BUF_SIZE);
  FILE *fp = NULL;
  char buf[16];
  char *translit_charset;
  char y;

#ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
#endif

  videomode(VIDEOMODE_80COL);
  screensize(&scrw, &scrh);

  clrscr();
  print_logo(scrw);
  
  y = wherey();
  set_scrollwindow(y, scrh);

  if (load_settings() < 0) {
    set_scrollwindow(0, scrh);
    cgetc();
    exit(1);
  }

  if (!strlen(oauth_token)) {
    if (do_login() < 0) {
      exit(1);
    }
    if (get_oauth_token() < 0) {
      exit(1);
    }
    save_settings();
    dputs("Saved OAuth token.\r\n");

  }
  /* We don't need those anymore */
  free(client_id);
  free(client_secret);
  free(login);
  free(oauth_code);
  client_id = NULL;
  client_secret = NULL;
  login = NULL;
  oauth_code = NULL;

  if (oauth_token == NULL || oauth_token[0] == '\0') {
    dputs("Could not login :(\n");
    cgetc();
    set_scrollwindow(0, scrh);
    exit(1);
  }

  fp = fopen("clisettings", "r");
  translit_charset = US_CHARSET;

  if (fp != NULL) {
    fgets(buf, 16, fp);
    if (strchr(buf, '\n')) {
      *strchr(buf, '\n') = '\0';
    }
    translit_charset = buf;
    fclose(fp);
  }

  snprintf(params, BUF_SIZE, "%s %s %s", instance_url, oauth_token, translit_charset);

  set_scrollwindow(0, scrh);
#ifdef __CC65__
  exec("mastocli", params);
#else
  printf("exec(mastocli %s)\n",params);
#endif
  exit(0);
}