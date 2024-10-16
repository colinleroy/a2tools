#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include "malloc0.h"
#include "platform.h"
#include "surl.h"
#include "clrzone.h"
#include "extended_conio.h"
#include "path_helper.h"
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
#include "runtime_once_clean.h"

#ifdef __CC65__
#pragma code-name (push, "LC")
#endif

#pragma register-vars(push, on)

unsigned char scrw, scrh;

char *instance_url = NULL;
char *oauth_token = NULL;
char monochrome = 1;
char writable_lines = 24;

extern account *my_account;

static list **all_lists;
static signed char cur_list_idx;

char cur_action;
static char search_buf[50];
static char search_type = 'm';
static char rship_toggle_action = RSHIP_FOLLOWING;
static char notifications_type = NOTIFICATION_FAVOURITE;
static void print_list(list *l, signed char limit);

static status *get_top_status(list *l) {
  status *root_status;
  signed char first = l->first_displayed_post;

  root_status = NULL;
  if (l->kind == SHOW_NOTIFICATIONS)
    goto err_out;

  if (first >= 0 && first < l->n_posts) {
    root_status = (status *)l->displayed_posts[first];
  }
err_out:
  return root_status;
}

static int print_account(account *a) {
  char y;
  dputs(a->display_name); /* 0 */
  CHECK_AND_CRLF();
  dputc(arobase);         /* 1 */
  dputs(a->username);
  CHECK_AND_CRLF();

  cprintf("%ld following, %ld followers\r\n" /* 2 */
          "Here since %s", /* 3 */
          a->following_count, a->followers_count, a->created_at);
  CHECK_NO_CRLF();

  api_relationship_get(a, 0);

#if NUMCOLS == 80
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
#else
  if (api_relationship_get(a, RSHIP_FOLLOWING)) {
    dputs("You follow them\r\n");
    CHECK_AND_CRLF();
  } else if (api_relationship_get(a, RSHIP_FOLLOW_REQ)) {
    dputs("You requested to follow them\r\n");
    CHECK_AND_CRLF();
  }
  if (api_relationship_get(a, RSHIP_FOLLOWED_BY)) {
    dputs("They follow you\r\n");
    CHECK_AND_CRLF();
  }
#endif

  if (wherey() < 4) {
    gotoy(4);
    CHECK_NO_CRLF();
  }

  CHECK_AND_CRLF();
  if (print_buf(a->note, 0, 1) < 0) {
    return -1;
  }
  CHECK_AND_CRLF();
  CHLINE_SAFE();

  return 0;
}

static int print_notification(notification *n) {
  char y, width;
  register char *w;

  width = scrw - RIGHT_COL_START;
  n->displayed_at = wherey();
  dputs(n->display_name);
#if NUMCOLS == 80
  gotox(TIME_COLUMN);
  if (writable_lines != 1)
    dputs(n->created_at);
  else
    cputs(n->created_at); /* no scrolling please */
  CHECK_NO_CRLF();
#else
  CHECK_AND_CRLF();
#endif

  w = notification_verb[n->type];
  y = wherey();
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
  if (y == wherey()) {
    dputc('\n');
  }

  CHECK_NO_CRLF();
  CHECK_AND_CRLF();

  CHLINE_SAFE();

  return 0;
}

/* Make those the same width so they overwrite each other */
#define LOADING_TOOT_MSG    "Loading toot...      "
#define NOTHING_TO_LOAD_MSG "Nothing more to load!"
#define CLEAR_LOAD_MSG      "                     "

static void item_free(list *l, char i) {
  if (l->kind != SHOW_NOTIFICATIONS) {
    status_free((status *)l->displayed_posts[i]);
  } else {
    notification_free((notification *)l->displayed_posts[i]);
  }
}

#ifdef __CC65__
#pragma code-name (pop)
#endif

static item *item_get(list *l, char i, char full) {
  if (l->kind == SHOW_NOTIFICATIONS) {
    return (item *)api_get_notification(l->ids[i]);
  }
  return (item *)api_get_status(l->ids[i], full);
}

static char load_around(list *l, char to_load, char *first, char *last, char **new_ids) {
  char loaded;
  l->half_displayed_post = 0;
  if (l->kind == SHOW_BOOKMARKS && (first || last)) {
    /* Must paginate those with Link: HTTP headers :( */
    return 0;
  }

  switch (l->kind) {
    case SHOW_HOME_TIMELINE:
    case SHOW_LOCAL_TIMELINE:
    case SHOW_GLOBAL_TIMELINE:
    case SHOW_BOOKMARKS:
      loaded = api_get_posts(tl_endpoints[l->kind], to_load, first, last, tl_filter[l->kind], ".[].id", new_ids);
      break;
    case SHOW_SEARCH_RES:
      loaded = api_search(to_load, search_buf, 'm', first, last, new_ids);
      break;
    case SHOW_FULL_STATUS:
      loaded = api_get_status_and_replies(to_load, l->root, l->leaf_root, first, last, new_ids);
      break;
    case SHOW_PROFILE:
      loaded = api_get_account_posts(l->account, to_load, first, last, new_ids);
      break;
    case SHOW_NOTIFICATIONS:
      loaded = api_get_notifications(to_load, notifications_type, first, last, new_ids);
      break;
    default:
      loaded = 0;
  }
  return loaded;
}

static void free_posts(list *l, char from, char to) {
  char i;
  for (i = from; i < to; i++) {
    item_free(l, i);
  }
}

static void move_post(list *l, char from, char to) {
  l->displayed_posts[from] = l->displayed_posts[to];
  l->post_height[from] = l->post_height[to];
  /* strcpy to avoid memory fragmentation */
  strcpy(l->ids[from], l->ids[to]);
}

static void init_post(list *l, char **new_ids, char from, char to) {
  l->displayed_posts[to] = NULL;
  l->post_height[to] = -1;
  /* avoid memory fragmentation, get rid
   * of new_ids-related storage */
  strcpy(l->ids[to], new_ids[from]);
  free(new_ids[from]);
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
  new_ids = malloc0(to_load * sizeof(char *));

  dputs(LOADING_TOOT_MSG);

  list_len = l->n_posts;
  last_id = list_len > 0 ? l->ids[list_len - 1] : NULL;

  loaded = load_around(l, to_load, NULL, last_id, new_ids);

  if (loaded > 0) {
    char split;
    /* free first ones */
    free_posts(l, 0, loaded);

    /* move last ones to first ones */
    split = list_len - loaded;
    for (i = 0; i < split ; i++) {
      move_post(l, i, i + loaded);
    }

    /* Set new ones at end */
    for (i = split; i < list_len; i++) {
      init_post(l, new_ids, i - split, i);
    }

    l->first_displayed_post -= loaded;
  } else {
    gotox(0);
    dputs(NOTHING_TO_LOAD_MSG);
    l->eof = 1;
  }
  free(new_ids);
  return loaded;
}

static char load_prev_posts(list *l) {
  char *first_id;
  char **new_ids;
  signed char loaded, i;
  char to_load, list_len;

  to_load = N_STATUS_TO_LOAD / 2;
  new_ids = malloc0(to_load * sizeof(char *));

  set_hscrollwindow(RIGHT_COL_START, scrw - RIGHT_COL_START);
  scrolldown_n(2);
  gotoxy(0,1);
  chline(scrw - RIGHT_COL_START);
  gotoxy(0, 0);
  dputs(LOADING_TOOT_MSG);

  list_len = l->n_posts;
  first_id = list_len > 0 ? l->ids[0] : NULL;

  loaded = load_around(l, to_load, first_id, NULL, new_ids);

  if (loaded > 0) {
    char split;

    split = list_len - loaded;
    /* free last ones */
    free_posts(l, split, list_len);

    /* move first ones to first ones */
    for (i = split - 1; i >= 0 ; i--) {
      move_post(l, i + loaded, i);
    }

    /* Set new ones at first */
    for (i = 0; i < loaded; i++) {
      init_post(l, new_ids, i, i);

    }
    /* fetch last one before removing the
     * "Loading" header */
    i = loaded - 1;
    l->displayed_posts[i] = item_get(l, i, !strcmp(l->root, l->ids[i]));

    l->first_displayed_post += loaded;
  }
  scrollup_n(2);
  set_hscrollwindow(0, scrw);

  free(new_ids);
  return loaded;
}

static char search_footer(char c) {
  c = tolower(c);
  switch (c) {
    case 't':
      search_type = 'm';
      break;
    case 'a':
      search_type = 'a';
      break;
    default:
      break;
  }
  gotoxy(0,1);
  cprintf("(%c) Toots (%c) Account   ("KEY_COMB" + T/A)\r\n",
          search_type == 'm' ? '*':' ',
          search_type == 'a' ? '*':' ');
  chline(scrw - RIGHT_COL_START);
  gotoxy(0, 0);
  return 0;
}

static int show_search(void) {
  set_hscrollwindow(RIGHT_COL_START, scrw - RIGHT_COL_START);

  scrolldown_n(3);

  search_footer(search_type);
  dputs("Search: ");

  search_buf[0] = '\0';
  dget_text(search_buf, 49, search_footer, 0);

  if (search_buf[0] != '\0') {
    clrscr();
    set_hscrollwindow(0, scrw);
    return 1;
  }

  scrollup_n(3);

  set_hscrollwindow(0, scrw);
  return 0;
}

static void __fastcall__ load_indicator(char on) {
#if NUMCOLS == 80
  gotoxy(LEFT_COL_WIDTH - 4, scrh - 1);
  dputs(on ? "...":"   ");
#endif
}
/* root is either an account or status id, depending on kind.
 * leaf_root is a reblogged status id
 */
static list *build_list(char *root, char *leaf_root, char kind) {
  list *l;
  char i, found_root;
  char n_posts;

  load_indicator(1);

  l = malloc0(sizeof(list));

  if (kind == SHOW_PROFILE) {
    l->account = api_get_full_account(root);
    l->first_displayed_post = -1;
  } else {
    if (root && *root) {
      l->root = strdup(root);
      l->leaf_root = strdup(leaf_root);
    }
  }

  l->kind = kind;

  n_posts = load_around(l, N_STATUS_TO_LOAD, NULL, NULL, l->ids);

  memset(l->displayed_posts, 0, n_posts * sizeof(item *));
  memset(l->post_height, -1, n_posts);

  found_root = 0;
  if (root) {
    for (i = 0; i < n_posts; i++) {
      if(!strcmp(l->ids[i], root)) {
          l->first_displayed_post = i;
          /* Load first for the header */
          l->displayed_posts[i] = item_get(l, i, 1);
          found_root = 1;
          break;
      }
    }
  }
  if (n_posts > 0 && !found_root) {
    l->displayed_posts[0] = item_get(l, 0, 0);
  }
  l->n_posts = n_posts;

  load_indicator(0);

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
  l->half_displayed_post = 0;
  if (first >= 0) {
    full = (l->root && !strcmp(l->root, l->ids[first]));
    l->displayed_posts[first] =
      item_get(l, first, full);
  }
}

static void clrscrollwin(void) {
  set_hscrollwindow(RIGHT_COL_START, scrw - RIGHT_COL_START);
  clrscr();
  set_hscrollwindow(0, scrw);
}

char hide_cw = 1;

static void shift_displayed_at(list *l, signed char val) {
  signed char i = l->first_displayed_post;
  item *item;

  if (i < 0) {
    i = 0;
  }
  for (; i <= l->last_displayed_post; i++) {
    item = l->displayed_posts[i];
    item->displayed_at -= val;
  }
}

/* limit set to 1 when scrolling up */
static void print_list(list *l, signed char limit) {
  char i, full;
  char bottom = 0;
  signed char first;
  char n_posts;
  item *disp;

update:
  /* set scrollwindow */
  set_hscrollwindow(RIGHT_COL_START, scrw - RIGHT_COL_START);
  gotoxy(0, 0);

  writable_lines = scrh;

  /* copy to temp vars to avoid pointer arithmetic */
  if (l->half_displayed_post > 0 && limit == 0) {
    first = l->half_displayed_post;
    disp = (item *)l->displayed_posts[first];
    if (disp->displayed_at >= 0) {
      gotoxy(0, disp->displayed_at);
      writable_lines = scrh - disp->displayed_at;
    } else {
      first = l->first_displayed_post;
    }
  } else {
    first = l->first_displayed_post;
  }
  n_posts = l->n_posts;

  l->half_displayed_post = 0;

  if (l->account && first == -1) {
    bottom = print_account(l->account);
    l->account_height = wherey();
  }

  for (i = max(0, first); i < n_posts; i++) {
    if (kbhit() && limit == 0) {
      break;
    }
    full = (l->root && !strcmp(l->root, l->ids[i]));
    disp = (item *)l->displayed_posts[i];
    if (disp == NULL) {
      if (i > first && writable_lines != 0) {
        dputs(LOADING_TOOT_MSG);
        gotox(0);
      }

      disp = item_get(l, i, full);
      l->displayed_posts[i] = disp;

      if (i > first && writable_lines != 0) {
        dputs(CLEAR_LOAD_MSG);
        gotox(0);
      }
    }
    if (disp == NULL) {
      dputs("Load error :(");
      if (--writable_lines != 0) {
        clrnln();
        if (--writable_lines != 0)
          chline(scrw - RIGHT_COL_START);
      } else {
        bottom = 1;
      }
      l->post_height[i] = 2;
      continue;
    }
    if (bottom == 0) {
      if (l->kind != SHOW_NOTIFICATIONS) {
        bottom = print_status((status *)disp, hide_cw || wherey() > 0, full);
      } else {
        bottom = print_notification((notification *)disp);
      }
      if (disp->displayed_at == 0 && wherey() == 0) {
        /* Specific case of a fullscreen, but not overflowing, post */
        l->post_height[i] = scrh;
      } else {
        l->post_height[i] = wherey() - disp->displayed_at;
      }
      l->last_displayed_post = i;
    }
    if (bottom) {
      l->half_displayed_post = i;
      break;
    }
    /* Let's see if we reached the limit
     * Hack: passing limit = 0 means no
     * limit because of thise comparison
     */
    if (--limit == 0) {
      break;
    }
  }

  if (bottom == 0 && i == n_posts) {
    if (load_next_posts(l) > 0)
      goto update;
  }
  set_hscrollwindow(0, scrw);
}

static void shift_posts_down(list *l) {
  char scroll_val;

  if (l->first_displayed_post == l->n_posts)
    return;

  if (l->first_displayed_post == -1) {
    scroll_val = l->account_height;
  } else {
    scroll_val = l->post_height[l->first_displayed_post];
  }

  if (scroll_val == scrh - 1) {
    /* we may have returned before the end */
    scroll_val = scrh;
  }

  /* recompute displayed_at */
  shift_displayed_at(l, scroll_val);

  l->first_displayed_post++;
  set_hscrollwindow(RIGHT_COL_START, scrw - RIGHT_COL_START);
  scrollup_n(scroll_val);

#ifndef __CC65__
  clrscr();
#endif

  set_hscrollwindow(0, scrw);
}

static char calc_post_height(status *s) {
  char height;
  register char *w;
  char x;

  w = s->content;

  height = 6; /* header(username + date + display_name) + one line content + footer(\r\n + stats + line)*/
  if (s->reblogged_by) {
    ++height;
  }
  if (s->spoiler_text) {
    ++height;
  }
  if (s->poll) {
    height += 3 * s->poll->options_count;
  }

  x = 0;
  while (*w) {
    if (*w == '\n') {
      x = 0;
      ++height;
    } else {
      ++x;
      if (x == scrw - RIGHT_COL_START) {
        x = 0;
        ++height;
      }
    }
    ++w;
  }
  return height;
}

static int shift_posts_up(list *l) {
  signed char scroll_val;
  signed char first;

  l->half_displayed_post = 0;
  first = l->first_displayed_post;
  if (l->account && first == -1) {
      return -1;
  } else if (!l->account && l->kind != SHOW_PROFILE && first == 0) {
      load_prev_posts(l);
      first = l->first_displayed_post;
      if (first == 0)
        return -1;
  }

  l->eof = 0;

  l->first_displayed_post = --first;

  if (first == -1) {
    scroll_val = l->account_height;
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
    scroll_val = l->post_height[first];
  }
  if (scroll_val < 0) {
    scroll_val = scrh;
  }
  if (scroll_val == scrh - 1) {
    /* we may have returned before the end */
    scroll_val = scrh;
  }

  set_hscrollwindow(RIGHT_COL_START, scrw - RIGHT_COL_START);
  if (scroll_val > 0)
    scrolldown_n(scroll_val);

  /* recompute displayed_at */
  shift_displayed_at(l, -scroll_val);

#ifndef __CC65__
  clrscr();
#endif

  set_hscrollwindow(0, scrw);
  return 0;
}

#ifdef __APPLE2ENH__
#define STATE_FILE "/RAM/mastostate"
#else
#define STATE_FILE "mastostate"
#endif

static void save_state(void) {
  char i,j;
  FILE *fp;

#ifdef __APPLE2__
  _filetype = PRODOS_T_TXT;
#endif

  clrscr();
  dputs("Saving state...");

  fp = fopen(STATE_FILE,"w");
  if (fp == NULL) {
    return;
  }

  if (fprintf(fp, "%d\n", cur_list_idx) < 0)
    goto err_out;

  for (i = 0; i <= cur_list_idx; i++) {
    list *l;
    l = all_lists[i];
    if (fprintf(fp, "%d\n"
                    "%s\n"
                    "%s\n"
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
  dputs(" Done.");
  return;

err_out:
  fclose(fp);
  unlink(STATE_FILE);
  dputs("Error.\n");
}

static void launch_command(char *command, char *p1, char *p2, char *p3) {
  char *params;

  save_state();

  reopen_start_device();

  params = malloc0(127);
  snprintf(params, 127, "%s %s %s %s %s %s",
            instance_url, oauth_token,
            translit_charset, p1?p1:"", p2?p2:"", p3?p3:"");
#ifdef __CC65__
  #ifdef __APPLE2__
  _filetype = PRODOS_T_TXT;
  #endif
  if (exec(command, params) != 0) {
    cprintf("\r\nError %d starting %s %s\r\n", errno, command, params);
    cgetc();
    clrscr();
  }
  free(params);
#else
  printf("exec(%s %s)\n",command, params);
  exit(0);
#endif
}

#ifdef __CC65__
#pragma code-name (push, "RT_ONCE")
#endif

static int state_get_int(FILE *fp) {
  /* coverity[tainted_argument] */
  fgets(gen_buf, BUF_SIZE, fp);
  return atoi(gen_buf);
}

static char *state_get_str(FILE *fp) {
  /* coverity[tainted_argument] */
  fgets(gen_buf, BUF_SIZE, fp);
  if (gen_buf[0] != '\n') {
    *strchr(gen_buf, '\n') = '\0';
    return strdup(gen_buf);
  }
  return NULL;
}

static int load_state(list ***lists) {
  char i,j, loaded;
  signed char num_lists;
  FILE *fp;

#ifdef __APPLE2__
  _filetype = PRODOS_T_TXT;
#endif

  *lists = NULL;
  fp = fopen(STATE_FILE, "r");
  if (fp == NULL) {
    *lists = NULL;
    return -1;
  }

  loaded = -1;

  dputs("Reloading state...");

  num_lists = state_get_int(fp);
  if (num_lists < 0) {
    *lists = NULL;
    cprintf("Error %d\r\n", errno);
    fclose(fp);
    unlink(STATE_FILE);

    return -1;
  }

  *lists = malloc0((num_lists + 1) * sizeof(list *));

  for (i = 0; i <= num_lists; i++) {
    list *l;
    char n_posts;

    l = malloc0(sizeof(list));
    (*lists)[i] = l;

    l->kind = state_get_int(fp);
    l->root = state_get_str(fp);
    l->leaf_root = state_get_str(fp);
    l->last_displayed_post = state_get_int(fp);
    l->eof = state_get_int(fp);
    l->first_displayed_post = state_get_int(fp);
    l->n_posts = state_get_int(fp);

    n_posts = l->n_posts;

    memset(l->displayed_posts, 0, N_STATUS_TO_LOAD * sizeof(status *));

    /* coverity[tainted_argument] */
    fgets(gen_buf, BUF_SIZE, fp);
    if (gen_buf[0] != '\n') {
      *strchr(gen_buf, '\n') = '\0';
      l->account = api_get_full_account(gen_buf);
    }

    l->account_height = state_get_int(fp);

    for (j = 0; j < n_posts; j++) {
      l->ids[j] = state_get_str(fp);
      l->post_height[j] = state_get_int(fp);

      if (i == num_lists && l->root && !strcmp(l->root, l->ids[j])) {
        l->displayed_posts[j] = item_get(l, j, 1);
        loaded = j;
      }
    }
    if (i == num_lists && loaded != l->first_displayed_post
        && l->first_displayed_post > -1) {
      l->displayed_posts[l->first_displayed_post] =
          item_get(l, l->first_displayed_post, 1);
    }
  }

  fclose(fp);
  unlink(STATE_FILE);
  cur_action = NAVIGATE;

  dputs(" Done");

  return num_lists;
}

#ifdef __CC65__
#pragma code-name (pop)
#endif

static char background_load(list *l) {
  char i;
  for (i = 0; i < l->n_posts; i++) {
    if (l->displayed_posts[i] == NULL) {
      load_indicator(1);

      l->displayed_posts[i] = item_get(l, i, 0);

      load_indicator(0);
      return 0; /* background load one by one to check kb */
    }
  }
  return -1;
}

static void do_vote (status *status) {
  char c;

  set_hscrollwindow(RIGHT_COL_START, scrw - RIGHT_COL_START);

  while (1) {
    gotoxy(0, 0);
    writable_lines = 23;
    if (print_status(status, 0, 1) == 0) {
      c = wherey();
      c -= 2;
    } else {
      c = scrh - 1;
    }
    clrzone(0, c, scrw - RIGHT_COL_START - 1, c);
#if NUMCOLS == 80
    dputs("1-4 to choose, Enter to vote, Escape to cancel");
#else
    dputs("1-4: choose, Enter: vote, Esc: cancel");
#endif
    c = tolower(cgetc());

    switch(c) {
      case CH_ESC:
        goto out;
      case CH_ENTER:
        goto vote;
      case '1':
      case '2':
      case '3':
      case '4':
        if (!status->poll->multiple) {
          memset(status->poll->own_votes, 0, MAX_POLL_OPTIONS);
        }
        status->poll->own_votes[c-'1'] = !status->poll->own_votes[c-'1'];
        break;
    }
  }
vote:
  poll_update_vote(status->poll);
  /* update display */
  gotoxy(0, 0);
  writable_lines = 23;
  print_status(status, 0, 1);

out:
  set_hscrollwindow(0, scrw);
  return;
}

static void show_list(list *l) {
  char c;
  char limit = 0;

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

    load_indicator(1);
    print_list(l, limit);
    limit = 0;
    load_indicator(0);

    while (!kbhit() && background_load(l) == 0) {
      /* keep loading */
    }
#if NUMCOLS == 80
    print_free_ram();
#endif

    c = tolower(cgetc());

#if NUMCOLS == 40
inject_cmd:
#endif

    switch(c) {
#ifdef __APPLE2ENH__
      case CH_CURS_DOWN:
#else
      case 'j':
#endif
        hide_cw = 1;
        shift_posts_down(l);
        break;
#ifdef __APPLE2ENH__
      case CH_CURS_UP:
#else
      case 'u':
#endif
        hide_cw = 1;
        shift_posts_up(l);
        limit = 1; /* only print one */
        break;
      case 'w':
        hide_cw = !hide_cw;
        limit = 1; /* print the first one */
        break;
      case 'f':
        if (root_status) {
          api_favourite_status(root_status);
          l->half_displayed_post = 0;
        } else if (l->account) {
          cur_action = ACCOUNT_TOGGLE_RSHIP;
          rship_toggle_action = RSHIP_FOLLOWING;
          return;
        }
        break;
      case 'b':
        if (root_status) {
          api_reblog_status(root_status);
          l->half_displayed_post = 0;
        } else if (l->account) {
          cur_action = ACCOUNT_TOGGLE_RSHIP;
          rship_toggle_action = RSHIP_BLOCKING;
          return;
        }
        break;
      case 'm':
        if (root_status) {
          api_bookmark_status(root_status);
          l->half_displayed_post = 0;
        } else if (l->account) {
          cur_action = ACCOUNT_TOGGLE_RSHIP;
          rship_toggle_action = RSHIP_MUTING;
          return;
        } else if (l->kind == SHOW_NOTIFICATIONS) {
          notifications_type = NOTIFICATION_MENTION;
          cur_action = SHOW_NOTIFICATIONS;
          return;
        }
        break;
      case 'd':
        if (root_status) {
          if (api_delete_status(root_status) == 0) {
            cur_action = BACK;
            return;
          }
        }
        break;
      case 'v':
        if (root_status && root_status->poll) {
          cur_action = VOTING;
          do_vote(root_status);
          l->half_displayed_post = 0;
          cur_action = NAVIGATE;
          return;
        }
        break;
      case 'a':
        if (l->kind == SHOW_NOTIFICATIONS) {
          notifications_type = NOTIFICATION_FAVOURITE;
          cur_action = SHOW_NOTIFICATIONS;
          return;
        }
        break;
      case 'h':
        cur_action = SHOW_HOME_TIMELINE;
        return;
      case 'l':
        cur_action = SHOW_LOCAL_TIMELINE;
        return;
      case 'g':
        cur_action = SHOW_GLOBAL_TIMELINE;
        return;
      case 'k':
        cur_action = SHOW_BOOKMARKS;
        return;
      case CH_ENTER: /* SHOW_FULL_STATUS */
      case CH_ESC:   /* BACK */
      case 'c':      /* COMPOSE */
      case 'o':      /* CONFIGURE */
      case 'n':      /* SHOW_NOTIFICATIONS */
      case 's':      /* SEARCH */
      case 'p':      /* SHOW_PROFILE */
      case 'i':      /* IMAGES */
      case 'r':      /* REPLY */
      case 'e':      /* EDIT */
        cur_action = c;
        return;
#if NUMCOLS == 40
      case SHOW_HELP:
        clrscr();
        show_help(l, root_status, root_notif);
        c = cgetc();
        clrscr();
        l->half_displayed_post = 0;
        if (c == HELP_KEY) {
          cur_action = NAVIGATE;
          return;
        } else {
          goto inject_cmd;
        }
#endif
    }
  }
}

static void push_list(void) {
  ++cur_list_idx;
  if (cur_list_idx > 0) {
    compact_list(all_lists[cur_list_idx - 1]);
  }
  all_lists = realloc(all_lists, (cur_list_idx + 1) * sizeof(list *));
}

static void pop_list(void) {
  if (cur_list_idx < 0)
    return;

  free_list(all_lists[cur_list_idx]);
  --cur_list_idx;

  all_lists = realloc(all_lists, (cur_list_idx + 1) * sizeof(list *));
  if (cur_list_idx >= 0) {
    uncompact_list(all_lists[cur_list_idx]);
  }
}

static void disconnect_account (void) {
  unlink("mastsettings");
}

static void cli(void) {
  item *disp;
  status *disp_status;
  notification *disp_notif;
  list *current_list;

  /* static buffer because we need to copy the
   * IDs before compacting parent list, to avoid
   * memory fragmentation */
  char new_root[32], new_leaf_root[32];

  if (surl_connect_proxy() != 0) {
    dputs("Can not connect serial proxy.");
    cgetc();
    exit(1);
  }

  cur_list_idx = load_state(&all_lists);

  /* Get rid of init code */
  runtime_once_clean();

  clrscr();

  if (print_header(NULL, NULL, NULL) != 0) {
    int code = surl_response_code();
    clrscr();
    cprintf("Error fetching data: code %d\r\n\r\n", code);
    if (code == 401) {
      disconnect_account();
      dputs("Your token is invalid. ");
    }
    dputs("Please try to login again.");
    cgetc();
#ifdef __CC65__
    exec("mastodon", NULL);
#endif
  }

  if (cur_list_idx == -1) {
    cur_action = SHOW_HOME_TIMELINE;
  }

  while (1) {
    if (cur_list_idx == -1) {
      current_list = NULL;
      disp_status = NULL;
    } else {
      current_list = all_lists[cur_list_idx];
      disp_status = get_top_status(current_list);
    }
    switch(cur_action) {
      case SHOW_HOME_TIMELINE:
      case SHOW_LOCAL_TIMELINE:
      case SHOW_GLOBAL_TIMELINE:
      case SHOW_BOOKMARKS:
        /* Create a new list if it's the first of different than
         * the current one. Otherwise, overwrite the current one.
         */
        if (cur_list_idx < 0 || current_list->kind != cur_action) {
          push_list();
        } else {
          /* hack, reusing current_list */
          free_list(all_lists[cur_list_idx]);
        }
        all_lists[cur_list_idx] = build_list(NULL, NULL, cur_action);
        current_list = all_lists[cur_list_idx];

        clrscrollwin();
        cur_action = NAVIGATE;
        break;
      case ACCOUNT_TOGGLE_RSHIP:
        account_toggle_rship(current_list->account, rship_toggle_action);
        cur_action = SHOW_PROFILE;
        /* and fallthrough to reuse list (we're obviously already
         * in a SHOW_PROFILE list)*/
      case SHOW_SEARCH_RES:
      case SHOW_NOTIFICATIONS:
        if (current_list->kind == cur_action) {
          free_list(all_lists[cur_list_idx]);
          goto navigate_reuse_list;
        }
      case SHOW_PROFILE:
      case SHOW_FULL_STATUS:
        *new_root = '\0';
        *new_leaf_root = '\0';
        disp = current_list->displayed_posts[current_list->first_displayed_post];
        if (current_list->kind != SHOW_NOTIFICATIONS) {
          disp_status = (status *)disp;
          if (cur_action == SHOW_FULL_STATUS) {
            strcpy(new_root, disp_status->reblog_id ? disp_status->reblog_id : disp_status->id);
            strcpy(new_leaf_root, disp_status->id);
          } else if (cur_action == SHOW_PROFILE) {
            strcpy(new_root, disp_status->account->id);
          }
        } else {
          disp_notif = (notification *)disp;
          if (cur_action == SHOW_FULL_STATUS && disp_notif->status_id) {
            strcpy(new_root, disp_notif->status_id);
            strcpy(new_leaf_root, disp_notif->status_id);
          } else {
            cur_action = SHOW_PROFILE;
            strcpy(new_root, disp_notif->account_id);
          }
        }
navigate_new_list:
        push_list();
navigate_reuse_list:
        all_lists[cur_list_idx] = build_list(new_root, new_leaf_root, cur_action);
        current_list = all_lists[cur_list_idx];
        clrscrollwin();
        /*
        cur_action = NAVIGATE;
        break;
        */
      case NAVIGATE:
        show_list(current_list);
        break;
      case BACK:
        pop_list();
        clrscrollwin();
        if (cur_list_idx >= 0) {
          cur_action = NAVIGATE;
        } else {
          cur_action = SHOW_HOME_TIMELINE;
        }
        break;
      case CONFIGURE:
          launch_command("mastodon", "conf", NULL, NULL);
          /* we're never coming back */
      case COMPOSE:
          if (current_list->account)
            launch_command("mastowrite", current_list->account->acct, NULL, NULL);
          else
            launch_command("mastowrite", NULL, NULL, NULL);
          /* we're never coming back */
      case REPLY:
          if (disp_status)
            launch_command("mastowrite", "r", disp_status->id, NULL);
          cur_action = NAVIGATE;
          break;
      case EDIT:
          if (disp_status && !strcmp(disp_status->account->id, my_account->id))
            launch_command("mastowrite", "e", disp_status->id, NULL);
          cur_action = NAVIGATE;
          break;
      case IMAGES:
          if (current_list->account && (!disp_status || disp_status->displayed_at > 0)) {
            launch_command("mastoimg", monochrome?"1":"0", "a", current_list->account->id);
          } else if (disp_status && disp_status->n_medias) {
            launch_command("mastoimg", monochrome?"1":"0", "s", disp_status->id);
          }
          cur_action = NAVIGATE;
          break;
      case SEARCH:
          if (show_search()) {
            if (search_type == 'm') {
              cur_action = SHOW_SEARCH_RES;
            } else {
              char **acc_id = malloc0(sizeof(char *));
              int loaded = api_search(1, search_buf, 'a', NULL, NULL, acc_id);
              if (loaded > 0) {
                strcpy(new_root, acc_id[0]);
                free(acc_id[0]);
                free(acc_id);
                cur_action = SHOW_PROFILE;
                goto navigate_new_list;
              }
              free(acc_id);
            }
          } else {
            cur_action = NAVIGATE;
          }
          break;
    }
  }
  /* we can't return to main(), it has been
   * garbage-collected */
  exit(0);
}

#ifdef __CC65__
#pragma code-name (push, "RT_ONCE")
#endif

int main(int argc, char **argv) {
  FILE *fp;

  if (argc < 3) {
    dputs("Missing parameters.\r\n");
  }

  register_start_device();

#ifdef __APPLE2ENH__
  videomode(VIDEOMODE_80COL);
#endif

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
#ifdef __APPLE2ENH__
    translit_charset = strdup(gen_buf);
#endif

    fgets(gen_buf, 16, fp);
    if (gen_buf[0] == '0') {
      monochrome = 0;
    }

    fclose(fp);
  }

//  0 2    7
// "US-ASCII"
// "ISO646-IT"  Nothing to do, § for @ and £ for #
// "ISO646-DK"  
// "ISO646-UK"  Nothing to do, @ for @ and £ for #
// "ISO646-DE"  Nothing to do, § for @ and # for #
// "ISO646-SE2"
// "ISO646-FR1" Change @ to 0x5D for § and £ for #
// "ISO646-ES"  Nothing to do, § for @ and £ for #

  switch(translit_charset[7]) {
    case 'F':
      arobase = 0x5D;
      break;
  }

  cli();
  return 0;
}

#ifdef __CC65__
#pragma code-name (pop)
#endif

#pragma register-vars(pop)
