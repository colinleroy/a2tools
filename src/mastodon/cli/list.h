#ifndef __list_h
#define __list_h

#include <string.h>
#include "common.h"
#include "cli.h"
#include "api.h"

typedef struct _list list;

struct _list {
  char kind;
  char root[SNOWFLAKE_ID_LEN]; /* For timelines */
  char leaf_root[SNOWFLAKE_ID_LEN];
  account *account; /* For accounts */
  char *ids[N_STATUS_TO_LOAD];
  signed char first_displayed_post;
  item *displayed_posts[N_STATUS_TO_LOAD];
  signed char account_height;
  signed char post_height[N_STATUS_TO_LOAD];
  char n_posts;
  char eof;
};
#endif
