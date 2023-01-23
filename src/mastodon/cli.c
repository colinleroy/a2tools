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
#define NAVIGATE           8
#define BACK               9
#define QUIT               10
static char cur_action;


static void print_list(list *l);

static void print_free_ram(void) {
#ifdef __CC65__
  gotoxy(0, 23);
  cprintf("%zuB free     ", _heapmemavail());
#endif
}

static void print_header(char level) {
  int r = 0;

  if (my_public_name == NULL) {
    r = api_get_profile(&my_public_name, &my_handle);
  }
  if (r == 0) {
    if (strlen(my_public_name) > LEFT_COL_WIDTH)
      my_public_name[LEFT_COL_WIDTH] = '\0';

    if (strlen(my_handle) > LEFT_COL_WIDTH)
      my_handle[LEFT_COL_WIDTH] = '\0';

    cputsxy(0, 0, my_public_name);
    gotoxy(0, 1);
    cprintf("@%s\r\n", my_handle);
  }

  #define BTM 19
  clrzone(0, BTM, LEFT_COL_WIDTH, BTM + 4);
  gotoxy(0,BTM);
  if (level == 0) {
    cputs("View toot: V/Enter \r\n");
    cputs("Scroll   : Up/down \r\n");
    cputs("Exit     : Escape  \r\n");
  } else {
    cputs("View toot: V/Enter \r\n");
    cputs("Scroll   : Up/down \r\n");
    cputs("Back     : Escape \r\n");
  }
  print_free_ram();
  cvlinexy(LEFT_COL_WIDTH, 0, scrh);
  
}

static int print_status(status *s, char full, char *scrolled) {
  char *w;
  
  *scrolled = 0;
  s->displayed_at = wherey();
  /* reblog header */
  if (s->reblog) {
    if (s->account)
      dputs(s->account->display_name);
    dputs(" shared");
    if (wherey() == scrh - 1) {
      return -1;
    }
    dputs("\r\n");
    s = s->reblog;
  }
  
  if (s->account) {
    /* Display name + date */
    dputs(s->account->display_name); 
    dputs(" - ");
    dputs(s->created_at); 
    if (wherey() == scrh - 1) {
      return -1;
    }
    dputs("\r\n");

    /* username (30 chars max)*/
    dputc('@');
    dputs(s->account->username);
    
    /* stats */
    /* 255 replies, 2 images */
    gotox(38);
    cprintf("%3d replies, %1d images",
          s->n_replies, s->n_images);

    if (wherey() == scrh - 1) {
      return -1;
    }
    dputs("\r\n");
  }
  /* Content */
  w = s->content;
  while (*w) {
    if (full && wherey() == scrh - 2) {
      char i;
      cputsxy(0, scrh-1, "Hit a key to continue.");
      cgetc();
      cputsxy(0, scrh-1, "                      ");
      for (i = 0; i < 10; i++) {
        scrollup();
      }
      gotoxy(0, scrh-12);
      *scrolled = 1;
    }
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

  if (full) {
    /* stats */
    /* 255 replies, 255 favs, 255 shares, 2 images */
      cprintf("%3d replies, %3d shares, %3d favs, %1d images",
            s->n_replies, s->n_reblogs, s->n_favourites, s->n_images);
      dputs("\r\n");
      if (wherey() == scrh - 1) {
        return -1;
      }
  }

  chline(scrw - LEFT_COL_WIDTH - 1);
  if (wherey() == scrh - 1) {
    return -1;
  }

  return 0;
}

#define N_STATUS_TO_LOAD 6
#define LOADING_TOOT_MSG "Loading toot..."

static char load_next_posts(list *l) {
  char *last_id;
  char **new_ids;
  char loaded, i;

  if (l->eof) {
    return 0;
  }
  new_ids = malloc((N_STATUS_TO_LOAD / 2) * sizeof(char *));

  dputs(LOADING_TOOT_MSG);

  last_id = l->n_posts > 0 ? l->ids[l->n_posts - 1] : NULL;
  loaded = 0;

  switch (l->kind) {
    case L_HOME_TIMELINE:
      loaded = api_get_timeline_posts(HOME_TIMELINE, N_STATUS_TO_LOAD / 2, NULL, last_id, new_ids);
      break;
    case L_FULL_STATUS:
      /* ... */
      break;
  }

  if (loaded > 0) {
    for (i = 0; i < loaded; i++) {
      status_free(l->displayed_posts[i]);
      free(l->ids[i]);
    }

    for (i = loaded; i < l->n_posts; i++) {
      l->displayed_posts[i - loaded] = l->displayed_posts[i];
      l->post_height[i - loaded] = l->post_height[i];
      l->ids[i - loaded] = l->ids[i];

      l->displayed_posts[i] = NULL;
      l->post_height[i] = -1;
      l->ids[i] = new_ids[i - loaded];
    }

    l->n_posts += ((N_STATUS_TO_LOAD / 2) - loaded);
    l->first_displayed_post -= loaded;
    if (l->first_displayed_post < 0) {
      l->first_displayed_post = 0;
    }
  } else {
    gotox(0);
    dputs("Nothing more to load!");
    l->eof = 1;
  }
  free(new_ids);
  return loaded;
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
    clrscr();
    set_hscrollwindow(0, scrw);
    return 1;
  }

  scrollup();
  scrollup();

  set_hscrollwindow(0, scrw);
  print_list(l);
  return 0;
}

static list *build_list(status *root, char kind) {
  list *l;
  char i;

  l = malloc(sizeof(list));

  if (root) {
    l->root = strdup(root->id);
  } else {
    l->root = NULL;
  }

  l->kind = kind;
  l->ids = malloc(N_STATUS_TO_LOAD * sizeof(char *));
  l->displayed_posts = malloc(N_STATUS_TO_LOAD * sizeof(status *));
  l->post_height = malloc(N_STATUS_TO_LOAD);
  switch (kind) {
    case L_HOME_TIMELINE:
      l->n_posts = api_get_timeline_posts(HOME_TIMELINE, N_STATUS_TO_LOAD, NULL, NULL, l->ids);
      break;
    case L_FULL_STATUS:
      l->n_posts = api_get_status_and_replies(N_STATUS_TO_LOAD, root, l->ids);
      break;
  }
  memset(l->displayed_posts, 0, l->n_posts * sizeof(status *));
  memset(l->post_height, -1, l->n_posts);
  l->first_displayed_post = 0;
  if (l->root) {
    for (i = 0; i < l->n_posts; i++) {
      if(!strcmp(l->ids[i], l->root)) {
          l->first_displayed_post = i;
          break;
      }
    }
  }
  l->last_displayed_post = 0;
  l->eof = 0;
  l->scrolled = 0;

  return l;
}

static void free_list(list *l) {
  char i;
  for (i = 0; i < l->n_posts; i++) {
    free(l->ids[i]);
    status_free(l->displayed_posts[i]);
  }
  free(l->root);
  free(l->ids);
  free(l->displayed_posts);
  free(l->post_height);
  free(l);
}

static void compact_list(list *l) {
  char i;
  for (i = 0; i < l->n_posts; i++) {
    status_free(l->displayed_posts[i]);
    l->displayed_posts[i] = NULL;
  }
}

static void uncompact_list(list *l, char full) {
  l->displayed_posts[l->first_displayed_post] = 
    api_get_status(l->ids[l->first_displayed_post], full);
}

static void print_list(list *l) {
  char i, full;
  char bottom = 0;

  /* set scrollwindow */
  set_hscrollwindow(LEFT_COL_WIDTH + 1, scrw - LEFT_COL_WIDTH - 1);
update:
  gotoxy(0, 0);
  if (l->scrolled) {
    clrscr();
  }
  l->scrolled = 0;
  for (i = l->first_displayed_post; i < l->n_posts; i++) {
    if (!bottom) {
      status *disp;

      if (l->displayed_posts[i] != NULL)
        disp = l->displayed_posts[i];
      else {
#ifdef __CC65__
        if (i > l->first_displayed_post) 
          cputs(LOADING_TOOT_MSG);
        gotox(0);
#endif
        full = (l->root && !strcmp(l->root, l->ids[i]));
        disp = api_get_status(l->ids[i], full);
#ifdef __CC65__
        if (i > l->first_displayed_post)
          cputs("                ");
        gotox(0);
#endif
      }
      l->displayed_posts[i] = disp;

      if (disp != NULL && bottom == 0) {
        char scrolled = 0;
        full = (l->root && !strcmp(l->root, l->ids[i]));
        bottom = print_status(disp, full, &scrolled);
        //printf("showing %s\n", disp->id);
        if (!bottom) {
          l->post_height[i] = wherey() - disp->displayed_at;
        }
        if (scrolled) {
          l->post_height[i] = scrh;
          l->scrolled = 1;
        }
        l->last_displayed_post = i;
        if (bottom && i < l->n_posts - 1) {
          /* load the next one if needed */
          if (l->displayed_posts[i + 1] == NULL) {
            full = (l->root && !strcmp(l->root, l->ids[i + 1]));
            l->displayed_posts[i + 1] = api_get_status(l->ids[i + 1], full);
          }
        }
      }
    }
  }
  /* Load one up for fast scroll */
  if (l->first_displayed_post > 0 && l->displayed_posts[l->first_displayed_post - 1] == NULL) {
    full = (l->root && !strcmp(l->root, l->ids[l->first_displayed_post - 1]));
    l->displayed_posts[l->first_displayed_post - 1] = api_get_status(l->ids[l->first_displayed_post - 1], full);
  }

  if (bottom == 0 && i == l->n_posts) {
    if (load_next_posts(l) > 0)
      goto update;
  }

  set_hscrollwindow(0, scrw);
  print_free_ram();
}

static void shift_posts_down(list *l) {
  char i;
  char scroll_val;

  if (l->eof)
    return;
    
  scroll_val = l->post_height[l->first_displayed_post];
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
  signed char scrollval;
  
  l->eof = 0;
  scrollval = l->post_height[l->first_displayed_post - 1];
  if (scrollval < 0) {
    scrollval = scrh;
  }
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

/* returns 1 to reload */
static int show_list(list *l) {
  char c;
  
  while (1) {
    print_list(l);

    c = cgetc();
    switch(c) {
      case CH_ENTER:
      case 'v':
      case 'V':
        cur_action = SHOW_FULL_STATUS;
        return 0;
      case CH_ESC:
        cur_action = BACK;
        return 0;
      case CH_CURS_DOWN:
        shift_posts_down(l);
        break;
      case CH_CURS_UP:
        if (l->first_displayed_post > 0)
          shift_posts_up(l);
        else {
          if (load_prev_posts(l)) {
            return 1;
          }
        }
        break;
    }
  }
  return 0;
}

void cli(void) {
  char cur_list = 0;
  list **l;

  screensize(&scrw, &scrh);
  clrscr();

  cur_action = SHOW_HOME_TIMELINE;
  l = malloc((cur_list + 1) * sizeof(list *));

  while (cur_action != QUIT) {
    switch(cur_action) {
      case SHOW_HOME_TIMELINE:
        l[cur_list] = build_list(NULL, L_HOME_TIMELINE);
        cur_action = NAVIGATE;
        break;
      case SHOW_FULL_STATUS:
        cur_list++;
        l = realloc(l, (cur_list + 1) * sizeof(list *));
        l[cur_list] = build_list(l[cur_list - 1]->displayed_posts[l[cur_list - 1]->first_displayed_post],
                   L_FULL_STATUS);
        clrzone(LEFT_COL_WIDTH + 1, l[cur_list - 1]->post_height[l[cur_list - 1]->first_displayed_post] - 2,
                scrw - 1, scrh - 1);
        compact_list(l[cur_list - 1]);
        cur_action = NAVIGATE;
        break;
      case NAVIGATE:
        print_header(cur_list);
        if (show_list(l[cur_list])) {
          if (cur_list > 0) {
            free_list(l[cur_list]);
            cur_list--;
            uncompact_list(l[cur_list], cur_list > 0);
            cur_action = SHOW_FULL_STATUS;
          } else {
            free_list(l[cur_list]);
            cur_action = SHOW_HOME_TIMELINE;
          }
          clrscr();
        }
        break;
      case BACK:
        if (cur_list > 0) {
          clrzone(LEFT_COL_WIDTH + 1, 3, scrw - 1, scrh - 1);
          free_list(l[cur_list]);
          cur_list--;
          cur_action = NAVIGATE;
        } else {
          cur_action = QUIT;
        }
        break;
      case QUIT:
        goto out;
    }
  }
out:
  free_list(l[cur_list]);
  l[cur_list] = NULL;
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
