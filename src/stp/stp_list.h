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
#include "dget_text.h"

extern unsigned char NUMCOLS;
#define NUMROWS 24

#define STP_URL_FILE "STPSTARTURL"

#define PAGE_BEGIN 2
#define PAGE_HEIGHT 17

enum StpDataAction {
  KEYBOARD_INPUT,
  SAVE_DIALOG,
  UPDATE_LIST
};

enum HeaderUrlAction {
  URL_SET,
  URL_ADD,
  URL_UP,
  URL_RESTORE
};


extern char **lines;
extern char **nat_lines;
extern unsigned int max_nat_data_size;
extern unsigned char nat_data_static;

#define BUFSIZE 255
#define SEARCH_BUF_SIZE 128

#ifdef __CC65__
/* Do not do that if exec() with argc > 1 is required - cf cc65 exec.s */
#define tmp_buf ((char *)0x300)
#define search_buf ((char *)0x200)
#else
extern char tmp_buf[BUFSIZE];
extern char search_buf[SEARCH_BUF_SIZE];
#endif

extern int num_lines;
extern int cur_line;
extern int cur_display_line;
extern char *data;
extern char *nat_data;

char *stp_url_enter(char *url, char *suffix);
char *stp_url_up(char *url);
char stp_list_scroll(signed char shift);
void stp_list_search(unsigned char new_search);
char *stp_build_login_url(char *url);
char *stp_get_start_url(char *header, char *default_url, cmd_handler_func cmd_cb);
void stp_update_list(char full_update);
unsigned char stp_get_data(char *url);
void stp_print_header(const char *url, enum HeaderUrlAction action);
void stp_print_result(void);
void stp_animate_list(char reset);
void stp_clr_page(void);
void stp_free_data(void);
#endif
