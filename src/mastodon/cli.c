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
#include "list.h"
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


static void print_list(list *l);

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

#define N_STATUS_TO_LOAD 6
#define LOADING_TOOT_MSG "Loading toot..."

static void load_next_posts(list *l) {
  char *last_id;
  char shift = N_STATUS_TO_LOAD / 2;
  char loaded, i;

  dputs(LOADING_TOOT_MSG);
  for (i = 0; i < shift; i++) {
    status_free(l->displayed_posts[i]);
    free(l->ids[i]);
  }

  for (i = shift; i < l->n_posts; i++) {
    l->displayed_posts[i - shift] = l->displayed_posts[i];
    l->post_height[i - shift] = l->post_height[i];
    l->ids[i - shift] = l->ids[i];

    l->displayed_posts[i] = NULL;
    l->post_height[i] = -1;
    l->ids[i] = NULL;
  }
  last_id = l->ids[i - shift - 1];

  switch (l->kind) {
    case L_HOME_TIMELINE:
      loaded = api_get_timeline_posts(HOME_TIMELINE, N_STATUS_TO_LOAD / 2, NULL, last_id, l->ids + shift);
      break;
    case L_FULL_STATUS:
      /* ... */
      break;
  }

  l->n_posts += ((N_STATUS_TO_LOAD / 2) - loaded);

  l->first_displayed_post -= shift;
}

static int load_prev_posts(list *l) {
  char c;
  set_hscrollwindow(LEFT_COL_WIDTH + 1, scrw - LEFT_COL_WIDTH - 1);

  scrolldn();
  scrolldn();
  
  gotoxy(0, 0);
  dputs("All caught up! Maybe reload? (y/N) ");
  gotoxy(0,1);
  chline(scrw - LEFT_COL_WIDTH - 1);

  c = cgetc();
  
  if (c == 'Y' || c == 'y') {
    set_hscrollwindow(0, scrw);
    return 1;
  }

  scrollup();
  scrollup();

  set_hscrollwindow(0, scrw);
  print_list(l);
  return 0;
}

static list *build_list(char kind) {
  list *l = malloc(sizeof(list));
  l->kind = kind;
  l->ids = malloc(N_STATUS_TO_LOAD * sizeof(char *));
  l->displayed_posts = malloc(N_STATUS_TO_LOAD * sizeof(status *));
  l->post_height = malloc(N_STATUS_TO_LOAD);
  switch (kind) {
    case L_HOME_TIMELINE:
      l->n_posts = api_get_timeline_posts(HOME_TIMELINE, N_STATUS_TO_LOAD, NULL, NULL, l->ids);
      break;
    case L_FULL_STATUS:
      /* ... */
      break;
  }
  memset(l->displayed_posts, 0, l->n_posts * sizeof(status *));
  memset(l->post_height, -1, l->n_posts);
  l->first_displayed_post = 0;
  l->last_displayed_post = 0;

  return l;
}

static void free_list(list *l) {
  char i;
  for (i = 0; i < l->n_posts; i++) {
    free(l->ids[i]);
    status_free(l->displayed_posts[i]);
  }
  free(l->ids);
  free(l->displayed_posts);
  free(l->post_height);
  free(l);
}

static void print_list(list *l) {
  char i;
  char bottom = 0;

  /* set scrollwindow */
  set_hscrollwindow(LEFT_COL_WIDTH + 1, scrw - LEFT_COL_WIDTH - 1);
update:
  gotoxy(0, 0);
  for (i = l->first_displayed_post; i < l->n_posts; i++) {
    if (!bottom) {
      status *disp;

      if (l->displayed_posts[i] != NULL)
        disp = l->displayed_posts[i];
      else {
#ifdef __CC65__
        cputs(LOADING_TOOT_MSG);
        gotox(0);
#endif
        disp = api_get_status(l->ids[i], 0);
#ifdef __CC65__
        cputs("                ");
        gotox(0);
#endif
      }
      l->displayed_posts[i] = disp;

      if (bottom == 0) {
        bottom = print_status(disp);
        //printf("showing %s\n", disp->id);
        if (!bottom) {
          l->post_height[i] = wherey() - disp->displayed_at;
        }
        l->last_displayed_post = i;
        if (bottom && i < l->n_posts - 1) {
          /* load the next one if needed */
          if (l->displayed_posts[i + 1] == NULL) {
            l->displayed_posts[i + 1] = api_get_status(l->ids[i + 1], 0);
          }
        }
      }
    }
  }
  /* Load one up for fast scroll */
  if (l->first_displayed_post > 0 && l->displayed_posts[l->first_displayed_post - 1] == NULL) {
    l->displayed_posts[l->first_displayed_post - 1] = api_get_status(l->ids[l->first_displayed_post - 1], 0);
  }

  if (bottom == 0 && i == l->n_posts) {
    load_next_posts(l);
    goto update;
  }

  set_hscrollwindow(0, scrw);
  print_free_ram();
}

static void shift_posts_down(list *l) {
  char i;
  char scroll_val = l->post_height[l->first_displayed_post];
  l->first_displayed_post++;
  /* Remove posts scrolled up, keeping just one for fast scroll */
  if (l->first_displayed_post > 0) {
    for (i = 0; i < l->first_displayed_post - 1 && i < l->last_displayed_post - 1; i++) {
      if (l->displayed_posts[i]) {
        status_free(l->displayed_posts[i]);
        l->displayed_posts[i] = NULL;
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

static void shift_posts_up(list *l) {
  int i;
  char scrollval = l->post_height[l->first_displayed_post - 1];
  l->first_displayed_post--;
  /* Remove posts scrolled down, keeping just one for fast scroll */
  if (l->last_displayed_post > 0) {
    for (i = l->last_displayed_post + 1; i < l->n_posts; i++) {
      if (l->displayed_posts[i]) {
        status_free(l->displayed_posts[i]);
        l->displayed_posts[i] = NULL;
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

static void show_timeline(list *l) {
  char c;
  print_header();
  
  while (1) {
    print_list(l);

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
        shift_posts_down(l);
        break;
      case CH_CURS_UP:
        if (l->first_displayed_post > 0)
          shift_posts_up(l);
        else {
          if (load_prev_posts(l)) {
            return;
          }
        }
        break;
    }
  }
}

static void show_full_status(list *l, char *status_id) {
  char c;
  status *s;

  print_header();
  
  s = api_get_status(status_id, 1);

  print_free_ram();
  set_hscrollwindow(LEFT_COL_WIDTH + 1, scrw - LEFT_COL_WIDTH - 1);

  /* cleanup beneath
   * We can assume we're first_displayed_post 
   */
  set_scrollwindow(l->post_height[l->first_displayed_post] - 1, scrh);
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
  set_scrollwindow(l->post_height[l->first_displayed_post] - 2, scrh);
  clrscr();
  set_scrollwindow(0, scrh);
  
  set_hscrollwindow(0, scrw);
  status_free(s);
  print_free_ram();
}

void cli(void) {
  list *l;
  cur_action = SHOW_HOME_TIMELINE;

  screensize(&scrw, &scrh);

  clrscr();
  while (cur_action != QUIT) {
    switch(cur_action) {
      case SHOW_HOME_TIMELINE:
        l = build_list(L_HOME_TIMELINE);
        show_timeline(l);
        break;
      case SHOW_FULL_STATUS:
        if (l)
          show_full_status(l, l->ids[l->first_displayed_post]);
        break;
    }
    free_list(l);
    l = NULL;
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
