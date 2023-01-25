#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
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

static account *my_account = NULL;

#define SHOW_HOME_TIMELINE 0
#define SHOW_FULL_STATUS   5
#define SHOW_ACCOUNT       6
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

static status *is_root_status_at_top(list *l) {
  status *root_status;
  char full;

  root_status = NULL;
  full = (l->root && l->ids[l->first_displayed_post] &&
         !strcmp(l->root, l->ids[l->first_displayed_post]));
  if (full) {
    root_status = l->displayed_posts[l->first_displayed_post];
    if (root_status && root_status->reblog) {
      root_status = root_status->reblog;
    }
  }
  return root_status;
}

static void print_header(list *l) {
  if (my_account == NULL) {
    my_account = api_get_profile(NULL);
  }
  if (my_account != NULL) {
    if (strlen(my_account->display_name) > LEFT_COL_WIDTH)
      my_account->display_name[LEFT_COL_WIDTH] = '\0';

    if (strlen(my_account->username) > LEFT_COL_WIDTH)
      my_account->username[LEFT_COL_WIDTH] = '\0';

    cputsxy(0, 0, my_account->display_name);
    gotoxy(0, 1);
    cprintf("@%s\r\n", my_account->username);
  }

  #define BTM 13
  clrzone(0, BTM, LEFT_COL_WIDTH, 23);
  gotoxy(0,BTM);

  cputs("Navigation:\r\n");
  cputs(" View toot: Enter\r\n");
  cputs(" Scroll   : Up/dn\r\n");
  if (!l->root) {
    cputs(" Exit     : Escape \r\n");
  } else {
    status *root_status;
    root_status = is_root_status_at_top(l);
    cputs(" Back     : Escape \r\n");
    
    if (root_status) {
      cputs("Toot: \r\n");
      if ((root_status->favorited_or_reblogged & FAVOURITED) != 0) {
        cputs(" Unfav.   : F      \r\n");
      } else {
        cputs(" Favourite: F      \r\n");
      }
      if ((root_status->favorited_or_reblogged & REBLOGGED) != 0) {
        cputs(" Unboost  : B      \r\n");
      } else {
        cputs(" Boost    : B      \r\n");
      }
      if (!strcmp(root_status->account->id, my_account->id)) {
        cputs(" Delete   : D      \r\n");
      }

      cputs("Author:\r\n");
      cputs(" Profile  : P      \r\n");
    }
  }
  print_free_ram();
  cvlinexy(LEFT_COL_WIDTH, 0, scrh);
  
}

#define CHECK_AND_CRLF() do { \
  if (wherey() == scrh - 1) { \
    return -1;                \
  }                           \
  dputs("\r\n");              \
} while (0)

static int print_buf(char *w, char allow_scroll, char *scrolled) {
  while (*w) {
    if (allow_scroll && wherey() == scrh - 2) {
      char i;
      cputsxy(0, scrh-1, "Hit a key to continue.");
      cgetc();
      cputsxy(0, scrh-1, "                      ");
      for (i = 0; i < 10; i++) {
        scrollup();
      }
      gotoxy(0, scrh - 12);
      *scrolled = 1;
    }
    if (*w == '\n') {
      CHECK_AND_CRLF();
    } else {
      if (wherey() == scrh - 1 && wherex() == scrw - LEFT_COL_WIDTH - 2) {
        return -1;
      }
      dputc(*w);
    }
    ++w;
  }
  return 0;
}

static int print_account(account *a, char *scrolled) {
  char y;
  *scrolled = 0;
  dputs(a->display_name);
  CHECK_AND_CRLF();
  dputc('@');
  dputs(a->username);
  CHECK_AND_CRLF();

  cprintf("%ld following, %ld followers", a->following_count, a->followers_count);
  CHECK_AND_CRLF();
  cprintf("Here since %s", a->created_at);
  CHECK_AND_CRLF();

  y = 0;
  if (api_relationship_get(a, RSHIP_FOLLOWING)) {
    gotoxy(32, y);
    dputs("             You follow them");
    CHECK_AND_CRLF();
  }
  if (api_relationship_get(a, RSHIP_FOLLOW_REQ)) {
    gotoxy(32, ++y);
    dputs("You requested to follow them");
    CHECK_AND_CRLF();
  }
  if (api_relationship_get(a, RSHIP_FOLLOWED_BY)) {
    gotoxy(32, ++y);
    dputs("             They follow you");
    CHECK_AND_CRLF();
  }

  if (wherey() < 4)
    gotoy(4);

  CHECK_AND_CRLF();
  if (print_buf(a->note, 1, scrolled) < 0) {
    return -1;
  }
  CHECK_AND_CRLF();
  chline(scrw - LEFT_COL_WIDTH - 1);
  if (wherey() == scrh - 1) {
    return -1;
  }
  return 0;
}

static int print_status(status *s, char full, char *scrolled) {
  char y;

  *scrolled = 0;
  s->displayed_at = wherey();
  /* reblog header */
  if (s->reblog) {
    dputs(s->account->display_name);
    dputs(" boosted");
    CHECK_AND_CRLF();
    s = s->reblog;
  }
  
  /* Display name + date */
  dputs(s->account->display_name); 
  dputs(" - ");
  dputs(s->created_at); 
  CHECK_AND_CRLF();

  /* username (30 chars max)*/
  dputc('@');
  dputs(s->account->username);
  
  /* stats */
  /* 255 replies, 2 images */
  gotox(39);
  y = wherey();
  cprintf("%3d replies, %1d images",
        s->n_replies, s->n_images);
  gotoxy(79, y);
  CHECK_AND_CRLF();

  /* Content */
  if (print_buf(s->content, (full && s->displayed_at == 0), scrolled) < 0)
    return -1;

  CHECK_AND_CRLF();
  if (wherey() == scrh - 1) {
    return -1;
  }

  if (full) {
    /* stats */
      CHECK_AND_CRLF();
      cprintf("%d replies, %s%d boosts, %s%d favs, %1d images      ",
            s->n_replies,
            (s->favorited_or_reblogged & REBLOGGED) ? "*":"", s->n_reblogs,
            (s->favorited_or_reblogged & FAVOURITED) ? "*":"", s->n_favourites,
            s->n_images);
      CHECK_AND_CRLF();
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

#define N_STATUS_TO_LOAD 10
#define LOADING_TOOT_MSG "Loading toot..."

static char load_next_posts(list *l) {
  char *last_id;
  char **new_ids;
  char loaded, i;
  char to_load;

  if (l->eof) {
    return 0;
  }

  to_load = min(N_STATUS_TO_LOAD / 2, l->first_displayed_post);
  new_ids = malloc(to_load * sizeof(char *));

  dputs(LOADING_TOOT_MSG);

  last_id = l->n_posts > 0 ? l->ids[l->n_posts - 1] : NULL;
  loaded = 0;

  switch (l->kind) {
    case L_HOME_TIMELINE:
      loaded = api_get_posts(TIMELINE_ENDPOINT HOME_TIMELINE, to_load, last_id, new_ids);
      break;
    case L_FULL_STATUS:
      loaded = api_get_status_and_replies(to_load, l->root, l->leaf_root, last_id, new_ids);
      break;
    case L_ACCOUNT:
      loaded = api_get_account_posts(l->account, to_load, last_id, new_ids);
      break;
  }

  if (loaded > 0) {
    char offset;
    /* free first ones */
    for (i = 0; i < loaded; i++) {
      status_free(l->displayed_posts[i]);
      free(l->ids[i]);
    }

    /* move last ones to first ones */
    for (i = 0; i < l->n_posts - loaded ; i++) {
      offset = i + loaded;
      l->displayed_posts[i] = l->displayed_posts[offset];
      l->post_height[i] = l->post_height[offset];
      l->ids[i] = l->ids[offset];
    }
    
    /* Set new ones at end */
    for (i = l->n_posts - loaded; i < l->n_posts; i++) {
      offset = i - (l->n_posts - loaded);
      l->displayed_posts[i] = NULL;
      l->post_height[i] = -1;
      l->ids[i] = new_ids[offset];
    }

    l->first_displayed_post -= loaded;
    if (l->first_displayed_post < 0) {
      clrscr();
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

  gotoxy(LEFT_COL_WIDTH - 4, scrh - 1);
  cputs("...");

  l = malloc(sizeof(list));
  memset(l, 0, sizeof(list));

  if (kind != L_ACCOUNT) {
    if (root) {
      l->root = strdup(root->id);
      l->leaf_root = strdup(root->reblog ? root->reblog->id : root->id);
    }
    l->first_displayed_post = 0;
  } else {
    l->account = api_get_full_account(root->reblog ? root->reblog->account : root->account);
    l->first_displayed_post = -1;
  }

  l->kind = kind;
  l->ids = malloc(N_STATUS_TO_LOAD * sizeof(char *));
  l->displayed_posts = malloc(N_STATUS_TO_LOAD * sizeof(status *));
  l->post_height = malloc(N_STATUS_TO_LOAD);

  switch (kind) {
    case L_HOME_TIMELINE:
      l->n_posts = api_get_posts(TIMELINE_ENDPOINT HOME_TIMELINE, N_STATUS_TO_LOAD, NULL, l->ids);
      memset(l->displayed_posts, 0, l->n_posts * sizeof(status *));
      break;
    case L_FULL_STATUS:
      l->n_posts = api_get_status_and_replies(N_STATUS_TO_LOAD, l->root, l->leaf_root, NULL, l->ids);
      memset(l->displayed_posts, 0, l->n_posts * sizeof(status *));
      break;
    case L_ACCOUNT:
      /* fixme */
      l->n_posts = api_get_account_posts(l->account, N_STATUS_TO_LOAD, NULL, l->ids);
      memset(l->displayed_posts, 0, l->n_posts * sizeof(status *));
      break;
  }
  memset(l->post_height, -1, l->n_posts);
  if (l->root) {
    for (i = 0; i < l->n_posts; i++) {
      if(!strcmp(l->ids[i], l->root)) {
          l->first_displayed_post = i;
          /* Load first for the header */
          status_free(l->displayed_posts[i]);
          l->displayed_posts[i] = api_get_status(l->ids[i], 1);
          break;
      }
    }
  }
  l->last_displayed_post = 0;
  l->eof = 0;
  l->scrolled = 0;

  gotoxy(LEFT_COL_WIDTH - 4, scrh - 1);
  cputs("   ");

  return l;
}

static void free_list(list *l) {
  char i;
  for (i = 0; i < l->n_posts; i++) {
    free(l->ids[i]);
    status_free(l->displayed_posts[i]);
  }
  account_free(l->account);
  free(l->root);
  free(l->leaf_root);
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
  if (l->first_displayed_post >= 0) {
    l->displayed_posts[l->first_displayed_post] = 
      api_get_status(l->ids[l->first_displayed_post], full);
  }
}

static void print_list(list *l) {
  char i, full;
  char bottom = 0;
  char scrolled = 0;

  
update:
  /* set scrollwindow */
  set_hscrollwindow(LEFT_COL_WIDTH + 1, scrw - LEFT_COL_WIDTH - 1);
  gotoxy(0, 0);
  if (l->scrolled) {
    clrscr();
  }
  l->scrolled = 0;

  if (l->account && l->first_displayed_post == -1) {
    bottom = print_account(l->account, &scrolled);
    if (!bottom) {
      l->account_height = wherey();
    }
    if (scrolled) {
      l->account_height = scrh;
      l->scrolled = 1;
    }
  }

  for (i = max(0, l->first_displayed_post); i < l->n_posts; i++) {
    if (!bottom) {
      status *disp;

      full = (l->root && !strcmp(l->root, l->ids[i]));
      if (l->displayed_posts[i] != NULL)
        disp = l->displayed_posts[i];
      else {
#ifdef __CC65__
        if (i > l->first_displayed_post) 
          cputs(LOADING_TOOT_MSG);
        gotox(0);
#endif
        disp = api_get_status(l->ids[i], full);
#ifdef __CC65__
        if (i > l->first_displayed_post)
          cputs("                ");
        gotox(0);
#endif
      }
      l->displayed_posts[i] = disp;

      if (disp != NULL && bottom == 0) {
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

  set_hscrollwindow(0, scrw);
  if (bottom == 0 && i == l->n_posts) {
    if (load_next_posts(l) > 0)
      goto update;
  }

  print_free_ram();
}

static void shift_posts_down(list *l) {
  char i;
  char scroll_val;

  if (l->first_displayed_post == l->n_posts)
    return;

  if (l->first_displayed_post == -1) {
    scroll_val = l->account_height;
  } else {
    scroll_val = l->post_height[l->first_displayed_post];
  }
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
#ifndef __CC65__
  clrscr();
#endif
  set_hscrollwindow(0, scrw);
  print_free_ram();
}

static int shift_posts_up(list *l) {
  int i;
  signed char scrollval;
  
  if (l->account && l->first_displayed_post == -1) {
      return -1;
  } else if (!l->account && l->first_displayed_post == 0) {
      return -1;
  }

  l->eof = 0;
  if (l->first_displayed_post == 0) {
    scrollval = l->account_height;
  } else {
    scrollval = l->post_height[l->first_displayed_post - 1];
  }
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
#ifndef __CC65__
  clrscr();
#endif
  set_hscrollwindow(0, scrw);
  print_free_ram();
  return 0;
}

/* returns 1 to reload */
static int show_list(list *l) {
  char c;
  
  while (1) {
    status *root_status;
    print_header(l);

    gotoxy(LEFT_COL_WIDTH - 4, scrh - 1);
    cputs("...");
    print_list(l);
    gotoxy(LEFT_COL_WIDTH - 4, scrh - 1);
    cputs("   ");

    root_status = is_root_status_at_top(l);
    c = tolower(cgetc());
    switch(c) {
      case CH_ENTER:
        cur_action = SHOW_FULL_STATUS;
        return 0;
      case CH_ESC:
        cur_action = BACK;
        return 0;
      case CH_CURS_DOWN:
        shift_posts_down(l);
        break;
      case CH_CURS_UP:
        if (shift_posts_up(l) < 0) {
          if (load_prev_posts(l)) {
            return 1;
          }
        }
        break;
      case 'p':
        cur_action = SHOW_ACCOUNT;
        return 0;
      case 'f':
        if (root_status) {
          api_favourite_status(root_status);
        }
        break;
      case 'b':
        if (root_status) {
          api_reblog_status(root_status);
        }
        break;
      case 'd':
        if (root_status) {
          if (api_delete_status(root_status)) {
            cur_action = BACK;
            return 0;
          }
        }
        break;
    }
  }
  return 0;
}

void cli(void) {
  char cur_list, to_clear, to_show;
  list **l, *prev_list;

  screensize(&scrw, &scrh);
  clrscr();

  cur_list = 0;
  cur_action = SHOW_HOME_TIMELINE;
  l = malloc((cur_list + 1) * sizeof(list *));

  while (cur_action != QUIT) {
    switch(cur_action) {
      case SHOW_HOME_TIMELINE:
        l[cur_list] = build_list(NULL, L_HOME_TIMELINE);
        cur_action = NAVIGATE;
        break;
      case SHOW_FULL_STATUS:
      case SHOW_ACCOUNT:
        prev_list = l[cur_list];
        ++cur_list;
        if (cur_action == SHOW_FULL_STATUS) {
          to_clear = prev_list->post_height[prev_list->first_displayed_post] - 2;
          to_show = L_FULL_STATUS;
        } else {
          to_clear = 0;
          to_show = L_ACCOUNT;
        }
        l = realloc(l, (cur_list + 1) * sizeof(list *));
        l[cur_list] = build_list(prev_list->displayed_posts[prev_list->first_displayed_post],
                   to_show);
        clrzone(LEFT_COL_WIDTH + 1, to_clear, scrw - 1, scrh - 1);
        compact_list(prev_list);
        cur_action = NAVIGATE;
        break;
      case NAVIGATE:
        if (show_list(l[cur_list])) {
          if (cur_list > 0) {
            cur_action = (l[cur_list]->account != NULL) 
                            ? SHOW_ACCOUNT : SHOW_FULL_STATUS;
            free_list(l[cur_list]);
            --cur_list;
            uncompact_list(l[cur_list], cur_list > 0);
          } else {
            free_list(l[cur_list]);
            cur_action = SHOW_HOME_TIMELINE;
          }
          clrzone(LEFT_COL_WIDTH + 1, 0, scrw - 1, scrh - 1);
        }
        break;
      case BACK:
        if (cur_list > 0) {
          clrzone(LEFT_COL_WIDTH + 1, 0, scrw - 1, scrh - 1);
          free_list(l[cur_list]);
          --cur_list;
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
