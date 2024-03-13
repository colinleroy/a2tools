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

#ifndef __stp_h__
#define __stp_h__

#include "surl.h"

#define BUFSIZE 255
#define STP_URL_FILE "STPSTARTURL"

#define PAGE_BEGIN 2
#define PAGE_HEIGHT 17

enum StpDataAction {
  KEYBOARD_INPUT,
  SAVE_DIALOG,
  UPDATE_LIST
};

extern char **lines;
extern int num_lines;
extern int cur_line;
extern int cur_display_line;
extern char data[STP_DATA_SIZE];

char *stp_url_enter(char *url, char *suffix);
char *stp_url_up(char *url);
char stp_list_scroll(signed char shift);
void stp_list_search(unsigned char new_search);
char *stp_build_login_url(char *url);
char *stp_get_start_url(void);
void stp_update_list(char full_update);
int stp_get_data(char *url, const surl_response **resp);
void stp_print_header(char *url);
void stp_print_result(const surl_response *response);

#endif
