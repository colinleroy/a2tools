#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "surl.h"
#include "extended_conio.h"
#include "extended_string.h"
#include "dputs.h"
#include "dputc.h"
#include "scroll.h"
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
  
  s->displayed_at = wherey();
  /* reblog header */
  if (s->reblog) {
    dputs(s->account->display_name);
    dputs(" shared");
    if (wherey() == scrh - 1) {
      return -1;
    }
    dputs("\r\n");
    s = s->reblog;
  }
  
  /* Display name + date */
  dputs(s->account->display_name); 
  dputs(" - ");
  dputs(s->created_at); 
  if (wherey() == scrh - 1) {
    return -1;
  }
  dputs("\r\n");

  /* username */
  dputc('@');
  dputs(s->account->username);
  if (wherey() == scrh - 1) {
    return -1;
  }
  dputs("\r\n");

  /* Content */
  w = s->content;
  while (*w) {
    if (*w == '\n') {
      if (wherey() == scrh - 1) {
        return -1;
      }
      dputs("\r\n");
    } else {
      if (wherey() == scrh - 1 && wherex() == scrw - LEFT_COL_WIDTH - 2) {
        return -1;
      }
      dputc(*w);
    }
    ++w;
  }
  if (wherey() == scrh - 1) {
    return -1;
  }
  dputs("\r\n");

  if (wherey() == scrh - 1) {
    return -1;
  }
  chline(scrw - LEFT_COL_WIDTH - 1);

  return 0;
}

static char first_displayed_post = 0;
static status **displayed_posts = NULL;
static char last_displayed_post = 0;
static char n_posts = 0;
static signed char *post_height = NULL;
static void print_timeline(char *tlid) {
  static char **ids = NULL;
  int i;
  char bottom = 0;
  
  if (ids == NULL) {
    n_posts = api_get_timeline_posts(tlid, &ids);
    if (displayed_posts) {
      for (i = 0; i < last_displayed_post; i++) {
        status_free(displayed_posts[i]);
      }
      free(displayed_posts);
      free(post_height);
    }
    displayed_posts = malloc(n_posts * sizeof(status *));
    memset(displayed_posts, 0, n_posts * sizeof(status *));
    post_height = malloc(n_posts);
    memset(post_height, -1, n_posts);
  }

  /* set scrollwindow */
  set_hscrollwindow(LEFT_COL_WIDTH + 1, scrw - LEFT_COL_WIDTH - 1);
  gotoxy(0, 0);
  for (i = first_displayed_post; i < n_posts; i++) {
    if (!bottom) {
      status *disp;

      if (displayed_posts[i] != NULL)
        disp = displayed_posts[i];
      else
        disp = api_get_status(ids[i]);

      displayed_posts[i] = disp;
      if (bottom == 0) {
        bottom = print_status(disp);
        if (!bottom) {
          post_height[i] = wherey() - disp->displayed_at;
        }
        last_displayed_post = i;
      }
    }
  }
  /* Need to free ids when refreshing */
  // free(ids[i]);
  // free(ids);

  set_hscrollwindow(0, scrw);
}

static void shift_posts_down(void) {
  char i;
  char scroll_val = post_height[first_displayed_post];
  first_displayed_post++;
  /* Remove posts scrolled up */
  if (first_displayed_post > 0) {
    for (i = 0; i < first_displayed_post && i < last_displayed_post; i++) {
      if (displayed_posts[i]) {
        status_free(displayed_posts[i]);
        displayed_posts[i] = NULL;
      }
    }
  }
  set_hscrollwindow(LEFT_COL_WIDTH + 1, scrw - LEFT_COL_WIDTH - 1);
  for (i = 0; i < scroll_val; i++) {
    scrollup();
  }
  set_hscrollwindow(0, scrw);
}

static void shift_posts_up(void) {
  int i;
  char scrollval = post_height[first_displayed_post - 1];
  first_displayed_post--;
  /* Remove posts scrolled down */
  if (last_displayed_post > 0) {
    for (i = last_displayed_post; i < n_posts; i++) {
      if (displayed_posts[i]) {
        status_free(displayed_posts[i]);
        displayed_posts[i] = NULL;
      }
    }
  }
  set_hscrollwindow(LEFT_COL_WIDTH + 1, scrw - LEFT_COL_WIDTH - 1);
  for (i = 0; i < scrollval; i++) {
    scrolldn();
  }
  set_hscrollwindow(0, scrw);
}

void cli(void) {
  char c;
  screensize(&scrw, &scrh);

  clrscr();

  print_header();
  while (1) {
    print_timeline(HOME_TIMELINE);

    c = cgetc();
    switch(c) {
      case CH_CURS_DOWN:
        shift_posts_down();
        break;
      case CH_CURS_UP:
        if (first_displayed_post > 0)
          shift_posts_up();
        else
          printf("%c", 0x07);
        break;
    }
  }
}
