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
#include "extended_conio.h"
#include "extended_string.h"
#ifdef __CC65__
#include "dputs.h"
#include "dputc.h"
#include "scroll.h"
#endif
#include "math.h"
#include "oauth.h"

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

#ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
#endif

  fp = fopen("mastsettings", "w");
  if (fp == NULL) {
    printf("Could not open settings file.\n");
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
    printf("Could not save settings file.\n");
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
  client_id     = malloc(BUF_SIZE);
  client_secret = malloc(BUF_SIZE);
  login         = malloc(BUF_SIZE);
  oauth_code    = malloc(BUF_SIZE);
  oauth_token   = malloc(BUF_SIZE);

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

    printf("Login as %s on %s [Y/n]? ", login, instance_url);
    c = cgetc();
    printf("\n");
    if (c == 'n' || c == 'N') {
      goto reenter_settings;
    }

    return 0;
  } else {
reenter_settings:
    printf("Your instance: ");
    cgets(instance_url, BUF_SIZE);
    *strchr(instance_url, '\n') = '\0';

    if (register_app() < 0) {
      return -1;
    }

    printf("If on a non-US keyboard, use @ instead of arobase.\n");
    printf("Your login: ");
    cgets(login, BUF_SIZE);
    *strchr(login, '\n') = '\0';
    
    return 0;
  }
  return -1;
}

int main(int argc, char **argv) {
  surl_response *response = NULL;
  
  videomode(VIDEOMODE_80COL);
  screensize(&scrw, &scrh);

  if (load_settings() < 0) {
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
    printf("Saved OAuth token.\n");
  }

  printf("Ready to go!\n");

  exit(0);
}
