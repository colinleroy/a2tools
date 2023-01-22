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
#include "math.h"

#define BUF_SIZE 255

#define LEFT_COL_WIDTH 19

static unsigned char scrw, scrh;

static char *my_public_name = NULL;
static char *my_handle = NULL;

#define SHOW_HOME_TIMELINE 0
#define SHOW_FULL_STATUS   1
#define QUIT               2
static char cur_action;


static void print_timeline(char *tlid);

static void print_free_ram(void) {
#ifdef __CC65__
  gotoxy(0, 23);
  cprintf("%zuB free     ", _heapmemavail());
#endif
}

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
  
  #define BTM 19
  clrzone(0, BTM, LEFT_COL_WIDTH, BTM + 4);
  gotoxy(0,BTM);
  if (cur_action == SHOW_HOME_TIMELINE) {
    cputs("view toot: V/Enter \r\n");
    cputs("scroll   : Up/down \r\n");
    cputs("exit     : Escape  \r\n");
  } else if (cur_action == SHOW_FULL_STATUS) {
    cputs("Timeline : Escape \r\n");
  }
  print_free_ram();
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
  if (wherey() == scrh - 1) {
    return -1;
  }

  return 0;
}

static char **ids = NULL;
static char first_displayed_post = 0;
static status **displayed_posts = NULL;
static char last_displayed_post = 0;
static char n_posts = 0;
static signed char *post_height = NULL;
char newer_posts = 0;

#define N_STATUS_TO_LOAD 6
#define LOADING_TOOT_MSG "Loading toot..."

static void load_next_posts(char *tlid) {
  char *last_id;
  char shift = N_STATUS_TO_LOAD / 2;
  int loaded, i;

  dputs(LOADING_TOOT_MSG);
  for (i = 0; i < shift; i++) {
    status_free(displayed_posts[i]);
    free(ids[i]);
  }

  for (i = shift; i < n_posts; i++) {
    //printf("shifting %d to %d\n", i, i-shift);
    displayed_posts[i - shift] = displayed_posts[i];
    post_height[i - shift] = post_height[i];
    ids[i - shift] = ids[i];

    displayed_posts[i] = NULL;
    post_height[i] = -1;
    ids[i] = NULL;
  }
  last_id = ids[i - shift - 1];

  loaded = api_get_timeline_posts(tlid, N_STATUS_TO_LOAD / 2, NULL, last_id, ids + shift);
  n_posts += ((N_STATUS_TO_LOAD / 2) - loaded);

  first_displayed_post -= shift;
}

static void load_prev_posts(char *tlid) {
  char c;
  int i;
  set_hscrollwindow(LEFT_COL_WIDTH + 1, scrw - LEFT_COL_WIDTH - 1);

  scrolldn();
  scrolldn();
  
  gotoxy(0, 0);
  dputs("All caught up! Maybe reload? (y/N) ");
  gotoxy(0,1);
  chline(scrw - LEFT_COL_WIDTH - 1);

  c = cgetc();
  
  if (c == 'Y' || c == 'y') {
    for (i = 0; i < n_posts; i++) {
      free(ids[i]);
      status_free(displayed_posts[i]);
    }
    free(ids);
    free(displayed_posts);
    free(post_height);
    ids = NULL;
    displayed_posts = NULL;
    post_height = NULL;
    clrscr();
    return;
  }

  scrollup();
  scrollup();

  set_hscrollwindow(0, scrw);
  print_timeline(tlid);
}

static void print_timeline(char *tlid) {
  int i;
  char bottom = 0;
  
  if (ids == NULL) {
    ids = malloc(N_STATUS_TO_LOAD * sizeof(char *));
    n_posts = api_get_timeline_posts(tlid, N_STATUS_TO_LOAD, NULL, NULL, ids);
    displayed_posts = malloc(n_posts * sizeof(status *));
    memset(displayed_posts, 0, n_posts * sizeof(status *));
    post_height = malloc(n_posts);
    memset(post_height, -1, n_posts);
  }

  /* set scrollwindow */
  set_hscrollwindow(LEFT_COL_WIDTH + 1, scrw - LEFT_COL_WIDTH - 1);
update:
  gotoxy(0, 0);
  for (i = first_displayed_post; i < n_posts; i++) {
    if (!bottom) {
      status *disp;

      if (displayed_posts[i] != NULL)
        disp = displayed_posts[i];
      else {
#ifdef __CC65__
        cputs(LOADING_TOOT_MSG);
        gotox(0);
#endif
        disp = api_get_status(ids[i], 0);
#ifdef __CC65__
        cputs("                ");
        gotox(0);
#endif
      }
      displayed_posts[i] = disp;

      if (bottom == 0) {
        bottom = print_status(disp);
        //printf("showing %s\n", disp->id);
        if (!bottom) {
          post_height[i] = wherey() - disp->displayed_at;
        }
        last_displayed_post = i;
        if (bottom && i < n_posts - 1) {
          /* load the next one if needed */
          if (displayed_posts[i + 1] == NULL) {
            displayed_posts[i + 1] = api_get_status(ids[i + 1], 0);
          }
        }
      }
    }
  }
  /* Load one up for fast scroll */
  if (first_displayed_post > 0 && displayed_posts[first_displayed_post - 1] == NULL) {
    displayed_posts[first_displayed_post - 1] = api_get_status(ids[first_displayed_post - 1], 0);
  }

  if (bottom == 0 && i == n_posts) {
    load_next_posts(tlid);
    goto update;
  }

  set_hscrollwindow(0, scrw);
  print_free_ram();
}

static void shift_posts_down(void) {
  char i;
  char scroll_val = post_height[first_displayed_post];
  first_displayed_post++;
  /* Remove posts scrolled up, keeping just one for fast scroll */
  if (first_displayed_post > 0) {
    for (i = 0; i < first_displayed_post - 1 && i < last_displayed_post - 1; i++) {
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
  print_free_ram();
}

static void shift_posts_up(void) {
  int i;
  char scrollval = post_height[first_displayed_post - 1];
  first_displayed_post--;
  /* Remove posts scrolled down, keeping just one for fast scroll */
  if (last_displayed_post > 0) {
    for (i = last_displayed_post + 1; i < n_posts; i++) {
      if (displayed_posts[i]) {
        status_free(displayed_posts[i]);
        displayed_posts[i] = NULL;
      }
    }
  }
  set_hscrollwindow(LEFT_COL_WIDTH + 1, scrw - LEFT_COL_WIDTH - 1);
  for (i = 0; scrollval > 0 && i < scrollval; i++) {
    scrolldn();
  }
  set_hscrollwindow(0, scrw);
  print_free_ram();
}

static void show_timeline(char *tlid) {
  char c;
  print_header();
  while (1) {
    print_timeline(tlid);

    c = cgetc();
    switch(c) {
      case CH_ENTER:
      case 'v':
      case 'V':
        cur_action = SHOW_FULL_STATUS;
        return;
      case CH_ESC:
        cur_action = QUIT;
        return;
      case CH_CURS_DOWN:
        shift_posts_down();
        break;
      case CH_CURS_UP:
        if (first_displayed_post > 0)
          shift_posts_up();
        else {
          load_prev_posts(HOME_TIMELINE);
        }
        break;
    }
  }
}

static void show_full_status(char *status_id) {
  char c;
  status *s;

  print_header();
  
  s = api_get_status(status_id, 1);

  print_free_ram();
  set_hscrollwindow(LEFT_COL_WIDTH + 1, scrw - LEFT_COL_WIDTH - 1);

  /* cleanup beneath
   * We can assume we're first_displayed_post 
   */
  set_scrollwindow(post_height[first_displayed_post] - 1, scrh);
  clrscr();
  set_scrollwindow(0, scrh);

  while (1) {
    gotoxy(0, 0);
    print_status(s);

    c = cgetc();
    switch(c) {
      case CH_ESC:
        cur_action = SHOW_HOME_TIMELINE;
        goto out;
    }
  }
out:
  /* one less to clear end of post and ... */
  set_scrollwindow(post_height[first_displayed_post] - 2, scrh);
  clrscr();
  set_scrollwindow(0, scrh);
  
  set_hscrollwindow(0, scrw);
  status_free(s);
  print_free_ram();
}

void cli(void) {
  cur_action = SHOW_HOME_TIMELINE;

  screensize(&scrw, &scrh);

  clrscr();
  while (cur_action != QUIT) {
    switch(cur_action) {
      case SHOW_HOME_TIMELINE:
        show_timeline(HOME_TIMELINE);
        break;
      case SHOW_FULL_STATUS:
        show_full_status(ids[first_displayed_post]);
        break;
    }
  }
}

char *instance_url = NULL;
char *client_id = NULL;
char *client_secret = NULL;
char *login = NULL;
char *password = NULL;
char *oauth_code = NULL;
char *oauth_token = NULL;

int main(int argc, char **argv) {
  if (argc < 3) {
    printf("Missing instance_url and/or oauth_token parameters.\n");
  }

  videomode(VIDEOMODE_80COL);
  screensize(&scrw, &scrh);

#ifdef __CC65__
  cprintf("Available memory: %zu/%zu bytes\r\n",
          _heapmaxavail(), _heapmemavail());
  cgetc();
#endif

  instance_url = argv[1];
  oauth_token = argv[2];
  cli();
  videomode(VIDEOMODE_40COL);
  exit(0);
}
