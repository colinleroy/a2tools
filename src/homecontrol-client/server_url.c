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
#include <errno.h>
#include "constants.h"
#ifdef __CC65__
#include <conio.h>
#else
#include "extended_conio.h"
#endif
#include "dgets.h"

static char* server_url = NULL;
const char *get_server_root_url(void) {
  FILE *fp;

#ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
#endif

  if (server_url != NULL) {
    return server_url;
  }
  else {
    server_url = malloc(BUFSIZE);
    fp = fopen(SRV_URL_FILE,"r");
    if (fp != NULL) {
      fgets(server_url, BUFSIZE, fp);
      fclose(fp);
      *strchr(server_url,'\n') = '\0';
      return server_url;
    }

    gotoxy(0, 10);
    printf("Please enter the server's root URL,\n");
    printf("Example: http://homecontrol.lan/a2domo\n");
    printf("\nURL: ");
    server_url[0] = '\0';
    dget_text(server_url, BUFSIZE, NULL, 0);

    *strchr(server_url,'\n') = '\0';

    fp = fopen(SRV_URL_FILE, "wb");
    if (fp != NULL) {
      fprintf(fp, "%s\n", server_url);
      fclose(fp);
    } else {
      printf("Can't save URL: %s\n", strerror(errno));
      exit(1);
    }

    return server_url;
  }
}
