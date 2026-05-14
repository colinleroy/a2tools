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
#include <errno.h>
#include <ctype.h>
#include "surl.h"
#include "simple_serial.h"
#include "strsplit.h"
#include "extended_conio.h"
#include "dget_text.h"
#include "dputc.h"
#include "dputs.h"
#include "clrzone.h"
#include "file_select.h"
#include "scroll.h"
#include "scrollwindow.h"
#include "hgr.h"
#include "a2_features.h"
#include "backup_hgrpage.h"
#include "citoa.h"
#include "malloc0.h"
#include "path_helper.h"
#include "platform.h"
#include "splash.h"

#include "api.h"

#pragma code-name(push, "LC")

unsigned char scrw = 255, scrh = 255;
unsigned char monochrome = 1;

#define BUF_SIZE 511
char gen_buf[BUF_SIZE+1];

void cleanup(void) {
}

static unsigned char dhgr_init_done = 0;
static unsigned char is_dhgr;
static char last_displayed[16] = "";

void display_post(post_t *post) {
  /* If re-displaying same post, spare the query. */
  if (!strcmp(post->id, last_displayed)) {
    goto print_description;
  }

  surl_start_request(NULL, 0, post->image_url, SURL_METHOD_GET);

  if (!surl_response_ok()) {
    return;
  }
  
  simple_serial_putc(has_128k && can_dhgr ? SURL_CMD_DHGR : SURL_CMD_HGR);
  simple_serial_putc(monochrome);
  simple_serial_putc(HGR_SCALE_FULL);

  if (simple_serial_getc() == SURL_ERROR_OK) {
    size_t len;

    surl_read_with_barrier((char *)&len, 2);
    len = ntohs(len);
    is_dhgr = surl_read_image_to_screen(len);

print_description:
    if (!dhgr_init_done) {
      init_graphics(monochrome, is_dhgr);
      hgr_mixon(); /* Legend on by default */
      dhgr_init_done = 1;
      set_scrollwindow(20, scrh);
    }
    strcpy(last_displayed, post->id);

    clrscr();
    dputs(post->description);
    gotoxy(0, 3);
    dputs(" By ");
    dputs(post->author);
    dputs(" on ");
    dputs(post->date);
    dputs (" (");
    cutoa(post->comment_count);
    dputs(" comments)");
  }
}

static void info(char *str) {
  clrscr();
  dputs(str);
  dputs("\r\nPress a key to return...");
  cgetc();
}

static char *load_creds(void) {
  char *creds = malloc0(256);
  int fd;

  if (!creds) {
    return NULL;
  }

  reopen_start_device();
  fd = open("SFLOGIN", O_RDONLY);
  if (fd) {
    read(fd, creds, 256);
    close(fd);
    return creds;
  } else {
    free(creds);
    return NULL;
  }
}

static void save_creds(void) {
  char *creds = api_get_creds();
  int fd;

  if (!creds) {
    return;
  }

  reopen_start_device();
  fd = open("SFLOGIN", O_CREAT|O_WRONLY);
  if (fd) {
    write(fd, creds, strlen(creds));
    close(fd);
  }
  free(creds);
}

static void do_text(void) {
  set_scrollwindow(0, scrh);
  clrscr();
  init_text();
  dhgr_init_done = 0;
}

static char prepare_post_upload(void) {
  char x, y, r;
  char *filename, *description;

  do_text();

  dputs("File name: ");
  if (!has_80cols) {
    dputs("\r\n");
  }

  filename = file_select(0, "Please choose an image");
  if (IS_NULL(filename)) {
    return EIO;
  }

  dputs("\r\nDescription: ");
  description = malloc0(512);
  dget_text_multi(description, 512, NULL, 0);

  dputs("\r\nUploading... ");
  x = wherex();
  y = wherey();
  r = api_post_hgr_image(filename,
                         description,
                         x, y, scrw - x);
  switch (r) {
    case 0:
      break;
    case EIO:
      info("Could not open file.");
      break;
    case ENOENT:
      info("Could not upload file.");
      break;
    default:
      info("Unknown error.");
  }
  free(description);
  free(filename);
  return r;
}

static void prepare_comment_upload(post_t *post) {
  char *comment = malloc0(256);

  dputs("Your comment: ");
  dget_text_multi(comment, 255, NULL, 0);
  if (comment[0] != '\0') {
    api_post_comment(post, comment);
  }

  free(comment);
}

#pragma code-name(pop)

static void view_comments(post_t *post) {
  int i;

  do_text();
  for (i = 0; i < post->comment_count; i++) {
    comment_t *comment = api_get_comment(post, i);
    if (wherey() > scrh - 2) {
      dputs("Press a key to continue\r\n");
      cgetc();
    }
    if (IS_NULL(comment)) {
      dputs("Can not load comment :-/\r\n");
    } else {
      dputs("From ");
      dputs(comment->author);
      dputs(" on ");
      dputs(comment->date);
      dputs(":\r\n");
      dputs(comment->text);
      dputs("\r\n");
      dputs("\r\n");
      comment_free(comment);
    }
  }
  dputs("\r\nPress a key to return...");
  dputs(" or 'C' to comment.\r\n");
  i = cgetc();
  if (tolower(i) == 'c') {
    prepare_comment_upload(post);
  }
}

int main(void) {
  post_t *post = NULL;
  unsigned char shift = 1, c;

  register_start_device();

  reserve_auxhgr_file();  /* Sets can_dhgr */

  atexit(&cleanup);

  try_videomode(VIDEOMODE_80COL);

  serial_throbber_set((void *)0x07F7);

  screensize(&scrw, &scrh);
  surl_ping();
  surl_user_agent = "SixForty "VERSION"/Apple II";

#ifdef __APPLE2__
  init_graphics(monochrome, 0);
  hgr_mixon();
  set_scrollwindow(20, scrh);
  clrscr();
#endif

  cprintf("Welcome to SixForty. %zuB free\r\n", _heapmaxavail());

  while (api_login(load_creds()) != 0) {
    clrscr();
    dputs("Login failed.\r\n");
  }
  save_creds();

  while (1) {
    post_free(post);
    post = api_get_post(shift);
    if (IS_NULL(post)) {
      clrscr();
      dputs("Could not load post :-/\r\n");
    } else {
display_again:
      display_post(post);
    }
get_command:
    shift = 0;
    c = tolower(cgetc());
    switch (c) {
      case CH_CURS_LEFT:      /* previous */
        shift = -1;
        break;
      case CH_CURS_RIGHT:     /* next */
        shift = 1;
        break;
      case 'h':
        info("Left: previous post; Next: next post; L: toggle legend; M: toggle color\r\n"
             "P: Post an image; D: Delete image; V: View comments; C: Comment; Q: Quit");
        goto display_again;
      case 'm':
        monochrome = !monochrome;
        dhgr_init_done = 0;
        last_displayed[0] = '\0'; /* Force reload to re-convert */
        goto display_again;
      case 'q':
        exit(0);
      case 'd':
        if (api_delete_post(post) != 0) {
          info("Could not delete post.");
        }
        break;
      case 'p':
        prepare_post_upload();
        break;
      case 'c':
        clrscr();
        prepare_comment_upload(post);
        break;
      case 'v':
        view_comments(post);
        break;
      case 'l':               /* legend */
        if (hgr_mix_is_on) {
          hgr_mixoff();
        } else {
          hgr_mixon();
        }
        /* Fallthrough to next command */
      default:
        goto get_command;
    }
  }
}
