#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#ifdef __APPLE2__
#include <apple2enh.h>
#endif
#include "surl.h"
#include "extended_conio.h"
#include "extended_string.h"
#include "dputs.h"
#include "dputc.h"
#include "scroll.h"
#include "header.h"
#include "cli.h"
#include "compose_header.h"
#include "compose.h"
#include "list.h"
#include "math.h"
#include "dgets.h"

char *instance_url = NULL;
char *oauth_token = NULL;
unsigned char scrw, scrh;

#define COMPOSE_HEIGHT 15
#define COMPOSE_FIELD_HEIGHT 9

static char compose_audience = COMPOSE_MENTION;
static char cancelled = 0;

static void update_compose_audience(void) {
  gotoxy(0, COMPOSE_FIELD_HEIGHT + 1);
  cprintf("Audience: (%c) Public  (%c) Unlisted  (%c) Private  (%c) Mention",
        compose_audience == COMPOSE_PUBLIC ? '*':' ',
        compose_audience == COMPOSE_UNLISTED ? '*':' ',
        compose_audience == COMPOSE_PRIVATE ? '*':' ',
        compose_audience == COMPOSE_MENTION ? '*':' ');
  gotoxy(0, COMPOSE_FIELD_HEIGHT + 3);
  cputs(translit_charset);
  if(!strcmp(translit_charset, "ISO646-FR1")) {
    cputs(": Use ] to mention, and # for hashtags.\r\n");
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
  set_scrollwindow(1, COMPOSE_FIELD_HEIGHT);
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
  char i, *text;

  text = NULL;

  set_hscrollwindow(LEFT_COL_WIDTH + 1, scrw - LEFT_COL_WIDTH - 1);

  for (i = 0; i < COMPOSE_HEIGHT; i++) {
    scrolldn();
  }
  
  gotoxy(0, 0);
  chline(scrw - LEFT_COL_WIDTH - 1);
  gotoxy(0, COMPOSE_FIELD_HEIGHT);
  chline(scrw - LEFT_COL_WIDTH - 1);

  update_compose_audience();
  chline(scrw - LEFT_COL_WIDTH - 1);

  set_scrollwindow(1, COMPOSE_FIELD_HEIGHT);

  gotoxy(0, 0);
  text = handle_compose_input();

  set_scrollwindow(0, scrh);

  for (i = 0; i < COMPOSE_HEIGHT; i++) {
    scrollup();
  }

  set_hscrollwindow(0, scrw);
  if (text && !cancelled) {
    api_send_toot(text, NULL, compose_audience);
  }

  free(text);
}


int main(int argc, char **argv) {
  char *params;
  if (argc < 4) {
    printf("Missing instance_url, oauth_token and/or charset parameters.\n");
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

  compose_print_header(NULL);
  compose_toot();

  params = malloc(127);
  snprintf(params, 127, "%s %s %s", instance_url, oauth_token, translit_charset);
#ifdef __CC65__
  exec("mastocli", params);
  exit(0);
#else
  printf("exec(mastocli %s)\n",params);
  exit(0);
#endif
}
