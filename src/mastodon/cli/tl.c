#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
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
#include "dget_text.h"
#include "scrollwindow.h"
#include "runtime_once_clean.h"
#include "a2_features.h"

#ifdef __CC65__
#pragma code-name (push, "LC")
#endif

#pragma register-vars(push, on)

char *key_combo = "Ctrl";

static char *tl_endpoints[4] = { TIMELINE_ENDPOINT "/" HOME_TIMELINE,
                                  TIMELINE_ENDPOINT "/" PUBLIC_TIMELINE,
                                  TIMELINE_ENDPOINT "/" PUBLIC_TIMELINE,
                                  BOOKMARKS_ENDPOINT};
static char *tl_filter[4] = { NULL,
                               "&local=true",
                               NULL,
                               NULL};

char *instance_url = NULL;
char *oauth_token = NULL;
char writable_lines = 24;

extern account *my_account;

static list **all_lists;
static signed char cur_list_idx;

char cur_action;
static char search_buf[50];
static char search_type = 't';
static char rship_toggle_action = RSHIP_FOLLOWING;
static char notifications_type = NOTIFICATION_FAVOURITE;
static void print_list(list *l, signed char limit);

signed char half_displayed_post;
char last_displayed_post;

static item *get_top_item(list *l) {
  signed char first = l->first_displayed_post;

  if (first >= 0 && first < l->n_posts) {
    return l->displayed_posts[first];
  }
  return NULL;
}

static int print_account(account *a) {
  char i, y;
  writable_lines--;
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

  y = 0;

  if (api_relationship_get(a, RSHIP_FOLLOWING)) {
    if (has_80cols) {
      gotoxy(32, y);
    }
    dputs("             You follow them\r\n");
  } else if (api_relationship_get(a, RSHIP_FOLLOW_REQ)) {
    if (has_80cols) {
      gotoxy(32, ++y);
    }
    dputs("You requested to follow them\r\n");
  }
  if (api_relationship_get(a, RSHIP_FOLLOWED_BY)) {
    if (has_80cols) {
      gotoxy(32, ++y);
    }
    dputs("             They follow you\r\n");
  }

  if (wherey() < 4) {
    gotoy(4);
    CHECK_NO_CRLF();
  }

  for (i = 0; i < a->n_fields; i++) {
    CHECK_AND_CRLF();
    print_buf(a->fields[i], 0, 0);
  }

  CHECK_AND_CRLF();
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

  width = NUMCOLS - RIGHT_COL_START;
  n->displayed_at = wherey();
  dputs(n->display_name);
  if (has_80cols) {
    gotox(TIME_COLUMN);
    if (writable_lines != 1)
      dputs(n->created_at);
    else
      cputs(n->created_at); /* no scrolling please */
    CHECK_NO_CRLF();
  } else {
    CHECK_AND_CRLF();
  }

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

#ifdef __CC65__
#pragma code-name (pop)
#endif

static void item_free(list *l, char i) {
  item *to_free = l->displayed_posts[i];

  switch(l->kind) {
    case SHOW_NOTIFICATIONS:
      notification_free((notification *)to_free);
      break;
    default:
      status_free((status *)to_free);
  }
  l->displayed_posts[i] = NULL;
}

static item *item_get(list *l, char i, char full) {
  char *id = l->ids[i];
  switch(l->kind) {
    case SHOW_NOTIFICATIONS:
      return (item *)api_get_notification(id);
    default:
      return (item *)api_get_status(id, full);
  }
}

static char load_around(list *l, char to_load, char *first, char *last, char **new_ids) {
  char loaded;
  half_displayed_post = 0;
  if (l->kind == SHOW_BOOKMARKS && (IS_NOT_NULL(first) || IS_NOT_NULL(last))) {
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

static void set_list_hscrollwindow(void) {
  set_hscrollwindow(RIGHT_COL_START, NUMCOLS - RIGHT_COL_START);
}

static void set_full_hscrollwindow(void) {
  set_hscrollwindow(0, NUMCOLS);
}

static char is_root(list *l, char idx) {
  return !strcmp(l->root, l->ids[idx]);
}

static item *load_item_at(list *l, char idx, char full) {
  return (l->displayed_posts[idx] = item_get(l, idx, full));
}

static char load_prev_posts(list *l) {
  char *first_id;
  char **new_ids;
  signed char loaded, i;
  char to_load, list_len;

  to_load = N_STATUS_TO_LOAD / 2;
  new_ids = malloc0(to_load * sizeof(char *));

  set_list_hscrollwindow();
  scrolldown_n(2);
  gotoxy(0,1);
  chline(NUMCOLS - RIGHT_COL_START);
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
    load_item_at(l, i, is_root(l, i));

    l->first_displayed_post += loaded;
  }
  scrollup_n(2);
  set_full_hscrollwindow();

  free(new_ids);
  return loaded;
}

static char search_footer(char c) {
  c = tolower(c);
  switch (c) {
    case 't':
    case 'a':
      search_type = c;
      break;
    default:
      break;
  }
  gotoxy(0,1);
  cprintf("(%c) Toots (%c) Account (%s + T/A)\r\n",
          search_type == 't' ? '*':' ',
          search_type == 'a' ? '*':' ',
          key_combo);
  chline(NUMCOLS - RIGHT_COL_START);
  gotoxy(0, 0);
  return 0;
}

static int show_search(void) {
  unsigned char r = 0;
  set_list_hscrollwindow();

  scrolldown_n(3);

  search_footer(search_type);
  dputs("Search: ");

  search_buf[0] = '\0';
  dget_text_single(search_buf, 49, search_footer);

  if (search_buf[0] != '\0') {
    clrscr();
    r = 1;
    goto out;
  }

  scrollup_n(3);

out:
  set_full_hscrollwindow();
  return r;
}

static void clear_displayed_posts(list *l) {
  memset(l->post_height, -1, N_STATUS_TO_LOAD);
}
/* root is either an account or status id, depending on kind.
 * leaf_root is a reblogged status id
 */
static list *build_list(char *root, char *leaf_root, char kind) {
  list *l;
  char i, found_root;
  char n_posts;

  l = malloc0(sizeof(list));

  if (kind == SHOW_PROFILE) {
    id_copy(l->root, root);
    l->account = api_get_full_account(root);
    l->first_displayed_post = -1;
  } else {
    if (IS_NOT_NULL(root) && *root) {
      id_copy(l->root, root);
      id_copy(l->leaf_root, leaf_root);
    }
  }

  l->kind = kind;

  n_posts = load_around(l, N_STATUS_TO_LOAD, NULL, NULL, l->ids);

  clear_displayed_posts(l);

  found_root = 0;
  if (IS_NOT_NULL(root)) {
    for (i = 0; i < n_posts; i++) {
      if (is_root(l, i)) {
          /* Load first for the header */
          if (IS_NOT_NULL(load_item_at(l, i, 1))) {
            l->first_displayed_post = i;
            found_root = 1;
          }
          break;
      }
    }
  }
  if (n_posts > 0 && !found_root) {
    load_item_at(l, 0, 0);
  }
  l->n_posts = n_posts;

  return l;
}

static void free_list(list *l) {
  char i;
  for (i = 0; i < l->n_posts; i++) {
    free(l->ids[i]);
    item_free(l, i);
  }
  account_free(l->account);
  free(l);
}

static void compact_list(list *l) {
  char i;
  for (i = 0; i < l->n_posts; i++) {
    item_free(l, i);
  }
}

static void uncompact_list(list *l) {
  signed char first, full;
  first = l->first_displayed_post;
  half_displayed_post = 0;
  if (first >= 0) {
    full = is_root(l, first);
    l->displayed_posts[first] =
      item_get(l, first, full);
  }
}

static void clrscrollwin(void) {
  set_list_hscrollwindow();
  clrscr();
  set_full_hscrollwindow();
}

char hide_cw = 1;

static void shift_displayed_at(list *l, signed char val) {
  signed char i = l->first_displayed_post;
  item *item;

  if (i < 0) {
    i = 0;
  }
  for (; i <= last_displayed_post; i++) {
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
  set_list_hscrollwindow();
  gotoxy(0, 0);

  writable_lines = NUMLINES;

  /* copy to temp vars to avoid pointer arithmetic */
  if (half_displayed_post > 0 && limit == 0) {
    first = half_displayed_post;
    disp = (item *)l->displayed_posts[first];
    if (disp->displayed_at >= 0) {
      gotoxy(0, disp->displayed_at);
      writable_lines = NUMLINES - disp->displayed_at;
    } else {
      first = l->first_displayed_post;
    }
  } else {
    first = l->first_displayed_post;
  }
  n_posts = l->n_posts;

  half_displayed_post = 0;

  if (IS_NOT_NULL(l->account) && first == -1) {
    bottom = print_account(l->account);
    l->account_height = wherey();
  }

  for (i = max(0, first); i < n_posts; i++) {
    if (kbhit() && limit == 0) {
      break;
    }
    full = is_root(l, i);
    disp = (item *)l->displayed_posts[i];
    if (IS_NULL(disp)) {
      if (i > first && writable_lines != 0) {
        dputs(LOADING_TOOT_MSG);
        gotox(0);
      }

      disp = load_item_at(l, i, full);

      if (i > first && writable_lines != 0) {
        dputs(CLEAR_LOAD_MSG);
        gotox(0);
      }
    }
    if (IS_NULL(disp)) {
      dputs("Load error :(");
      if (--writable_lines != 0) {
        clrnln();
        if (--writable_lines != 0)
          chline(NUMCOLS - RIGHT_COL_START);
      } else {
        bottom = 1;
      }
      l->post_height[i] = 2;
      continue;
    }
    if (bottom == 0) {
      switch(l->kind) {
        case SHOW_NOTIFICATIONS:
          bottom = print_notification((notification *)disp);
          break;
        default:
          bottom = print_status((status *)disp, hide_cw || wherey() > 0, full);
      }

      if (disp->displayed_at == 0 && wherey() == 0) {
        /* Specific case of a fullscreen, but not overflowing, post */
        l->post_height[i] = NUMLINES;
      } else {
        l->post_height[i] = wherey() - disp->displayed_at;
      }
      last_displayed_post = i;
    }
    if (bottom) {
      half_displayed_post = i;
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
  set_full_hscrollwindow();
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

  if (scroll_val == NUMLINES - 1) {
    /* we may have returned before the end */
    scroll_val = NUMLINES;
  }

  /* recompute displayed_at */
  shift_displayed_at(l, scroll_val);

  l->first_displayed_post++;
  set_list_hscrollwindow();
  scrollup_n(scroll_val);

#ifndef __CC65__
  clrscr();
#endif

  set_full_hscrollwindow();
}

static char calc_post_height(status *s) {
  char height;
  register char *w;
  char x;

  w = s->content;

  height = 6; /* header(username + date + display_name) + one line content + footer(\r\n + stats + line)*/
  if (IS_NOT_NULL(s->reblogged_by)) {
    ++height;
  }
  if (IS_NOT_NULL(s->spoiler_text)) {
    ++height;
  }
  if (IS_NOT_NULL(s->poll)) {
    height += 3 * s->poll->options_count;
  }

  x = 0;
  while (*w) {
    if (*w == '\n') {
      x = 0;
      ++height;
    } else {
      ++x;
      if (x == NUMCOLS - RIGHT_COL_START) {
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

  half_displayed_post = 0;
  first = l->first_displayed_post;
  if (IS_NOT_NULL(l->account) && first == -1) {
      return -1;
  } else if (IS_NULL(l->account) && l->kind != SHOW_PROFILE && first == 0) {
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
    if (IS_NULL(l->displayed_posts[first])) {
      l->displayed_posts[first] =
        item_get(l, first, 0);
    }
    if (l->post_height[first] == -1) {
      switch(l->kind) {
        case SHOW_NOTIFICATIONS:
          l->post_height[first] = 4;
          break;
        default:
          l->post_height[first] =
            calc_post_height((status *)l->displayed_posts[first]);
      }
    }
    scroll_val = l->post_height[first];
  }
  if (scroll_val < 0) {
    scroll_val = NUMLINES;
  }
  if (scroll_val == NUMLINES - 1) {
    /* we may have returned before the end */
    scroll_val = NUMLINES;
  }

  set_list_hscrollwindow();
  if (scroll_val > 0)
    scrolldown_n(scroll_val);

  /* recompute displayed_at */
  shift_displayed_at(l, -scroll_val);

#ifndef __CC65__
  clrscr();
#endif

  set_full_hscrollwindow();
  return 0;
}

static void save_state(void) {
  char i,j;
  FILE *fp;

#ifdef __APPLE2__
  _filetype = PRODOS_T_TXT;
#endif

  clrscr();
  dputs("Saving state...");

  fp = fopen(STATE_FILE,"w");
  if (IS_NULL(fp)) {
    return;
  }

  if (fputc(cur_list_idx, fp) < 0)
    goto err_out;

  for (i = 0; i <= cur_list_idx; i++) {
    list *l;
    l = all_lists[i];
    if (fwrite(l, 1, sizeof(list), fp) < sizeof(list)) {
      goto err_out;
    }

    for (j = 0; j < l->n_posts; j++) {
      if (fprintf(fp, "%s\n", l->ids[j]) < 0) {
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

static void launch_command(char *command, char *p1, char *p2) {
  char *params;

  save_state();

  reopen_start_device();

  params = malloc0(127);
  snprintf(params, 127, "%s %s %d %s %s %s",
            instance_url, oauth_token, monochrome,
            translit_charset, p1?p1:"", p2?p2:"");
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

/* state_buf NOT with the common buffers in buffers.s,
 * as 0x800-0xC00 will be overwritten by file I/O. */
#define STATE_BUF_SIZE 32
static char state_buf[STATE_BUF_SIZE];

static char *state_get_str(FILE *fp) {
  /* coverity[tainted_argument] */
  fgets(state_buf, STATE_BUF_SIZE, fp);
  if (state_buf[0] != '\n') {
    *strchr(state_buf, '\n') = '\0';
    return strdup(state_buf);
  }
  return NULL;
}

static int load_state(list ***lists) {
  signed char i, j, first;
  signed char num_lists;
  FILE *fp;
  list *l = NULL;

#ifdef __APPLE2__
  _filetype = PRODOS_T_TXT;
#endif

  *lists = NULL;
  fp = fopen(STATE_FILE, "r");
  if (IS_NULL(fp)) {
    *lists = NULL;
    return -1;
  }

  dputs("Reloading state...");

  num_lists = fgetc(fp);

  if (num_lists < 0) {
    *lists = NULL;
    cprintf("Error %d\r\n", errno);
    fclose(fp);
    unlink(STATE_FILE);

    return -1;
  }

  *lists = malloc0((num_lists + 1) * sizeof(list *));

  for (i = 0; i <= num_lists; i++) {
    l = malloc0(sizeof(list));
    (*lists)[i] = l;

    fread(l, 1, sizeof(list), fp);

    bzero(l->displayed_posts, N_STATUS_TO_LOAD*sizeof(item *));

    for (j = 0; j < l->n_posts; j++) {
      l->ids[j] = state_get_str(fp);
    }
  }

  fclose(fp);
  unlink(STATE_FILE);

  /* Load the first item on the last list, *AFTER* having closed
   * the file - the file I/O buffer and json buffer are shared.
 */
  if (IS_NOT_NULL(l) && (first = l->first_displayed_post) != -1) {
    load_item_at(l, first, 1);
  }

  /* Load the accounts in all lists, *AFTER* having closed
   * the file - the file I/O buffer and json buffer are shared.
   */
  for (i = 0; i <= num_lists; i++) {
    l = (*lists)[i];
    if (l->kind == SHOW_PROFILE) {
      l->account = api_get_full_account(l->root);
    }
  }

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
    if (IS_NULL(l->displayed_posts[i])) {
      load_item_at(l, i, 0);
      return 0; /* background load one by one to check kb */
    }
  }
  return -1;
}

static void do_vote (status *status) {
  char c;

  set_list_hscrollwindow();

  while (1) {
    gotoxy(0, 0);
    writable_lines = 23;
    if (print_status(status, 0, 1) == 0) {
      c = wherey() - 2;
    } else {
      c = NUMLINES - 1;
    }
    clrzone(0, c, NUMCOLS - RIGHT_COL_START - 1, c);
    dputs("1-7: choose, Enter: vote, Esc: cancel");

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
      case '5':
      case '6':
      case '7':
        if (!status->poll->multiple) {
          /* reset all options */
          bzero(status->poll->own_votes, MAX_POLL_OPTIONS);
        }
        /* toggle current option */
        c -= '1';
        status->poll->own_votes[c] = !status->poll->own_votes[c];
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
  set_full_hscrollwindow();
  return;
}

static void show_list(list *l) {
  char c;
  char limit = 0;

  while (1) {
    status *root_status = NULL;
    notification *root_notif = NULL;

    switch (l->kind) {
      case SHOW_NOTIFICATIONS:
        root_notif = (notification *)get_top_item(l);
        break;
      default:
        root_status = (status *)get_top_item(l);
    }

    print_header(l, root_status, root_notif);

    print_list(l, limit);
    limit = 0;

    while (!kbhit() && background_load(l) == 0) {
      /* keep loading */
    }

    if (has_80cols) {
      print_free_ram();
    }

    c = tolower(cgetc());
inject_cmd:
    switch(c) {
      case CH_CURS_DOWN:
      case 'j':
        hide_cw = 1;
        shift_posts_down(l);
        break;
      case CH_CURS_UP:
      case 'u':
        hide_cw = 1;
        shift_posts_up(l);
        limit = 1; /* only print one */
        break;
      case 'w':
        hide_cw = !hide_cw;
        limit = 1; /* print the first one */
        break;
      case FAV: /* Or FOLLOW, depends */
        if (IS_NOT_NULL(root_status)) {
          api_favourite_status(root_status);
          half_displayed_post = 0;
        } else if (IS_NOT_NULL(l->account)) {
          cur_action = ACCOUNT_TOGGLE_RSHIP;
          rship_toggle_action = RSHIP_FOLLOWING;
          return;
        }
        break;
      case BOOST: /* Or BLOCK, depends */
        if (IS_NOT_NULL(root_status)) {
          api_reblog_status(root_status);
          half_displayed_post = 0;
        } else if (IS_NOT_NULL(l->account)) {
          cur_action = ACCOUNT_TOGGLE_RSHIP;
          rship_toggle_action = RSHIP_BLOCKING;
          return;
        }
        break;
      case BOOKMARK:
        if (IS_NOT_NULL(root_status)) {
          api_bookmark_status(root_status);
          half_displayed_post = 0;
        } else if (IS_NOT_NULL(l->account)) {
          cur_action = ACCOUNT_TOGGLE_RSHIP;
          rship_toggle_action = RSHIP_MUTING;
          return;
        } else if (l->kind == SHOW_NOTIFICATIONS) {
          notifications_type = NOTIFICATION_MENTION;
          cur_action = SHOW_NOTIFICATIONS;
          return;
        }
        break;
      case DELETE:
        if (IS_NOT_NULL(root_status)) {
          if (api_delete_status(root_status) == 0) {
            cur_action = BACK;
            return;
          }
        }
        break;
      case VOTING:
        if (IS_NOT_NULL(root_status) && IS_NOT_NULL(root_status->poll)) {
          cur_action = VOTING;
          do_vote(root_status);
          half_displayed_post = 0;
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
      /* Special cases with mapped arrays */
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
      case SHOW_QUOTE:
        if (IS_NULL(root_status) || IS_NULL(root_status->quote)) {
          cur_action = NAVIGATE;
          return;
        } /* else fallthrough */
      /* Simple cases: */
      case CH_ENTER:
      case CH_ESC:
      case COMPOSE:
      case CONFIGURE:
      case SHOW_NOTIFICATIONS:
      case SEARCH:
      case SHOW_PROFILE:
      case IMAGES:
      case REPLY:
      case EDIT:
        cur_action = c;
        return;
      case 'q':      /* QUIT */
        exit(0);
      default:
        if (!has_80cols && c == SHOW_HELP) {
          clrscr();
          show_help(l, root_status, root_notif);
          c = tolower(cgetc());
          clrscr();
          half_displayed_post = 0;
          if (c == SHOW_HELP) {
            cur_action = NAVIGATE;
            return;
          } else {
            goto inject_cmd;
          }
        }
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
      disp_status = (status *)get_top_item(current_list);
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
        /* fallthrough to reuse list (we're obviously already
         * in a SHOW_PROFILE list)*/
      case SHOW_SEARCH_RES:
      case SHOW_NOTIFICATIONS:
        if (current_list->kind == cur_action) {
          free_list(all_lists[cur_list_idx]);
          goto navigate_reuse_list;
        }
      case SHOW_PROFILE:
      case SHOW_FULL_STATUS:
      case SHOW_QUOTE:
        *new_root = '\0';
        *new_leaf_root = '\0';
        disp = current_list->displayed_posts[current_list->first_displayed_post];
        /* Confusing case here. We're in a list that is either notifications or normal.
         * We will display a list that could be one of:
         * - SHOW_PROFILE
         * - SHOW_SEARCH_RES
         * - SHOW_NOTIFICATIONS
         * - SHOW_FULL_STATUS
         * We check the current list type (NOTIFICATIONS or something else) to know
         * which kind of object we need to access; then, we check cur_action to know
         * which ID to copy from that object.
         */
        if (current_list->kind != SHOW_NOTIFICATIONS) {
          disp_status = (status *)disp;
          if (cur_action == SHOW_QUOTE && IS_NOT_NULL(disp_status->quote)) {
            disp_status = disp_status->quote;
            cur_action = SHOW_FULL_STATUS;
          }
          if (cur_action == SHOW_FULL_STATUS) {
            strcpy(new_root, disp_status->reblog_id[0] ? disp_status->reblog_id : disp_status->id);
            strcpy(new_leaf_root, disp_status->id);
          } else if (cur_action == SHOW_PROFILE) {
            strcpy(new_root, disp_status->account->id);
          }
        } else {
          disp_notif = (notification *)disp;
          if (cur_action == SHOW_FULL_STATUS && disp_notif->status_id[0]) {
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
          launch_command("mastodon", "conf", NULL);
          /* we're never coming back */
      case COMPOSE:
          launch_command("mastowrite", IS_NOT_NULL(current_list->account) ? current_list->account->acct : NULL, NULL);
          /* we're never coming back */
      case REPLY:
          if (IS_NOT_NULL(disp_status))
            launch_command("mastowrite", "r", disp_status->id);
          cur_action = NAVIGATE;
          break;
      case EDIT:
          if (IS_NOT_NULL(disp_status) && !strcmp(disp_status->account->id, my_account->id))
            launch_command("mastowrite", "e", disp_status->id);
          cur_action = NAVIGATE;
          break;
      case IMAGES:
          if (IS_NOT_NULL(current_list->account) && (!disp_status || disp_status->displayed_at > 0)) {
            launch_command("mastoimg", "a", current_list->account->id);
          } else if (IS_NOT_NULL(disp_status) && disp_status->n_medias) {
            launch_command("mastoimg", "s", disp_status->id);
          }
          cur_action = NAVIGATE;
          break;
      case SEARCH:
          if (show_search()) {
            if (search_type == 't') {
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
  /* unreachable */
}

#ifdef __CC65__
#pragma code-name (push, "RT_ONCE")
#endif

void initconst(void);

int main(int argc, char **argv) {
  if (argc < 5) {
    dputs("Missing parameters.\r\n");
    cgetc();
  }

  initconst();
  register_start_device();

  if (has_80cols) {
    serial_throbber_set((void *)0x07D8);
  }

  if (is_iie) {
    key_combo = "Open-Apple";
  }

  instance_url     = argv[1];
  oauth_token      = argv[2];
  monochrome       = (argv[3][0] == '1');
  translit_charset = argv[4];

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
