#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "malloc0.h"
#include "surl.h"
#include "dputs.h"
#include "extended_conio.h"
#include "path_helper.h"
#include "file_select.h"
#include "strsplit.h"
#include "scroll.h"
#include "print.h"
#include "cli.h"
#include "compose_header.h"
#include "compose.h"
#include "media.h"
#include "poll.h"
#include "list.h"
#include "math.h"
#include "dget_text.h"
#include "clrzone.h"
#include "scrollwindow.h"
#include "vsdrive.h"
#include "hyphenize.h"
#include "a2_features.h"

char *instance_url = NULL;
char *oauth_token = NULL;
char top = 0;
char writable_lines = NUMLINES;

char n_medias = 0;
char sensitive_medias = 0;
#define MAX_IMAGES 4

char *media_files[4];
char *media_ids[4];
char *media_descriptions[4];

char cw[50] = "";

poll *toot_poll = NULL;

#define COMPOSE_HEIGHT 15
#define COMPOSE_FIELD_HEIGHT 9

static char compose_audience = COMPOSE_PUBLIC;
static char cancelled = 0;
static char should_open_menu = 0;
static char should_resume_composing = 0;

static status *ref_status = NULL;
static char *compose_mode = "c";

static void update_compose_audience(void) {
  gotoxy(0, top + COMPOSE_FIELD_HEIGHT + 1);

  if (has_80cols) {
    cprintf("Audience: (%c) Public  (%c) Unlisted  (%c) Private  (%c) Mention",
          compose_audience == COMPOSE_PUBLIC ? '*':' ',
          compose_audience == COMPOSE_UNLISTED ? '*':' ',
          compose_audience == COMPOSE_PRIVATE ? '*':' ',
          compose_audience == COMPOSE_MENTION ? '*':' ');

    gotoxy(0, top + COMPOSE_FIELD_HEIGHT + 3);
    dputs(translit_charset);
    if(translit_charset[0] != 'U') {
      dputs(": Use ");
      cputc(arobase);
      dputs(" to mention, and # for hashtags.");
    }
  } else {
    cprintf("Audience: %s     ",
            compose_audience_str(compose_audience));
  }
}

static void update_cw(void) {
  clrzone(0, top + COMPOSE_FIELD_HEIGHT + 2, NUMCOLS - (RIGHT_COL_START+1), top + COMPOSE_FIELD_HEIGHT + 2);
  if (has_80cols) {
    if (cw[0] == '\0') {
      dputs("( ) ");
    } else {
      dputs("(*) CW: ");
    }
  }
  if (cw[0] == '\0') {
    dputs("Content warning not set");
  } else {
    dputs(cw);
  }
}
static char dgt_cmd_cb(char c) {
  c = tolower(c);
  switch(c) {
    case 's':    /* send */
      return 1;

    case 'i':
    case 'c':
    case 'v':
      should_open_menu = c;
      return 1;

    case 'p':    compose_audience = COMPOSE_PUBLIC;   break;
    case 'r':    compose_audience = COMPOSE_PRIVATE;  break;
    case 'u':    compose_audience = COMPOSE_UNLISTED; break;
    case 'm':    compose_audience = COMPOSE_MENTION;  break;
    case 'n':    compose_audience = COMPOSE_UNLISTED; break;
    case 'd':    compose_audience = COMPOSE_MENTION;  break;
    case 'x':    cancelled = 1;                       return 1;
    case 'y':    if (!has_80cols) {
                   should_resume_composing = 1;
                   set_scrollwindow(0, NUMLINES);
                   clrscr();
                   compose_show_help();
                   c = cgetc();
                   clrscr();
                   return 1;
                 }
  }
  set_scrollwindow(0, NUMLINES);
  update_compose_audience();
  update_cw();
  print_free_ram();
  set_scrollwindow(top + 1, top + COMPOSE_FIELD_HEIGHT);
  return 0;
}

static void setup_gui(void)
{
  set_scrollwindow(0, NUMLINES);
  clrscr();
  gotoxy(0, 0);

  if (IS_NOT_NULL(ref_status)) {
    if (compose_mode[0] == 'r') {
      writable_lines = NUMLINES - COMPOSE_HEIGHT + 2;
      print_status(ref_status, 0, 0);

      /* we want to make sure we'll have one and
       * only one separator line */
      gotoxy(0, wherey() - 1);
      chline(NUMCOLS - RIGHT_COL_START);
      clrzone(wherex(), wherey(), NUMCOLS - (RIGHT_COL_START+1), wherey());
      dputs("Your reply:\r\n");
      top = wherey();
    }
  }
  chline(NUMCOLS - RIGHT_COL_START);
  gotoxy(0, top + COMPOSE_FIELD_HEIGHT);
  chline(NUMCOLS - RIGHT_COL_START);

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
  char x, y;
  clrscr();
  gotoxy(0, 1);

  if (n_medias == MAX_IMAGES) {
    return;
  }

  dputs("File name: ");
  if (!has_80cols) {
    dputs("\r\n");
  }

  media_files[n_medias] = file_select(0, "Please choose an image");
  if (IS_NULL(media_files[n_medias])) {
    return;
  }

  dputs("\r\nDescription: ");
  media_descriptions[n_medias] = malloc0(512);
  dget_text_multi(media_descriptions[n_medias], 512, NULL, 0);

try_again:
  dputs("\r\nUploading... ");
  x = wherex();
  y = wherey();
  media_ids[n_medias] = api_send_hgr_image(media_files[n_medias],
                                           media_descriptions[n_medias],
                                           &err, x, y, NUMCOLS  - (RIGHT_COL_START+1) - x);
  if (IS_NULL(media_ids[n_medias])) {
    char t;
    dputs("An error happened uploading the file:\r\n");
    dputs(IS_NOT_NULL(err) ? err:"Unknown error");
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
  set_scrollwindow(0, NUMLINES);

  clrzone(0, top + COMPOSE_FIELD_HEIGHT + 2, NUMCOLS - (RIGHT_COL_START+1), top + COMPOSE_FIELD_HEIGHT + 2);
  dputs("(*) CW: ");
  dget_text_multi(cw, sizeof(cw) - 1, NULL, 0);
  update_cw();

  set_scrollwindow(top + 1, top + COMPOSE_FIELD_HEIGHT);
}

static void open_images_menu(void) {
  char c;

  set_scrollwindow(0, NUMLINES);
image_menu:
  clrscr();
  gotoxy(0, 1);

  cprintf("Images %s(%d/%d):\r\n",
          sensitive_medias ? "(SENSITIVE) ":"",
          n_medias, MAX_IMAGES);

  if (IS_NOT_NULL(toot_poll)) {
    dputs("\r\nCan not add images to a toot with a poll.");
    cgetc();
    clrscr();
    return;
  }

  for (c = 0; c < n_medias; c++) {
    char short_desc[55];
    strncpy(short_desc, media_descriptions[c], TL_SPOILER_TEXT_BUF);
    hyphenize(short_desc, TL_SPOILER_TEXT_BUF);

    cprintf("\r\n- %s (%s)"
            "\r\n  %s\r\n",
      media_files[c],
      media_ids[c],
      short_desc);
  }

  print_free_ram();
  gotoxy(0, NUMLINES - 3);
  if (n_medias < MAX_IMAGES) {
    dputs("Enter: add image");
  }
  if (n_medias > 0 && n_medias < MAX_IMAGES) {
    dputs(" - ");
  }
  if (n_medias > 0) {
    cprintf("R: remove image %d", n_medias);
  }
  cprintf("\r\nS: Mark image(s) %ssensitive"
          "\r\nEscape: back to editing",
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

static void add_poll_option(void) {
  char r = toot_poll->options_count + 1;

  if (r > MAX_POLL_OPTIONS) {
    return;
  }

  cprintf("Option %d: ", r);
  r--;
  if (IS_NULL(toot_poll->options[r].title)) {
    toot_poll->options[r].title = malloc0(MAX_POLL_OPTION_LEN + 1);
  }
  dget_text_multi(toot_poll->options[r].title, MAX_POLL_OPTION_LEN, NULL, 0);
  if (toot_poll->options[r].title[0] != '\0') {
    compose_sanitize_str(toot_poll->options[r].title);
    r++;
    toot_poll->options_count = r;
  } else {
    free(toot_poll->options[r].title);
    toot_poll->options[r].title = NULL;
  }
}

static void remove_poll_option(void) {
  char c = toot_poll->options_count;
  if (c == 0) {
    return;
  }
  c--;
  free(toot_poll->options[c].title);
  toot_poll->options[c].title = NULL;
  toot_poll->options_count = c;
}

static void print_poll_header(void) {
  gotoxy(0, 0);
  cprintf("Poll (%d %s):     \r\n"
          "%u options, %s choice\r\n\r\n",
          toot_poll->expires_in_hours < 48 ? 
            toot_poll->expires_in_hours : toot_poll->expires_in_hours / 24,
          toot_poll->expires_in_hours < 48 ? 
            "hours":"days",
          toot_poll->options_count,
          toot_poll->multiple ? "multiple":"single");
}

static void set_poll_duration(void) {
  char c, i;
  do {
    gotoxy(0, NUMLINES - 1);
    dputs("Left/Right:set duration, Enter:validate");
    for (i = 0; i < NUM_POLL_DURATIONS; i++) {
      if (toot_poll->expires_in_hours == compose_poll_durations_hours[i]) {
        break;
      }
    }
    c = cgetc();
    if (c == CH_CURS_LEFT && i > 0) {
      toot_poll->expires_in_hours = compose_poll_durations_hours[i - 1];
    }
    if (c == CH_CURS_RIGHT && i < NUM_POLL_DURATIONS) {
      toot_poll->expires_in_hours = compose_poll_durations_hours[i + 1];
    }
    print_poll_header();
  } while (c != CH_ENTER);
}

static void open_poll_menu(void) {
  char c, next_option_y;

  set_scrollwindow(0, NUMLINES);
poll_menu:
  clrscr();
  gotoxy(0, 1);

  if (IS_NULL(toot_poll)) {
    toot_poll = poll_new();
    toot_poll->expires_in_hours = 24;
  }

  print_poll_header();

  if (n_medias > 0) {
    poll_free(toot_poll);
    toot_poll = NULL;
    dputs("Can not add a poll to a toot with images.");
    cgetc();
    clrscr();
    return;
  }

  for (c = 0; c < toot_poll->options_count; c++) {
    cprintf("Option %d: %s\r\n\r\n",
            c + 1, toot_poll->options[c].title);
  }
  next_option_y = wherey();

  print_free_ram();
  gotoxy(0, NUMLINES - 3);
  if (toot_poll->options_count < MAX_POLL_OPTIONS) {
    dputs("Enter: add option");
  }
  if (toot_poll->options_count > 0 && toot_poll->options_count < MAX_POLL_OPTIONS) {
    dputs(" - ");
  }
  if (toot_poll->options_count > 0) {
    cprintf("R: remove option %d", toot_poll->options_count);
  }

  cprintf("\r\nT: set %s choice; E: set duration"
          "\r\nD: delete poll; Escape: back to editing",
          toot_poll->multiple ? "single":"multiple");

  c = cgetc();
  switch (tolower(c)) {
    case CH_ENTER:
      gotoxy(0, next_option_y);
      add_poll_option();
      break;
    case 'r':
      remove_poll_option();
      break;
    case 'e':
      set_poll_duration();
      break;
    case 't':
      toot_poll->multiple = !toot_poll->multiple;
      break;
    case 'd':
      poll_free(toot_poll);
      toot_poll = NULL;
      /* Fallback to clrscr/return */
    case CH_ESC:
      clrscr();
      return;
  }
  goto poll_menu;
}

static char *handle_compose_input(char *initial_buf) {
  char *text;
  text = malloc0(NUM_CHARS);

  if (IS_NOT_NULL(initial_buf) && initial_buf[0]) {
    int len = min(NUM_CHARS - 3, strlen(initial_buf));

    if (compose_mode[0] == 'r') {
      int i;
      /* Insert initial_buf as handle to reply to */
      text[0] = arobase;
      for (i = 0; i < len; i++) {
        if (initial_buf[i] == '@') {
          initial_buf[i] = arobase;
        }
      }
      strncpy(text + 1, initial_buf, NUM_CHARS - 3);
      text[len + 1] = ' ';
      text[len + 2] = '\0';
    } else {
      strncpy(text, initial_buf, NUM_CHARS);
      text[NUM_CHARS - 1] = '\0';
    }
  } else {
    text[0] = '\0';
  }

resume_composing:
  setup_gui();
  if (IS_NULL(dget_text_multi(text, NUM_CHARS, dgt_cmd_cb, 1))) {
    free(text);
    return NULL;
  }
  if (should_open_menu == 'i') {
    should_open_menu = 0;
    open_images_menu();
    goto resume_composing;
  }
  if (should_open_menu == 'c') {
    should_open_menu = 0;
    open_cw_menu();
    goto resume_composing;
  }
  if (should_open_menu == 'v') {
    should_open_menu = 0;
    open_poll_menu();
    goto resume_composing;
  }
  if (should_resume_composing) {
    should_resume_composing = 0;
    goto resume_composing;
  }

  return text;
}

static void compose_toot(char *initial_buf) {
  char i;
  char *text;

reedit:
  text = handle_compose_input(initial_buf);

  set_scrollwindow(0, NUMLINES);

  if (IS_NOT_NULL(text) && !cancelled) {
    signed char r;
    char *err;

try_again:
    clrscr();
    gotoxy(0, 1);
    dputs("Sending toot...\r\n\r\n");
    r = api_send_toot(compose_mode[0], text, cw, sensitive_medias,
                      IS_NOT_NULL(ref_status) ? ref_status->id : NULL,
                      media_ids, n_medias,
                      toot_poll, compose_audience,
                      &err);
    if (r < 0) {
      char t;

      dputs("\r\nAn error happened sending the toot.\r\n\r\n");

      if (IS_NOT_NULL(err)) {
        dputs(err);
        free(err);
        dputs("\r\n\r\n");
      }

      dputs("Try Sending again or Re-edit ? (s/R)");
      t = cgetc();
      if (tolower(t) == 's') {
        goto try_again;
      } else {
        initial_buf = text;
        /* Leak here */
        goto reedit;
      }
    }
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
  char *text = NULL;

  surl_connect_proxy();
  vsdrive_install();

  surl_user_agent = "Mastodon for Apple II / "VERSION;

  if (argc < 4) {
    dputs("Missing parameters.\n");
  }

  register_start_device();

  instance_url = argv[1];
  oauth_token = argv[2];
  monochrome = (argv[3][0] == '1');
  translit_charset = argv[4];

//  0 2    7
// "US-ASCII"
// "ISO646-IT"  Nothing to do, § for @ and £ for #
// "ISO646-DK"  
// "ISO646-UK"  Nothing to do, @ for @ and £ for #
// "ISO646-DE"  Nothing to do, § for @ and # for #
// "ISO646-SE2"
// "ISO646-FR1" Change @ to 0x5D for § and £ for #
// "ISO646-ES"  Nothing to do, § for @ and £ for #

  switch(translit_charset[7]) {
    case 'F':
      arobase = 0x5D;
      break;
  }

  compose_print_header();

  set_hscrollwindow(RIGHT_COL_START, NUMCOLS - RIGHT_COL_START);
  gotoxy(0, 0);
  if (argc == 7) {
    compose_mode = argv[5];
    ref_status = api_get_status(argv[6], 0);

    if (IS_NOT_NULL(ref_status)) {
      /* Set CW from reference status */
      if (IS_NOT_NULL(ref_status->spoiler_text)) {
        strncpy(cw, ref_status->spoiler_text, sizeof(cw) - 1);
        cw[sizeof(cw) - 1] = '\0';
      }

      /* Set compose audience */
      compose_audience = ref_status->visibility;

      /* If editing, set medias from reference status */
      if (compose_mode[0] == 'e' && ref_status->n_medias > 0) {
        media *medias = api_get_status_media(ref_status->id);
        if (IS_NOT_NULL(medias)) {
          char i;
          n_medias = medias->n_media;
          for (i = 0; i < n_medias; i++) {
            media_ids[i] = strdup(medias->media_id[i]);
            media_files[i] = strdup(strrchr(medias->media_url[i], '/'));
            media_descriptions[i] = strdup(medias->media_alt_text[i]);
          }
          media_free(medias);
        }
      }
    }
  } else if (argc == 6) {
    text = malloc0(strlen(argv[5])+2);

    text[0] = arobase;
    strcpy(text+1, argv[5]);
    while (IS_NOT_NULL(params = strchr(text, '@'))) {
      *params = arobase;
    }
    ref_status = NULL;
  }

  /* Auto-mention parent toot's sender, unless it's us */
  if (IS_NOT_NULL(ref_status) && compose_mode[0] == 'r' && strcmp(ref_status->account->id, my_account->id)) {
    compose_toot(ref_status->account->acct);
  } else if (compose_mode[0] == 'e') {
    char *orig_status = compose_get_status_text(ref_status->id);
    if (IS_NULL(orig_status)) {
      orig_status = strdup("Can not fetch status");
    }
    compose_toot(orig_status);
    free(orig_status);
  } else {
    if (IS_NULL(text)) {
      text = strdup("");
    }
    compose_toot(text);
  }
  free(text);
  set_hscrollwindow(0, NUMCOLS);

  params = malloc0(127);
  snprintf(params, 127, "%s %s %d %s", instance_url, oauth_token, monochrome, translit_charset);
#ifdef __CC65__
  reopen_start_device();

  exec("mastocli", params);
  exit(0);
#else
  printf("exec(mastocli %s)\n",params);
  exit(0);
#endif
}
