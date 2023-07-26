#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#ifdef __APPLE2__
#include <apple2enh.h>
#endif
#include "surl.h"
#ifdef __CC65__
#include <conio.h>
#include "dputs.h"
#else
#include "extended_conio.h"
#endif
#include "strsplit.h"
#include "scroll.h"
#include "print.h"
#include "cli.h"
#include "compose_header.h"
#include "compose.h"
#include "list.h"
#include "math.h"
#include "dgets.h"
#include "clrzone.h"
#include "scrollwindow.h"

char *instance_url = NULL;
char *oauth_token = NULL;
unsigned char scrw, scrh;
char top = 0;

char n_medias = 0;
char sensitive_medias = 0;
#define MAX_IMAGES 4

char *media_files[4];
char *media_ids[4];
char *media_descriptions[4];

char cw[50] = "";

#define COMPOSE_HEIGHT 15
#define COMPOSE_FIELD_HEIGHT 9

static char compose_audience = COMPOSE_PUBLIC;
static char cancelled = 0;
static char should_open_images_menu = 0;
static char should_open_cw_menu = 0;
static status *reply_to = NULL;

static void update_compose_audience(void) {
  gotoxy(0, top + COMPOSE_FIELD_HEIGHT + 1);
  cprintf("Audience: (%c) Public  (%c) Unlisted  (%c) Private  (%c) Mention",
        compose_audience == COMPOSE_PUBLIC ? '*':' ',
        compose_audience == COMPOSE_UNLISTED ? '*':' ',
        compose_audience == COMPOSE_PRIVATE ? '*':' ',
        compose_audience == COMPOSE_MENTION ? '*':' ');

  gotoxy(0, top + COMPOSE_FIELD_HEIGHT + 3);
  cputs(translit_charset);
  if(!strcmp(translit_charset, "ISO646-FR1")) {
    cputs(": Use ] to mention, and # for hashtags.");
  } /* FIXME add other local charsets */
}

static void update_cw(void) {
  clrzone(0, top + COMPOSE_FIELD_HEIGHT + 2, scrw - LEFT_COL_WIDTH - 2, top + COMPOSE_FIELD_HEIGHT + 2);
  gotoxy(0, top + COMPOSE_FIELD_HEIGHT + 2);
  if (cw[0] == '\0') {
    cputs("( ) Content warning not set");
  } else {
    cputs("(*) CW: ");
    cputs(cw);
  }
}
static char dgt_cmd_cb(char c) {
  char x, y;
  switch(tolower(c)) {
    case 's':    return 1;
    case CH_ESC: cancelled = 1;                       return 1;
    case 'i':    should_open_images_menu = 1;         return 1;
    case 'c':    should_open_cw_menu = 1;             return 1;
    case 'p':    compose_audience = COMPOSE_PUBLIC;   break;
    case 'r':    compose_audience = COMPOSE_PRIVATE;  break;
    case 'u':    compose_audience = COMPOSE_UNLISTED; break;
    case 'm':    compose_audience = COMPOSE_MENTION;  break;
  }
  x = wherex();
  y = wherey();
  set_scrollwindow(0, scrh);
  update_compose_audience();
  update_cw();
  print_free_ram();
  set_scrollwindow(top + 1, top + COMPOSE_FIELD_HEIGHT);
  gotoxy(x, y);
  return 0;
}

static void setup_gui(void)
{
  char scrolled;

  set_scrollwindow(0, scrh);
  clrscr();
  gotoxy(0, 0);

  if (reply_to) {
    print_status(reply_to, 0, &scrolled);
    if (wherey() > scrh - COMPOSE_HEIGHT) {
      clrzone(0, scrh - COMPOSE_HEIGHT, scrw - LEFT_COL_WIDTH - 2, scrh - 1);
      gotoxy(0, scrh - COMPOSE_HEIGHT);
      chline(scrw - LEFT_COL_WIDTH - 1);
    }
    cputs("Your reply:\r\n");
    top = wherey();
  }
  chline(scrw - LEFT_COL_WIDTH - 1);
  gotoxy(0, top + COMPOSE_FIELD_HEIGHT);
  chline(scrw - LEFT_COL_WIDTH - 1);

  update_compose_audience();
  update_cw();
  print_free_ram();

  set_scrollwindow(top + 1, top + COMPOSE_FIELD_HEIGHT);

  gotoxy(0, 0);
}

static void remove_image() {
  if (n_medias == 0) {
    return;
  }

  n_medias--;
  free(media_files[n_medias]);
  free(media_ids[n_medias]);
  free(media_descriptions[n_medias]);
}

static void add_image() {
  char *err = NULL;
  clrscr();
  gotoxy(0, 1);

  if (n_medias == MAX_IMAGES) {
    return;
  }
  dputs("Please insert media disk if needed. If you switch disks,\r\n"
          "please enter full path to file, like /VOLNAME/FILENAME\r\n"
          "\r\n");

  dputs("File name: ");
  media_files[n_medias] = malloc(32);
  media_files[n_medias][0] = '\0';
  dget_text(media_files[n_medias], 32, NULL);

  dputs("\r\nDescription: ");
  media_descriptions[n_medias] = malloc(512);
  media_descriptions[n_medias][0] = '\0';
  dget_text(media_descriptions[n_medias], 512, NULL);

try_again:
  dputs("\r\nUploading...\r\n");
  media_ids[n_medias] = api_send_hgr_image(media_files[n_medias],
                                           media_descriptions[n_medias],
                                           &err);
  if (media_ids[n_medias] == NULL) {
    char t;
    dputs("An error happened uploading the file:\r\n");
    dputs(err != NULL ? err:"Unknown error");
    dputs("\r\n\r\nTry again? (y/n)");
    t = cgetc();
    if (tolower(t) != 'n') {
      goto try_again;
    } else {
      free(media_files[n_medias]);
      free(media_descriptions[n_medias]);
    }
  } else {
    n_medias++;
  }
}

static void open_cw_menu(void) {
  set_scrollwindow(0, scrh);

  clrzone(0, top + COMPOSE_FIELD_HEIGHT + 2, scrw - LEFT_COL_WIDTH - 2, top + COMPOSE_FIELD_HEIGHT + 2);
  gotoxy(0, top + COMPOSE_FIELD_HEIGHT + 2);
  cputs("(*) CW: ");
  dget_text(cw, sizeof(cw) - 1, NULL);
  update_cw();

  set_scrollwindow(top + 1, top + COMPOSE_FIELD_HEIGHT);
}

static void open_images_menu(void) {
  char c;

  set_scrollwindow(0, scrh);
image_menu:
  clrscr();
  gotoxy(0, 1);

  cprintf("Images %s(%d/%d):\r\n",
          sensitive_medias ? "(SENSITIVE) ":"",
          n_medias, MAX_IMAGES);

  for (c = 0; c < n_medias; c++) {
    char short_desc[55];
    strncpy(short_desc, media_descriptions[c], 54);
    if (strlen(short_desc) > 51) {
      short_desc[51] = short_desc[52] = short_desc[53] = '.';
    }
    cprintf("\r\n- %s (%s)"
            "\r\n  %s\r\n",
      media_files[c],
      media_ids[c],
      short_desc);
  }

  print_free_ram();
  gotoxy(0, scrh -2);
  if (n_medias < MAX_IMAGES) {
    cputs("Enter: add image");
  }
  if (n_medias > 0 && n_medias < MAX_IMAGES) {
    cputs(" - ");
  }
  if (n_medias > 0) {
    cprintf(" - R: remove image %d", n_medias);
  }
  cprintf("\r\nS: Mark image(s) %ssensitive - Escape: back to editing",
          sensitive_medias ? "not ":"");

  c = cgetc();
  switch (tolower(c)) {
    case CH_ENTER:
      add_image();
      break;
    case 'r':
      remove_image();
      break;
    case 's':
      sensitive_medias = !sensitive_medias;
      break;
    case CH_ESC:
      clrscr();
      return;
  }
  goto image_menu;
}

static char *handle_compose_input(char *reply_to_account) {
  char *text;
  text = malloc(500);

  if (reply_to_account && reply_to_account[0]) {
    int len = strlen(reply_to_account);
    int i;

    for (i = 0; i < len; i++) {
      if (reply_to_account[i] == '@') {
        reply_to_account[i] = arobase;
        break; /* no need to continue, there can only be one */
      }
    }
    text[0] = arobase;
    strncpy(text + 1, reply_to_account, 497);
    text[len + 1] = ' ';
    text[len + 2] = '\0';
  } else {
    text[0] = '\0';
  }

resume_composing:
  setup_gui();
  if (dget_text(text, 500, dgt_cmd_cb) == NULL) {
    free(text);
    text = NULL;
    goto out;
  }
  if (should_open_images_menu) {
    should_open_images_menu = 0;
    open_images_menu();
    goto resume_composing;
  }
  if (should_open_cw_menu) {
    should_open_cw_menu = 0;
    open_cw_menu();
    goto resume_composing;
  }
out:
  return text;
}

static void compose_toot(char *reply_to_account) {
  char i;
  char *text;

  text = handle_compose_input(reply_to_account);

  set_scrollwindow(0, scrh);

  if (text && !cancelled) {
    signed char r;

try_again:
    clrscr();
    gotoxy(0, 1);
    cputs("Sending toot...\r\n\r\n");
    r = api_send_toot(text, cw, sensitive_medias,
                      reply_to ? reply_to->id : NULL,
                      media_ids, n_medias,
                      compose_audience);
    if (r < 0) {
      char t;

      cputs("\r\nAn error happened sending the toot.\r\n\r\nTry again? (y/n)");
      t = cgetc();
      if (tolower(t) != 'n') {
        goto try_again;
      }
    }
  }

  if (n_medias > 0) {
    cputs("Please re-insert Mastodon disk if needed.\r\n\r\n");
    cgetc();
  }

  for (i = 0; i < n_medias; i++) {
    free(media_ids[i]);
    free(media_files[i]);
    free(media_descriptions[i]);
  }
  free(text);
}

int main(int argc, char **argv) {
  char *params;
  if (argc < 4) {
    cputs("Missing instance_url, oauth_token and/or charset parameters.\n");
  }

  videomode(VIDEOMODE_80COL);
  screensize(&scrw, &scrh);

  instance_url = argv[1];
  oauth_token = argv[2];
  translit_charset = argv[3];
  if(!strcmp(translit_charset, "ISO646-FR1")) {
    /* Cleaner than 'à' */
    arobase = ']';
  }
  compose_print_header();

  set_hscrollwindow(LEFT_COL_WIDTH + 1, scrw - LEFT_COL_WIDTH - 1);
  gotoxy(0, 0);
  if (argc == 5) {
    reply_to = api_get_status(argv[4], 0);
  } else {
    reply_to = NULL;
  }

  /* Auto-mention parent toot's sender, unless it's us */
  if (reply_to != NULL && strcmp(reply_to->account->id, my_account->id)) {
    compose_toot(reply_to->account->acct);
  } else {
    compose_toot("");
  }
  set_hscrollwindow(0, scrw);

  params = malloc(127);
  snprintf(params, 127, "%s %s", instance_url, oauth_token);
#ifdef __CC65__
  exec("mastocli", params);
  exit(0);
#else
  printf("exec(mastocli %s)\n",params);
  exit(0);
#endif
}
