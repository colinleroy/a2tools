#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "surl.h"
#include "extended_conio.h"
#include "extended_string.h"
#include "dputs.h"
#include "dputc.h"
#include "api.h"

#define BUF_SIZE 255

#define LEFT_COL_WIDTH 19

static unsigned char scrw, scrh;

static char *my_public_name = NULL;
static char *my_handle = NULL;

static void print_header(void) {
  //print_logo();

  if (my_public_name == NULL) {
    api_get_profile(&my_public_name, &my_handle);
  }
  if (strlen(my_public_name) > LEFT_COL_WIDTH)
    my_public_name[LEFT_COL_WIDTH] = '\0';

  if (strlen(my_handle) > LEFT_COL_WIDTH)
    my_handle[LEFT_COL_WIDTH] = '\0';

  cputsxy(0, 0, my_public_name);
  gotoxy(0, 1);
  cprintf("@%s\r\n", my_handle);
  cvlinexy(LEFT_COL_WIDTH, 0, scrh);
}

static int print_status(status *s) {
  char *w;
  /* reblog header */
  if (s->reblog) {
    dputs(s->account->display_name);
    dputs(" shared\r\n");
    s = s->reblog;
  }
  if (wherey() == scrh - 1) {
    return -1;
  }
  
  /* Display name + date */
  dputs(s->account->display_name); 
  dputs(" - ");
  dputs(s->created_at); 
  dputs("\r\n");
  if (wherey() == scrh - 1) {
    return -1;
  }

  /* username */
  dputc('@');
  dputs(s->account->username);
  dputs("\r\n");
  if (wherey() == scrh - 1) {
    return -1;
  }

  /* Content */
  w = s->content;
  while (*w) {
    if (*w == '\n') {
      if (wherey() == scrh - 1) {
        return -1;
      }
      dputs("\r\n");
    } else {
      dputc(*w);
    }
    ++w;
  }
  dputs("\r\n");
  if (wherey() == scrh - 1) {
    return -1;
  }

  dputs("\r\n");
  if (wherey() == scrh - 1) {
    return -1;
  }

  return 0;
}

static void print_timeline(char *tlid) {
  status **posts;
  int n_posts, i;
  char bottom = 0;
  n_posts = api_get_timeline_posts(tlid, &posts);

  /* set scrollwindow */
  set_hscrollwindow(LEFT_COL_WIDTH + 1, scrw - LEFT_COL_WIDTH - 1);
  gotoxy(0, 0);
  for (i = 0; i < n_posts; i++) {
    status *disp = posts[i];

    if (bottom == 0)
      bottom = print_status(posts[i]);

    status_free(posts[i]);
  }
  set_hscrollwindow(0, scrw);
  free(posts);
}

void cli(void) {
  screensize(&scrw, &scrh);

  clrscr();

  print_header();
  print_timeline(HOME_TIMELINE);
  cgetc();
}
