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
#include "clrzone.h"
#include "scrollwindow.h"

#define BUF_SIZE 255

unsigned char scrw, scrh;

char *instance_url = NULL;
char *oauth_token = NULL;

#define SHOW_HOME_TIMELINE 0
#define SHOW_FULL_STATUS   5
#define SHOW_ACCOUNT       6
#define SHOW_SEARCH_RES    7
#define NAVIGATE           8
#define BACK               9
#define QUIT               10
#define CONFIGURE          11
#define COMPOSE            15
#define REPLY              16
#define IMAGES             17
#define SEARCH             18

static char cur_action;
static char search_buf[50] = "colin_mcmillen";

static void print_list(list *l);

static status *get_top_status(list *l) {
  status *root_status;

  if (l->first_displayed_post < 0 || l->n_posts == 0) 
    return NULL;

  root_status = l->displayed_posts[l->first_displayed_post];
  if (root_status && root_status->reblog) {
    root_status = root_status->reblog;
  }

  return root_status;
}

static int print_account(account *a, char *scrolled) {
  char y;
  *scrolled = 0;
  dputs(a->display_name);
  dputc(arobase);
  dputs(a->username);

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
      loaded = api_get_posts(TIMELINE_ENDPOINT "/" HOME_TIMELINE, to_load, last_id, NULL, ".[].id", new_ids);
      break;
    case L_SEARCH:
      loaded = api_search(to_load, search_buf, last_id, new_ids);
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

  gotoxy(LEFT_COL_WIDTH - 4, scrh - 1);
  dputs("...");

  l = malloc(sizeof(list));
  memset(l, 0, sizeof(list));

  if (kind == L_ACCOUNT) {
    l->account = api_get_full_account(root);
    l->first_displayed_post = -1;
  } else {
    if (root) {
      l->root = root;
      l->leaf_root = leaf_root;
    }
    l->first_displayed_post = 0;
  }

  l->kind = kind;
  l->ids = malloc(N_STATUS_TO_LOAD * sizeof(char *));
  l->displayed_posts = malloc(N_STATUS_TO_LOAD * sizeof(status *));
  l->post_height = malloc(N_STATUS_TO_LOAD);

  switch (kind) {
    case L_HOME_TIMELINE:
      l->n_posts = api_get_posts(TIMELINE_ENDPOINT "/" HOME_TIMELINE, N_STATUS_TO_LOAD, NULL, NULL, ".[].id", l->ids);
      memset(l->displayed_posts, 0, l->n_posts * sizeof(status *));
      break;
    case L_SEARCH:
      l->n_posts = api_search(N_STATUS_TO_LOAD, search_buf, NULL, l->ids);
      memset(l->displayed_posts, 0, l->n_posts * sizeof(status *));
      break;
    case L_FULL_STATUS:
      l->n_posts = api_get_status_and_replies(N_STATUS_TO_LOAD, l->root, l->leaf_root, NULL, l->ids);
      memset(l->displayed_posts, 0, l->n_posts * sizeof(status *));
      break;
    case L_ACCOUNT:
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
  } else if (l->n_posts > 0){
    l->displayed_posts[0] = api_get_status(l->ids[0], 0);
  }
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
      if (kbhit()) {
        break;
      }
      full = (l->root && !strcmp(l->root, l->ids[i]));
      if (l->displayed_posts[i] != NULL)
        disp = l->displayed_posts[i];
      else {
        if (i > l->first_displayed_post) 
          dputs(LOADING_TOOT_MSG);
        gotox(0);

        disp = api_get_status(l->ids[i], full);

        if (i > l->first_displayed_post)
          dputs("                ");
        gotox(0);
      }
      l->displayed_posts[i] = disp;

      if (disp != NULL && bottom == 0) {
        bottom = print_status(disp, full, &scrolled);
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

static void configure(void) {
  char *params;
  params = malloc(127);
  snprintf(params, 127, "%s %s", instance_url, oauth_token);
#ifdef __CC65__
  exec("mastoconf", params);
  exit(0);
#else
  printf("exec(mastoconf %s)\n",params);
  exit(0);
#endif
}

void launch_compose(status *s) {
  char *params;
  params = malloc(127);
  snprintf(params, 127, "%s %s %s %s", instance_url, oauth_token, 
                                       translit_charset,
                                       s ? s->id:"");
#ifdef __CC65__
  exec("mastowrite", params);
  exit(0);
#else
  printf("exec(mastowrite %s)\n",params);
  exit(0);
#endif
}

void launch_images(status *s) {
  char *params;
  if (s == NULL) {
    return;
  }
  params = malloc(127);
  snprintf(params, 127, "%s %s %s %s", instance_url, oauth_token, translit_charset, s->id);
#ifdef __CC65__
  exec("mastoimg", params);
  exit(0);
#else
  printf("exec(mastoimg %s)\n",params);
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
  printf("Saving state...");

  fp = fopen(STATE_FILE,"w");
  if (fp == NULL) {
    return;
  }

  if (fprintf(fp, "%d\n", cur_list) < 0)
    goto err_out;

  for (i = 0; i <= cur_list; i++) {
    list *l = lists[i];
    if (fprintf(fp, "%d\n", l->kind) < 0)
      goto err_out;
    if (fprintf(fp, "%s\n", l->root ? l->root : "") < 0)
      goto err_out;
    if (fprintf(fp, "%s\n", l->leaf_root ? l->leaf_root : "") < 0)
      goto err_out;
    if (fprintf(fp, "%d\n", l->last_displayed_post) < 0)
      goto err_out;
    if (fprintf(fp, "%d\n", l->eof) < 0)
      goto err_out;
    if (fprintf(fp, "%d\n", l->scrolled) < 0)
      goto err_out;
    if (fprintf(fp, "%d\n", l->first_displayed_post) < 0)
      goto err_out;
    if (fprintf(fp, "%d\n", l->n_posts) < 0)
      goto err_out;
    for (j = 0; j < l->n_posts; j++) {
      if (fprintf(fp, "%s\n", l->ids[j]) < 0)
        goto err_out;
      if (fprintf(fp, "%d\n", l->post_height[j]) < 0)
        goto err_out;
    }

    if (fprintf(fp, "%s\n", l->account ? l->account->id : "") < 0)
      goto err_out;
    if (fprintf(fp, "%d\n", l->account_height) < 0)
      goto err_out;
  }

  fclose(fp);
  printf("Done.\n");
  return;

err_out:
  fclose(fp);
  unlink(STATE_FILE);
  printf("Error.\n");
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

  printf("Reloading state...");

  fgets(gen_buf, BUF_SIZE, fp);
  num_lists = atoi(gen_buf);
  if (num_lists < 0) {
    *lists = NULL;
    fclose(fp);
    return -1;
  }
  *lists = malloc((num_lists + 1) * sizeof(list *));

  for (i = 0; i <= num_lists; i++) {
    list *l = malloc(sizeof(list));
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

    for (j = 0; j < l->n_posts; j++) {
      fgets(gen_buf, BUF_SIZE, fp);
      *strchr(gen_buf, '\n') = '\0';
      l->ids[j] = strdup(gen_buf);

      fgets(gen_buf, BUF_SIZE, fp);
      l->post_height[j] = atoi(gen_buf);

      if (l->root && !strcmp(l->root, l->ids[j])) {
        l->displayed_posts[j] = api_get_status(l->ids[j], 1);
        loaded = 1;
      }
    }
    if (!loaded && l->first_displayed_post > -1) {
      l->displayed_posts[l->first_displayed_post] = 
          api_get_status(l->ids[l->first_displayed_post], 1);
    }


    fgets(gen_buf, BUF_SIZE, fp);
    if (gen_buf[0] != '\n') {
      *strchr(gen_buf, '\n') = '\0';
      l->account = api_get_full_account(gen_buf);
    }

    fgets(gen_buf, BUF_SIZE, fp);
    l->account_height = atoi(gen_buf);
  }

  fclose(fp);
  unlink(STATE_FILE);
  cur_action = NAVIGATE;

  printf("Done\n");

  return num_lists;
}

static void background_load(list *l) {
  char i;
  for (i = 0; i < l->n_posts; i++) {
    if (l->displayed_posts[i] == NULL) {
      gotoxy(LEFT_COL_WIDTH - 4, scrh - 1);
      dputs("...");

      l->displayed_posts[i] = api_get_status(l->ids[i], 0);

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
    root_status = get_top_status(l);
    print_header(l, root_status);

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
    }
  }
  return 0;
}

void cli(void) {
  signed char cur_list, to_clear, to_show;
  list **l, *prev_list;
  char starting = 1;
  status *disp;
  char *new_root, *new_leaf_root;

  if (starting) {
    cur_list = load_state(&l);
  }
  if (cur_list == -1) {
    cur_list = 0;
    l = malloc(1 * sizeof(list *));
    cur_action = SHOW_HOME_TIMELINE;
  }

  while (cur_action != QUIT) {
    switch(cur_action) {
      case SHOW_HOME_TIMELINE:
        l[cur_list] = build_list(NULL, NULL, L_HOME_TIMELINE);
        cur_action = NAVIGATE;
        break;
      case SHOW_FULL_STATUS:
      case SHOW_ACCOUNT:
      case SHOW_SEARCH_RES:
        prev_list = l[cur_list];
        ++cur_list;
        to_clear = 0;
        new_root = NULL;
        new_leaf_root = NULL;
        /* we don't want get_top_status because we don't want to go into
         * reblog */
        disp = prev_list->displayed_posts[prev_list->first_displayed_post];
        if (cur_action == SHOW_FULL_STATUS) {
          to_clear = prev_list->post_height[prev_list->first_displayed_post] - 2;
          to_show = L_FULL_STATUS;
          new_root = strdup(disp->id);
          new_leaf_root = strdup(disp->reblog ? disp->reblog->id : disp->id);
        } else if (cur_action == SHOW_SEARCH_RES) {
          to_show = L_SEARCH;
        } else if (cur_action == SHOW_ACCOUNT) {
          new_root = strdup(disp->reblog ? disp->reblog->account->id : disp->account->id);
          to_show = L_ACCOUNT;
        }
        l = realloc(l, (cur_list + 1) * sizeof(list *));
        if (cur_list > 0) {
          compact_list(l[cur_list - 1]);
        }
        l[cur_list] = build_list(new_root, new_leaf_root, to_show);
        clrzone(LEFT_COL_WIDTH + 1, to_clear, scrw - 1, scrh - 1);
        /* free status on list n-2 */
        cur_action = NAVIGATE;
        break;
      case NAVIGATE:
        if (starting) {
          starting = 0;
          clrscr();
        }
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
      case CONFIGURE:
          save_state(l, cur_list);
          configure();
          cur_action = NAVIGATE;
          break;
      case COMPOSE:
          save_state(l, cur_list);
          launch_compose(NULL);
          cur_action = NAVIGATE;
      case REPLY:
          save_state(l, cur_list);
          launch_compose(get_top_status(l[cur_list]));
          cur_action = NAVIGATE;
          break;
      case IMAGES:
          save_state(l, cur_list);
          launch_images(get_top_status(l[cur_list]));
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
  if (argc < 4) {
    printf("Missing instance_url, oauth_token and/or charset parameters.\n");
  }

  videomode(VIDEOMODE_80COL);
  screensize(&scrw, &scrh);

  instance_url = argv[1];
  oauth_token = argv[2];
  translit_charset = argv[3];
  if(!strcmp(translit_charset, "ISO646-FR1")) {
    /* § in french charset, cleaner than 'à' */
    arobase = ']';
  }

  cli();
  videomode(VIDEOMODE_40COL);
  exit(0);
}
