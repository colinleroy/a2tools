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
#include "platform.h"
#include "scroll.h"
#include "scrollwindow.h"
#include "vsdrive.h"
#include "hgr.h"
#include "a2_features.h"
#include "backup_hgrpage.h"
#include "citoa.h"
#include "malloc0.h"
#include "path_helper.h"
#include "platform.h"
#include "splash.h"

#include "api.h"

unsigned char scrw = 255, scrh = 255;
unsigned char monochrome = 1;

unsigned long max_id = 0;
unsigned char in_slideshow = 0;
unsigned char in_random = 0;

char gen_buf[BUF_SIZE+1];

static unsigned char dhgr_init_done = 0;
static unsigned char is_dhgr;
static char last_displayed[16] = "";

static void do_text(void) {
  set_scrollwindow(0, scrh);
  clrscr();
  init_text();
  dhgr_init_done = 0;
}

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
    char *cam;

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
    if (has_80cols) {
      dputs(post->description);
      gotoxy(0, 2);
    }
    dputs("By ");
    dputs(post->author);
    dputs(" on ");
    dputs(post->date);
    dputs (" (");
    cutoa(post->comment_count);
    dputs(" comments)");
    cam = cameras[post->camera_id];
    if (IS_NOT_NULL(cam)) {
      if (!has_80cols) {
        dputs("\r\n");
      } else {
        dputs(", ");
      }
      dputs(cam);
    }
    dputs("\r\n");
    dputs("Nav mode: ");
    dputs(in_slideshow ? "Slideshow,":"Manual,");
    dputs(in_random ? " Random":" Sequential");
  }
}

#pragma code-name(push, "LC")

static void info(char *str) {
  clrscr();
  dputs(str);
  dputs("\r\nPress a key to return...");
  cgetc();
}

static char *load_creds(void) {
  char *creds = malloc0(256);
  char *p;
  int fd;

  if (!creds) {
    return NULL;
  }

  reopen_start_device();
  fd = open("SFSETTINGS", O_RDONLY);
  if (fd) {
    read(fd, creds, 256);
    p = strrchr(creds, '\n');
    if (IS_NOT_NULL(p)) {
      monochrome = *(++p);
    }
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

  reopen_start_device();
  fd = open("SFSETTINGS", O_CREAT|O_WRONLY);
  if (fd) {
    if (IS_NOT_NULL(creds)) {
      write(fd, creds, strlen(creds));
    } else {
      write(fd, "\n", 1);
    }
    write(fd, "\n", 1);
    write(fd, &monochrome, 1);
    close(fd);
  }
  free(creds);
}

static void list_cameras(unsigned char max_lines) {
  int i, lines_printed;
  unsigned char even = 0;

  for (i = lines_printed = 0; i < num_cameras; i++) {
    char *cam = cameras[i];
    if (IS_NOT_NULL(cam)) {
      cprintf("%d: %s", i, cam);
      if ((even = !even) && has_80cols) {
        gotox(40);
      } else {
        dputs("\r\n");
        lines_printed++;
      }
      if (lines_printed == max_lines) {
        dputs("Press a key to continue...");
        cgetc();
        dputs("\r\n");
        lines_printed = 0;
      }
    }
  }
}

static unsigned char get_camera_from_list(unsigned char max_lines) {
ask_cam:
  dputs("\r\nCamera (0 for list, number to specify, Enter to skip):\r\n");
  small_buf[0] = '\0';
  dget_text_multi(small_buf, 3, NULL, 0);

  switch (small_buf[0]) {
    case '\0':
      return 0;
    case '0':
      list_cameras(max_lines);
      goto ask_cam;
    default:
      return atoi(small_buf);
  }
}

static char prepare_post_upload(void) {
  char x, y, r, cam_id;
  char *filename, *description;

  do_text();

  filename = file_select(0, "Select an HGR, DHGR, QTK or JPG image");
  if (IS_NULL(filename)) {
    return EIO;
  }

  dputs("\r\nDescription: ");
  description = malloc0(512);
  dget_text_multi(description, 512, NULL, 0);

  cam_id = get_camera_from_list(22);

  dputs("\r\nUploading... ");
  x = wherex();
  y = wherey();
  r = api_post_image(filename,
                     description, cam_id,
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

static unsigned char wait_keypress(unsigned char seconds) {
  platform_interruptible_msleep(1000*seconds);
  return kbhit();
}

#pragma code-name(pop)

void cleanup(void) {
  save_creds();
}

static void prepare_post_edit(post_t *post) {
  char *tmp;
  char cam_id;

  /* Very naive permission check, offloading the work to the server.
   * Sorry Brian :-|
   */
  if (api_patch_post(post, 'S', "description", post->description) != 0) {
    info("You don't have permission to edit that post.");
    return;
  }
  clrscr();
  dputs("\r\nDescription: ");
  tmp = realloc(post->description, 512);
  if (IS_NULL(tmp)) {
    info("Out of memory.");
    return;
  }
  post->description = tmp;
  dget_text_multi(post->description, 511, NULL, 0);
  api_patch_post(post, 'S', "description", post->description);

  cam_id = get_camera_from_list(3);
  if (cam_id) {
    char cam_id_str[4];
    snprintf(cam_id_str, 3, "%d", cam_id);
    api_patch_post(post, 'B', "camera_id", cam_id_str);
  }
}

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

  vsdrive_install();

#ifdef __APPLE2__
  init_graphics(monochrome, 0);
  hgr_mixon();
  set_scrollwindow(20, scrh);
  clrscr();
#endif

  dputs("Welcome to SixForty. Press H for help.\r\n");
  
  while (api_login(load_creds()) != 0) {
    clrscr();
    dputs("Login failed.\r\n");
  }

  api_load_cameras();

  _randomize();

  while (1) {
    post_free(post);
    if (in_random) {
      unsigned long new_id = (unsigned long)((((unsigned long)rand() * (max_id)))/RAND_MAX);
      post = api_get_post_by_id(new_id);
      if (post == NULL) {
        continue; /* keep iterating */
      }
    } else {
      post = api_get_next_post(shift);
    }
    if (IS_NULL(post)) {
      clrscr();
      dputs("Could not load post :-/\r\n");
    } else {
      /* Store max post id for slideshow */
      if (max_id == 0) {
        max_id = strtoul(post->id, NULL, 10);
      }
display_again:
      display_post(post);
    }
get_command:
    shift = 0;
    if (wait_keypress(10)) {
      c = tolower(cgetc());
      in_slideshow = 0;
    } else {
      if (in_slideshow) {
        shift = 1;
        continue; /* Back to while */
      }
      goto get_command;
    }
    switch (c) {
      case CH_CURS_LEFT:      /* previous */
        shift = -1;
        break;
      case CH_CURS_RIGHT:     /* next */
        shift = 1;
        break;
      case 'h':
        do_text();
        info("Left:  Previous post\r\n"
             "Right: Next post\r\n"
             "\r\n"
             "L:     Toggle legend\r\n"
             "M:     Toggle color\r\n"
             "\r\n"
             "P:     Post image\r\n"
             "D:     Delete image\r\n"
             "E:     Edit image\r\n"
             "V:     View comments\r\n"
             "C:     Comment\r\n"
             "\r\n"
             "S:     Slideshow\r\n"
             "R:     Toggle random mode\r\n"
             "\r\n"
             "Q:     Quit");
        goto display_again;
      case 'm':
        monochrome = !monochrome;
        dhgr_init_done = 0;
        last_displayed[0] = '\0'; /* Force reload to re-convert */
        goto display_again;
      case 'q':
        exit(0);
      case 'd':
        clrscr();
        dputs("Delete this post? (y/N)");
        if (tolower(cgetc()) != 'y') {
          break;
        }
        if (api_delete_post(post) != 0) {
          info("Could not delete post.");
        }
        break;
      case 'p':
        prepare_post_upload();
        break;
      case 'e':
        prepare_post_edit(post);
        break;
      case 'c':
        clrscr();
        prepare_comment_upload(post);
        break;
      case 'v':
        view_comments(post);
        break;
      case 's':
        in_slideshow = 1;
        break;
      case 'r':
        in_random = !in_random;
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
