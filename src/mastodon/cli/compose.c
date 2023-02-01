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
#else
#include "extended_conio.h"
#endif
#include "strsplit.h"
#include "dputs.h"
#include "dputc.h"
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

#define COMPOSE_HEIGHT 15
#define COMPOSE_FIELD_HEIGHT 9

static char compose_audience = COMPOSE_PUBLIC;
static char cancelled = 0;
static status *reply_to = NULL;

static void update_compose_audience(void) {
  gotoxy(0, top + COMPOSE_FIELD_HEIGHT + 1);
  cprintf("Audience: (%c) Public  (%c) Unlisted  (%c) Private  (%c) Mention",
        compose_audience == COMPOSE_PUBLIC ? '*':' ',
        compose_audience == COMPOSE_UNLISTED ? '*':' ',
        compose_audience == COMPOSE_PRIVATE ? '*':' ',
        compose_audience == COMPOSE_MENTION ? '*':' ');
  gotoxy(0, top + COMPOSE_FIELD_HEIGHT + 3);
  dputs(translit_charset);
  if(!strcmp(translit_charset, "ISO646-FR1")) {
    dputs(": Use ] to mention, and # for hashtags.");
  } /* FIXME add other local charsets */
}

char dgt_cmd_cb(char c) {
  char x, y;
  switch(tolower(c)) {
    case 's':    return 1;
    case CH_ESC: cancelled = 1; return 1;
    case 'p': compose_audience = COMPOSE_PUBLIC; break;
    case 'r': compose_audience = COMPOSE_PRIVATE; break;
    case 'u': compose_audience = COMPOSE_UNLISTED; break;
    case 'm': compose_audience = COMPOSE_MENTION; break;
  }
  x = wherex();
  y = wherey();
  set_scrollwindow(0, scrh);
  update_compose_audience();
  set_scrollwindow(top + 1, top + COMPOSE_FIELD_HEIGHT);
  gotoxy(x, y);
  return 0;
}

static char *handle_compose_input(void) {
  char *text;
  text = malloc(500);
  if (dget_text(text, 500, dgt_cmd_cb) == NULL) {
    free(text);
    text = NULL;
  }
  return text;
}

void compose_toot(void) {
  char *text;
  
  top = wherey();
  text = NULL;

  chline(scrw - LEFT_COL_WIDTH - 1);
  gotoxy(0, top + COMPOSE_FIELD_HEIGHT);
  chline(scrw - LEFT_COL_WIDTH - 1);

  update_compose_audience();

  set_scrollwindow(top + 1, top + COMPOSE_FIELD_HEIGHT);

  gotoxy(0, 0);
  text = handle_compose_input();

  set_scrollwindow(0, scrh);

  if (text && !cancelled) {
    api_send_toot(text, reply_to ? reply_to->id : NULL, compose_audience);
  }

  free(text);
}


int main(int argc, char **argv) {
  char *params;
  if (argc < 4) {
    dputs("Missing instance_url, oauth_token and/or charset parameters.\n");
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
    char scrolled;
    reply_to = api_get_status(argv[4], 0);

    print_status(reply_to, 0, &scrolled);

    if (wherey() > scrh - COMPOSE_HEIGHT) {
      clrzone(0, scrh - COMPOSE_HEIGHT, scrw - LEFT_COL_WIDTH - 2, scrh - 1);
      gotoxy(0, scrh - COMPOSE_HEIGHT);
    }
    chline(scrw - LEFT_COL_WIDTH - 1);
    dputs("Your reply:\r\n");
  }

  compose_toot();
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
