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
#include "malloc0.h"
#include "surl.h"
#include "simple_serial.h"
#include "scrollwindow.h"
#include "strsplit.h"
#include "dgets.h"
#include "dputs.h"
#include "dputc.h"
#include "scroll.h"
#include "extended_conio.h"
#include "math.h"
#include "cli.h"
#include "api.h"
#include "oauth.h"
#include "logo.h"
#include "runtime_once_clean.h"
#include "config.h"

unsigned char scrw, scrh;
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
  if (IS_NULL(fp)) {
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

  instance_url  = malloc0(BUF_SIZE);
  client_id     = malloc0(50);
  client_secret = malloc0(50);
  login         = malloc0(50);
  oauth_code    = malloc0(50);
  oauth_token   = malloc0(50);

  if (IS_NOT_NULL(fp)) {
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

    dputs("Your instance URL: ");
    strcpy(instance_url, "https://");
    dget_text(instance_url, BUF_SIZE, NULL, 0);

    if (instance_url[0] == '\0') {
      goto reenter_settings;
    }
    if (strlen(instance_url) > 70) {
      dputs("URL is too long.\r\n");
      goto reenter_settings;
    }
    if (register_app() < 0) {
      goto reenter_settings;
    }

    dputs("If on a non-US keyboard, use @ instead of arobase.\r\n");
    dputs("Your login: ");
    login[0] = '\0';
    dget_text(login, BUF_SIZE, NULL, 0);

    return 0;
  }
  return -1;
}

int main(int argc, char **argv) {
  char *params;
  char y;

  if (argc > 1) {
    return conf_main(argc, argv);
  }

  params = malloc0(BUF_SIZE);

#ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
#endif

#ifdef __APPLE2ENH__
  videomode(VIDEOMODE_80COL);
#endif
  screensize(&scrw, &scrh);

  clrscr();

  print_logo(scrw);

  y = wherey();
  set_scrollwindow(y, scrh);

  surl_ping();
  surl_user_agent = "Mastodon for Apple II / "VERSION;

  runtime_once_clean();

try_login_again:
  if (load_settings() < 0) {
    set_scrollwindow(0, scrh);
    cgetc();
    exit(1);
  }

  if (!strlen(oauth_token)) {
    if (do_login() < 0 || get_oauth_token() < 0) {
      dputs("\r\nPress a key to try again.\r\n");
      cgetc();
      goto try_login_again;
    }
    save_settings();
    dputs("Saved OAuth token.\r\n");

  }

  if (IS_NULL(oauth_token) || oauth_token[0] == '\0') {
    dputs("Could not login :(\n");
    cgetc();
    set_scrollwindow(0, scrh);
    exit(1);
  }

#if NUMCOLS == 40
  dputs("\r\nHint: Use Ctrl-Y to toggle help menu");
  dputs("\r\nfrom anywhere in the program.");
#endif
  snprintf(params, BUF_SIZE, "%s %s", instance_url, oauth_token);

  set_scrollwindow(0, scrh);
#ifdef __CC65__
  //snprintf(params, BUF_SIZE, "%s %s ISO646-FR1 e 110882478679186108", instance_url, oauth_token);
  //snprintf(params, BUF_SIZE, "%s %s ISO646-FR1", instance_url, oauth_token);
  // exec("mastowrite", params);

  // snprintf(params, BUF_SIZE, "%s %s ISO646-FR1 1 s 112016758824153809", instance_url, oauth_token);
  // exec("mastoimg", params);

  exec("mastocli", params);
#else
  printf("exec(mastocli %s)\n",params);
#endif
  exit(0);
}
