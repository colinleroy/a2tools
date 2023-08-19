#ifndef __status_h
#define __status_h

#include "account.h"

typedef struct _status status;

#define FAVOURITED (1<<0)
#define REBLOGGED  (1<<1)

struct _status {
  signed char displayed_at;
  char *id;
  char *reblog_id;
  char *created_at;
  char *spoiler_text;
  char *content;
  account *account;
  char *reblogged_by;
  char n_replies;
  char n_reblogs;
  char n_favourites;
  char n_images;
  char favorited_or_reblogged;
  char visibility;
};

status *status_new(void);
void status_free(status *s);

#endif
