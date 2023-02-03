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
#include "cli.h"
#include "print.h"
#include "header.h"
#include "api.h"
#include "list.h"
#include "math.h"
#include "dgets.h"
#include "scrollwindow.h"

#define BUF_SIZE 255

unsigned char scrw, scrh;

char *instance_url = NULL;
char *oauth_token = NULL;
char monochrome = 1;

#define SHOW_HOME_TIMELINE   0
#define SHOW_LOCAL_TIMELINE  1
#define SHOW_GLOBAL_TIMELINE 2
#define SHOW_FULL_STATUS     5
#define SHOW_ACCOUNT         6
#define SHOW_SEARCH_RES      7
#define NAVIGATE             8
#define BACK                 9
#define QUIT                10
#define CONFIGURE           11
#define COMPOSE             15
#define REPLY               16
#define IMAGES              17
#define SEARCH              18
#define SHOW_NOTIFICATIONS  19

static char cur_action;
static char search_buf[50];

static void print_list(list *l);

static status *get_top_status(list *l) {
  status *root_status;
  signed char first = l->first_displayed_post;

  root_status = NULL;
  if (l->kind == SHOW_NOTIFICATIONS) 
    goto err_out;
  
  if (first >= 0 && first < l->n_posts) {
    root_status = (status *)l->displayed_posts[first];
    if (root_status && root_status->reblog) {
      root_status = root_status->reblog;
    }
  }
err_out:
  return root_status;
}

static int print_account(account *a, char *scrolled) {
  char y;
  *scrolled = 0;
  dputs(a->display_name);
  dputs("\r\n");
  dputc(arobase);
  dputs(a->username);
  dputs("\r\n");

  cprintf("%ld following, %ld followers\r\n"
          "Here since %s", a->following_count, a->followers_count, a->created_at);

  api_relationship_get(a, 0);
  y = 0;
  if (api_relationship_get(a, RSHIP_FOLLOWING)) {
    gotoxy(32, y);
    dputs("             You follow them\r\n");
  } else if (api_relationship_get(a, RSHIP_FOLLOW_REQ)) {
    gotoxy(32, ++y);
    dputs("You requested to follow them\r\n");
  }
  if (api_relationship_get(a, RSHIP_FOLLOWED_BY)) {
    gotoxy(32, ++y);
    dputs("             They follow you\r\n");
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

static int print_notification(notification *n) {
  char width;
  char *w;
  
  width = scrw - LEFT_COL_WIDTH - 1;
  n->displayed_at = wherey();
  cprintf("%s - %s", n->display_name, n->created_at);
  CHECK_AND_CRLF();
  w = notification_verb(n);
  dputs(w);
  dputs(": ");

  width = (2 * width) - strlen(w) - 6;
  w = n->excerpt;
  while (width > 0 && *w) {
    if (*w == '\n') {
      dputc(' ');
    } else {
      dputc(*w);
    }
    ++w;
    --width;
  }
  if (*w) {
    dputs("...");
  }
  CHECK_AND_CRLF();
  if (wherey() == scrh - 1) {
    return -1;
  }

  chline(scrw - LEFT_COL_WIDTH - 1);
  if (wherey() == scrh - 1) {
    return -1;
  }
  return 0;
}

#define N_STATUS_TO_LOAD 10
#define LOADING_TOOT_MSG "Loading toot..."

static void item_free(list *l, char i) {
  if (l->kind != SHOW_NOTIFICATIONS) {
    status_free((status *)l->displayed_posts[i]);
  } else {
    notification_free((notification *)l->displayed_posts[i]);
  }
}

static item *item_get(list *l, char i, char full) {
  if (l->kind == SHOW_NOTIFICATIONS) {
    return (item *)api_get_notification(l->ids[i]);
  }
  return (item *)api_get_status(l->ids[i], full);
}

static char load_next_posts(list *l) {
  char *last_id;
  char **new_ids;
  char loaded, i;
  char to_load, list_len;

  if (l->eof) {
    return 0;
  }

  to_load = min(N_STATUS_TO_LOAD / 2, l->first_displayed_post);
  new_ids = malloc(to_load * sizeof(char *));

  dputs(LOADING_TOOT_MSG);

  list_len = l->n_posts;
  last_id = list_len > 0 ? l->ids[list_len - 1] : NULL;

  switch (l->kind) {
    case SHOW_HOME_TIMELINE:
    case SHOW_LOCAL_TIMELINE:
    case SHOW_GLOBAL_TIMELINE:
      loaded = api_get_posts(tl_endpoints[l->kind], to_load, last_id, tl_filter[l->kind], ".[].id", new_ids);
      break;
    case SHOW_SEARCH_RES:
      loaded = api_search(to_load, search_buf, last_id, new_ids);
      break;
    case SHOW_FULL_STATUS:
      loaded = api_get_status_and_replies(to_load, l->root, l->leaf_root, last_id, new_ids);
      break;
    case SHOW_ACCOUNT:
      loaded = api_get_account_posts(l->account, to_load, last_id, new_ids);
      break;
    case SHOW_NOTIFICATIONS:
      loaded = api_get_notifications(to_load, last_id, new_ids);
      break;
    default:
      loaded = 0;
  }

  if (loaded > 0) {
    char offset;
    /* free first ones */
    for (i = 0; i < loaded; i++) {
      item_free(l, i);
    }

    /* move last ones to first ones */
    for (i = 0; i < list_len - loaded ; i++) {
      offset = i + loaded;
      l->displayed_posts[i] = l->displayed_posts[offset];
      l->post_height[i] = l->post_height[offset];
      /* strcpy to avoid memory fragmentation */
      strcpy(l->ids[i], l->ids[offset]);
    }
    
    /* Set new ones at end */
    for (i = list_len - loaded; i < list_len; i++) {
      offset = i - (list_len - loaded);
      l->displayed_posts[i] = NULL;
      l->post_height[i] = -1;
      /* avoid memory fragmentation, get rid
       * of new_ids-related storage */
      strcpy(l->ids[i], new_ids[offset]);
      free(new_ids[offset]);
    }

    l->first_displayed_post -= loaded;
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
  dputs("All caught up! Maybe reload? (y/N)\r\n");
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

static int show_search(void) {
  set_hscrollwindow(LEFT_COL_WIDTH + 1, scrw - LEFT_COL_WIDTH - 1);

  scrolldn();
  scrolldn();
  
  gotoxy(0,1);
  chline(scrw - LEFT_COL_WIDTH - 1);
  gotoxy(0, 0);
  dputs("Search: ");

  dget_text(search_buf, 49, NULL);

  if (search_buf[0] != '\0') {
    clrscr();
    set_hscrollwindow(0, scrw);
    return 1;
  }

  scrollup();
  scrollup();

  set_hscrollwindow(0, scrw);
  return 0;
}

/* root is either an account or status id, depending on kind.
 * leaf_root is a reblogged status id
 */
static list *build_list(char *root, char *leaf_root, char kind) {
  list *l;
  char i;
  char n_posts;

  gotoxy(LEFT_COL_WIDTH - 4, scrh - 1);
  dputs("...");

  l = malloc(sizeof(list));
  memset(l, 0, sizeof(list));

  if (kind == SHOW_ACCOUNT) {
    l->account = api_get_full_account(root);
    l->first_displayed_post = -1;
  } else {
    if (root && *root) {
      l->root = strdup(root);
      l->leaf_root = strdup(leaf_root);
    }
    l->first_displayed_post = 0;
  }

  l->kind = kind;
  l->ids = malloc(N_STATUS_TO_LOAD * sizeof(char *));
  l->displayed_posts = malloc(N_STATUS_TO_LOAD * sizeof(item *));
  l->post_height = malloc(N_STATUS_TO_LOAD);

  switch (kind) {
    case SHOW_HOME_TIMELINE:
    case SHOW_LOCAL_TIMELINE:
    case SHOW_GLOBAL_TIMELINE:
      n_posts = api_get_posts(tl_endpoints[kind], N_STATUS_TO_LOAD, NULL, tl_filter[kind], ".[].id", l->ids);
      break;
    case SHOW_SEARCH_RES:
      n_posts = api_search(N_STATUS_TO_LOAD, search_buf, NULL, l->ids);
      break;
    case SHOW_FULL_STATUS:
      n_posts = api_get_status_and_replies(N_STATUS_TO_LOAD, l->root, l->leaf_root, NULL, l->ids);
      break;
    case SHOW_ACCOUNT:
      n_posts = api_get_account_posts(l->account, N_STATUS_TO_LOAD, NULL, l->ids);
      break;
    case SHOW_NOTIFICATIONS:
      n_posts = api_get_notifications(N_STATUS_TO_LOAD, NULL, l->ids);
      break;
    default:
      n_posts = 0;
  }
  memset(l->displayed_posts, 0, n_posts * sizeof(item *));
  memset(l->post_height, -1, n_posts);

  if (root) {
    for (i = 0; i < n_posts; i++) {
      if(!strcmp(l->ids[i], root)) {
          l->first_displayed_post = i;
          /* Load first for the header */
          l->displayed_posts[i] = item_get(l, i, 1);
          break;
      }
    }
  } else if (n_posts > 0){
    l->displayed_posts[0] = item_get(l, 0, 0);
  }
  l->n_posts = n_posts;
  l->last_displayed_post = 0;
  l->eof = 0;
  l->scrolled = 0;

  gotoxy(LEFT_COL_WIDTH - 4, scrh - 1);
  dputs("   ");

  return l;
}

static void free_list(list *l) {
  char i;
  for (i = 0; i < l->n_posts; i++) {
    free(l->ids[i]);
    item_free(l, i);
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
    item_free(l, i);
    l->displayed_posts[i] = NULL;
  }
}

static void uncompact_list(list *l) {
  signed char first, full;
  first = l->first_displayed_post;
  if (first >= 0) {
    full = (l->root && !strcmp(l->root, l->ids[first]));
    l->displayed_posts[first] = 
      item_get(l, first, full);
  }
}

static void clrscrollwin(void) {
  set_hscrollwindow(LEFT_COL_WIDTH + 1, scrw - LEFT_COL_WIDTH - 1);
  clrscr();
  set_hscrollwindow(0, scrw);
}

static void print_list(list *l) {
  char i, full;
  char bottom = 0;
  char scrolled = 0;
  signed char first;
  char n_posts;


update:
  /* copy to temp vars to avoid pointer arithmetic */
  first = l->first_displayed_post;
  n_posts = l->n_posts;

  /* set scrollwindow */
  set_hscrollwindow(LEFT_COL_WIDTH + 1, scrw - LEFT_COL_WIDTH - 1);
  gotoxy(0, 0);
  if (l->scrolled) {
    clrscr();
  }
  l->scrolled = 0;

  if (l->account && first == -1) {
    bottom = print_account(l->account, &scrolled);
    if (!scrolled) {
      l->account_height = wherey();
    } else {
      l->account_height = scrh;
      l->scrolled = 1;
    }
  }

  for (i = max(0, first); i < n_posts; i++) {
    if (!bottom) {
      item *disp;
      if (kbhit()) {
        break;
      }
      full = (l->root && !strcmp(l->root, l->ids[i]));
      disp = (item *)l->displayed_posts[i];
      if (disp == NULL) {
        if (i > first) {
          dputs(LOADING_TOOT_MSG);
          gotox(0);
        }

        disp = item_get(l, i, full);
        l->displayed_posts[i] = disp;

        if (i > first) {
          dputs("                ");
          gotox(0);
        }
      }

      if (disp != NULL && bottom == 0) {
        if (l->kind != SHOW_NOTIFICATIONS) {
          bottom = print_status((status *)disp, full, &scrolled);
        } else {
          bottom = print_notification((notification *)disp);
        }
        if (!scrolled) {
          l->post_height[i] = wherey() - disp->displayed_at;
        } else {
          l->post_height[i] = scrh;
          l->scrolled = 1;
        }
        l->last_displayed_post = i;
      }
    }
  }

  set_hscrollwindow(0, scrw);
  if (bottom == 0 && i == n_posts) {
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

static char calc_post_height(status *s) {
  char height;
  char *w, x;

  height = 6; /* header(username + date + display_name) + one line content + footer(\r\n + stats + line)*/
  if (s->reblog) {
    ++height;
  }
  w = s->reblog ? s->reblog->content : s->content;

  x = 0;
  while (*w) {
    if (*w == '\n') {
      x = 0;
      ++height;
    } else {
      ++x;
      if (x == scrw - LEFT_COL_WIDTH - 1) {
        x = 0;
        ++height;
      }
    }
    ++w;
  }
  return height;
}

static int shift_posts_up(list *l) {
  int i;
  signed char scrollval;
  signed char first;

  first = l->first_displayed_post;
  if (l->account && first == -1) {
      return -1;
  } else if (!l->account && first == 0) {
      return -1;
  }

  l->eof = 0;

  l->first_displayed_post = --first;

  if (first == -1) {
    scrollval = l->account_height;
  } else {
    if (l->displayed_posts[first] == NULL) {
      l->displayed_posts[first] =
        item_get(l, first, 0);
    }
    if (l->post_height[first] == -1) {
      if (l->kind != SHOW_NOTIFICATIONS) {
        l->post_height[first] = 
          calc_post_height((status *)l->displayed_posts[first]);
      } else {
        l->post_height[first] = 4;
      }
    }
    scrollval = l->post_height[first];
  }
  if (scrollval < 0) {
    scrollval = scrh;
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

static void launch_command(char *command, char *p1, char *p2, char *p3, char *p4) {
  char *params;
  params = malloc(127);
  snprintf(params, 127, "%s %s %s %s %s %s", 
            instance_url, oauth_token,
            p1?p1:"", p2?p2:"", p3?p3:"", p4?p4:"");
#ifdef __CC65__
  exec(command, params);
  exit(0);
#else
  printf("exec(%s %s)\n",command, params);
  exit(0);
#endif
}

#ifndef __APPLE2ENH__
#define STATE_FILE "mastostate"
#else
#define STATE_FILE "/RAM/mastostate"
#endif

static void save_state(list **lists, char cur_list) {
  char i,j;
  FILE *fp;

#ifdef __CC65__
  _filetype = PRODOS_T_TXT;
#endif

  clrscr();
  dputs("Saving state...");

  fp = fopen(STATE_FILE,"w");
  if (fp == NULL) {
    return;
  }

  if (fprintf(fp, "%d\n", cur_list) < 0)
    goto err_out;

  for (i = 0; i <= cur_list; i++) {
    list *l;
    l = lists[i];
    if (fprintf(fp, "%d\n"
                    "%s\n"
                    "%s\n"
                    "%d\n"
                    "%d\n"
                    "%d\n"
                    "%d\n"
                    "%d\n"
                    "%s\n"
                    "%d\n",
                    l->kind,
                    l->root ? l->root : "",
                    l->leaf_root ? l->leaf_root : "",
                    l->last_displayed_post,
                    l->eof,
                    l->scrolled,
                    l->first_displayed_post,
                    l->n_posts,
                    l->account ? l->account->id : "",
                    l->account_height) < 0) {
      goto err_out;
    }
    for (j = 0; j < l->n_posts; j++) {
      if (fprintf(fp, "%s\n"
                      "%d\n",
                      l->ids[j],
                      l->post_height[j]) < 0) {
        goto err_out;
      }
    }
  }

  fclose(fp);
  dputs("Done.\n");
  return;

err_out:
  fclose(fp);
  unlink(STATE_FILE);
  dputs("Error.\n");
}


static int load_state(list ***lists) {
  char i,j, loaded;
  signed char num_lists;
  FILE *fp;

#ifdef __CC65__
  _filetype = PRODOS_T_TXT;
#endif

  *lists = NULL;
  fp = fopen(STATE_FILE,"r");
  if (fp == NULL) {
    *lists = NULL;
    return -1;
  }

  loaded = 0;

  dputs("Reloading state...");

  fgets(gen_buf, BUF_SIZE, fp);
  num_lists = atoi(gen_buf);
  if (num_lists < 0) {
    *lists = NULL;
    fclose(fp);
    return -1;
  }

  *lists = malloc((num_lists + 1) * sizeof(list *));

  for (i = 0; i <= num_lists; i++) {
    list *l;

    l = malloc(sizeof(list));
    memset(l, 0, sizeof(list));
    (*lists)[i] = l;

    fgets(gen_buf, BUF_SIZE, fp);
    l->kind = atoi(gen_buf);

    fgets(gen_buf, BUF_SIZE, fp);
    if (gen_buf[0] != '\n') {
      *strchr(gen_buf, '\n') = '\0';
      l->root = strdup(gen_buf);
    }

    fgets(gen_buf, BUF_SIZE, fp);
    if (gen_buf[0] != '\n') {
      *strchr(gen_buf, '\n') = '\0';
      l->leaf_root = strdup(gen_buf);
    }
  
    fgets(gen_buf, BUF_SIZE, fp);
    l->last_displayed_post = atoi(gen_buf);

    fgets(gen_buf, BUF_SIZE, fp);
    l->eof = atoi(gen_buf);

    fgets(gen_buf, BUF_SIZE, fp);
    l->scrolled = atoi(gen_buf);

    fgets(gen_buf, BUF_SIZE, fp);
    l->first_displayed_post = atoi(gen_buf);

    fgets(gen_buf, BUF_SIZE, fp);
    l->n_posts = atoi(gen_buf);
    l->ids = malloc(l->n_posts * sizeof(char *));
    l->post_height = malloc(l->n_posts * sizeof(char));
    l->displayed_posts = malloc(l->n_posts * sizeof(status *));
    memset(l->displayed_posts, 0, l->n_posts * sizeof(status *));

    fgets(gen_buf, BUF_SIZE, fp);
    if (gen_buf[0] != '\n') {
      *strchr(gen_buf, '\n') = '\0';
      l->account = api_get_full_account(gen_buf);
    }

    fgets(gen_buf, BUF_SIZE, fp);
    l->account_height = atoi(gen_buf);

    for (j = 0; j < l->n_posts; j++) {
      fgets(gen_buf, BUF_SIZE, fp);
      *strchr(gen_buf, '\n') = '\0';
      l->ids[j] = strdup(gen_buf);

      fgets(gen_buf, BUF_SIZE, fp);
      l->post_height[j] = atoi(gen_buf);

      if (l->root && !strcmp(l->root, l->ids[j])) {
        l->displayed_posts[j] = item_get(l, j, 1);
        loaded = 1;
      }
    }
    if (!loaded && l->first_displayed_post > -1) {
      l->displayed_posts[l->first_displayed_post] = 
          item_get(l, l->first_displayed_post, 1);
    }
  }

  fclose(fp);
  unlink(STATE_FILE);
  cur_action = NAVIGATE;

  dputs("Done\n");

  return num_lists;
}

static void background_load(list *l) {
  char i;
  for (i = 0; i < l->n_posts; i++) {
    if (l->displayed_posts[i] == NULL) {
      gotoxy(LEFT_COL_WIDTH - 4, scrh - 1);
      dputs("...");

      l->displayed_posts[i] = item_get(l, i, 0);

      gotoxy(LEFT_COL_WIDTH - 4, scrh - 1);
      dputs("   ");
      break; /* background load one by one to check kb */
    }
  }
}

/* returns 1 to reload */
static int show_list(list *l) {
  char c;
  
  while (1) {
    status *root_status;
    notification *root_notif;
    
    if (l->kind == SHOW_NOTIFICATIONS) {
      root_status = NULL;
      root_notif = (notification *)l->displayed_posts[l->first_displayed_post];
    } else {
      root_status = (status *)get_top_status(l);
      root_notif = NULL;
    }
    
    print_header(l, root_status, root_notif);

    gotoxy(LEFT_COL_WIDTH - 4, scrh - 1);
    dputs("...");
    print_list(l);
    gotoxy(LEFT_COL_WIDTH - 4, scrh - 1);
    dputs("   ");

    while (!kbhit()) {
      background_load(l);
      print_free_ram();
    }
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
      case 'o':
        cur_action = CONFIGURE;
        return 0;
      case 'c':
        cur_action = COMPOSE;
        return 0;
      case 'r':
        cur_action = REPLY;
        return 0;
      case 'i':
        cur_action = IMAGES;
        return 0;
      case 's':
        cur_action = SEARCH;
        return 0;
      case 'n':
        cur_action = SHOW_NOTIFICATIONS;
        return 0;
      case 'h':
        cur_action = SHOW_HOME_TIMELINE;
        return 0;
      case 'l':
        cur_action = SHOW_LOCAL_TIMELINE;
        return 0;
      case 'g':
        cur_action = SHOW_GLOBAL_TIMELINE;
        return 0;
    }
  }
  return 0;
}

void cli(void) {
  signed char cur_list;
  list **l, *prev_list;
  char starting = 1;
  item *disp;
  status *disp_status;
  notification *disp_notif;
  /* static buffer because we need to copy the
   * IDs before compacting parent list, to avoid
   * memory fragmentation */
  char new_root[32], new_leaf_root[32];

  if (starting) {
    cur_list = load_state(&l);
    /* else cur_action will be NAVIGATE */
  }
  if (cur_list == -1) {
    cur_action = SHOW_HOME_TIMELINE;
  }

  while (cur_action != QUIT) {
    switch(cur_action) {
      case SHOW_HOME_TIMELINE:
      case SHOW_LOCAL_TIMELINE:
      case SHOW_GLOBAL_TIMELINE:
        ++cur_list;
        if (cur_list > 0) {
          compact_list(l[cur_list - 1]);
        }
        l = realloc(l, (cur_list + 1) * sizeof(list *));
        l[cur_list] = build_list(NULL, NULL, cur_action);
        clrscrollwin();
        cur_action = NAVIGATE;
        break;
      case SHOW_FULL_STATUS:
      case SHOW_ACCOUNT:
      case SHOW_SEARCH_RES:
      case SHOW_NOTIFICATIONS:
        prev_list = l[cur_list];
        ++cur_list;
        *new_root = '\0';
        *new_leaf_root = '\0';
        /* we don't want get_top_status because we don't want to go into
         * reblog */
        disp = prev_list->displayed_posts[prev_list->first_displayed_post];
        if (prev_list->kind != SHOW_NOTIFICATIONS) {
          disp_status = (status *)disp;
          if (cur_action == SHOW_FULL_STATUS) {
            strcpy(new_root, disp_status->id);
            strcpy(new_leaf_root, disp_status->reblog ? disp_status->reblog->id : disp_status->id);
          } else if (cur_action == SHOW_ACCOUNT) {
            strcpy(new_root, disp_status->reblog ? disp_status->reblog->account->id : disp_status->account->id);
          }
        } else {
          disp_notif = (notification *)disp;
          if (cur_action == SHOW_FULL_STATUS && disp_notif->status_id) {
            strcpy(new_root, disp_notif->status_id);
            strcpy(new_leaf_root, disp_notif->status_id);
          } else {
            cur_action = SHOW_ACCOUNT;
            strcpy(new_root, disp_notif->account_id);
          }
        }
        compact_list(l[cur_list - 1]);
        l = realloc(l, (cur_list + 1) * sizeof(list *));
        l[cur_list] = build_list(new_root, new_leaf_root, cur_action);
        clrscrollwin();
        cur_action = NAVIGATE;
        break;
      case NAVIGATE:
        if (starting) {
          starting = 0;
          clrscr();
        }
        if (show_list(l[cur_list])) {
          /* reload, freeing the list
           * and resetting action to load it */
          cur_action = l[cur_list]->kind;
          free_list(l[cur_list]);
          --cur_list;
          clrscrollwin();
        }
        break;
      case BACK:
        if (cur_list > 0) {
          free_list(l[cur_list]);
          --cur_list;
          l = realloc(l, (cur_list + 1) * sizeof(list *));
          uncompact_list(l[cur_list]);
          clrscrollwin();
          cur_action = NAVIGATE;
        } else {
          cur_action = QUIT;
        }
        break;
      case CONFIGURE:
          save_state(l, cur_list);
          launch_command("mastoconf", NULL, NULL, NULL, NULL);
          cur_action = NAVIGATE;
          break;
      case COMPOSE:
          save_state(l, cur_list);
          launch_command("mastowrite", translit_charset, NULL, NULL, NULL);
          cur_action = NAVIGATE;
          break;
      case REPLY:
          save_state(l, cur_list);
          launch_command("mastowrite", translit_charset, get_top_status(l[cur_list])->id, NULL, NULL);
          cur_action = NAVIGATE;
          break;
      case IMAGES:
          save_state(l, cur_list);
          if (l[cur_list]->account) {
            launch_command("mastoimg", translit_charset, monochrome?"1":"0", "a", l[cur_list]->account->id);
          } else {
            launch_command("mastoimg", translit_charset, monochrome?"1":"0", "s", get_top_status(l[cur_list])->id);
          }
          cur_action = NAVIGATE;
          break;
      case SEARCH:
          if (show_search()) {
            cur_action = SHOW_SEARCH_RES;
          } else {
            cur_action = NAVIGATE;
          }
          break;
      case QUIT:
        goto out;
    }
  }
out:
  free_list(l[cur_list]);
  free(l);
}

int main(int argc, char **argv) {
  FILE *fp;
  if (argc < 3) {
    dputs("Missing parameters.\r\n");
  }

  videomode(VIDEOMODE_80COL);
  screensize(&scrw, &scrh);

  instance_url = argv[1];
  oauth_token = argv[2];


  fp = fopen("clisettings", "r");
  translit_charset = US_CHARSET;

  if (fp != NULL) {
    fgets(gen_buf, 16, fp);
    if (strchr(gen_buf, '\n')) {
      *strchr(gen_buf, '\n') = '\0';
    }
    translit_charset = strdup(gen_buf);

    fgets(gen_buf, 16, fp);
    if (gen_buf[0] == '0') {
      monochrome = 0;
    }

    fclose(fp);
  }

  if(!strcmp(translit_charset, "ISO646-FR1")) {
    /* § in french charset, cleaner than 'à' */
    arobase = ']';
  }

  cli();
  videomode(VIDEOMODE_40COL);
  exit(0);
}
