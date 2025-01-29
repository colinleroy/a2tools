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
#include <fcntl.h>
#include <ctype.h>
#include "malloc0.h"
#include "surl.h"
#include "simple_serial.h"
#include "scrollwindow.h"
#include "strsplit.h"
#include "dget_text.h"
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
#include "login_data.h"

unsigned char scrw, scrh;

login_data_t login_data;

char *instance_url  = login_data.instance_url;
char *client_id     = login_data.client_id;
char *client_secret = login_data.client_secret;
char *login         = login_data.login;
char *oauth_code    = login_data.oauth_code;
char *oauth_token   = login_data.oauth_token;

static int save_settings(void) {
  int fd;
  int r;

  fd = open("mastsettings", O_WRONLY|O_CREAT);
  if (fd < 0) {
    dputs("Could not open settings file.\r\n");
    return -1;
  }

  r = write(fd, &login_data, sizeof(login_data));

  if (r != sizeof(login_data)) {
    close(fd);
err_close:
    clrscr();
    dputs("Could not save settings file.\r\n");
    cgetc();
    return -1;
  }
  if (close(fd) != 0) {
    goto err_close;
  }

  return 0;
}

static char read_settings(void) {
  int fd;
  int r;

#ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
#endif

  fd = open("mastsettings", O_RDONLY);

  if (fd > 0) {
    r = read(fd, &login_data, sizeof(login_data));
    close(fd);
    if (r == sizeof(login_data)) {
      return 0;
    }
  }
  bzero(&login_data, sizeof(login_data));
  return -1;
}

static void get_settings(void) {
  char c;

  /* If we could read settings, don't skip the question. */
  if (login[0]) {
    cprintf("Login as %s on %s (Y/n)? ", login, instance_url);
    c = tolower(cgetc());
    dputs("\r\n");
    if (c != 'n') {
      return;
    }
  }

reenter_settings:
  /* Invalidate oauth token */
  oauth_token[0] = '\0';
  /* unlink state file */
  unlink(STATE_FILE);

  dputs("Your instance URL: ");
  strcpy(instance_url, "https://");
  dget_text_single(instance_url, 70, NULL);

  if (instance_url[0] == '\0') {
    goto reenter_settings;
  }
  if (register_app() < 0) {
    goto reenter_settings;
  }

#ifdef __APPLE2ENH__
  dputs("If your Apple II has a non-US keyboard, use @ instead of arobase.\r\n");
#endif

reenter_login:
  dputs("Your login: ");
  login[0] = '\0';
  dget_text_single(login, 50, NULL);
  if (login[0] == '\0') {
    goto reenter_login;
  }
}

int main(int argc, char **argv) {
  char *params;
  char y;

  params = malloc0(127);

#ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
#endif

#ifdef __APPLE2ENH__
  videomode(VIDEOMODE_80COL);
#endif
  screensize(&scrw, &scrh);

  clrscr();

  print_logo();

  y = wherey();
  set_scrollwindow(y, NUMLINES);

  surl_ping();
  surl_user_agent = "Mastodon for Apple II / "VERSION;

  runtime_once_clean();

  /* Did we read our config file, and are we configuring? */
  if (read_settings() == 0 && argc == 1) {
    goto start_main_ui;
  }

try_login_again:
  get_settings();

  if (!strlen(oauth_token)) {
    if (do_login() < 0 || get_oauth_token() < 0) {
      dputs("\r\nPress a key to try again.\r\n");
      cgetc();
      goto try_login_again;
    }
    dputs("Saved OAuth token.\r\n");
do_cli_config_anyway:
    config_cli();
    save_settings();
  } else if (argc > 1) {
    goto do_cli_config_anyway;
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

start_main_ui:
  snprintf(params, 127, "%s %s %d %s", instance_url, oauth_token, login_data.monochrome, login_data.charset);

  set_scrollwindow(0, scrh);
#ifdef __CC65__
  //snprintf(params, 127, "%s %s 1 ISO646-FR1 e 110882478679186108", instance_url, oauth_token);
  //snprintf(params, 127, "%s %s 1 ISO646-FR1", instance_url, oauth_token);
  // exec("mastowrite", params);

  // snprintf(params, 127, "%s %s 1 ISO646-FR1 s 112016758824153809", instance_url, oauth_token);
  // exec("mastoimg", params);

  exec("mastocli", params);
#else
  printf("exec(mastocli %s)\n",params);
#endif
  exit(0);
}
