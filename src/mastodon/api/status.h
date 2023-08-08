#ifndef __status_h
#define __status_h

#include "account.h"

typedef struct _status status;

#define FAVOURITED (1<<0)
#define REBLOGGED  (1<<1)

struct _status {
  signed char displayed_at;
  char *id;
  char *created_at;
  char *spoiler_text;
  char *content;
  account *account;
  status *reblog;
  char n_replies;
  char n_reblogs;
  char n_favourites;
  char n_images;
  char favorited_or_reblogged;
};

status *status_new(void);
status *status_new_from_json(char *id, char full, char is_reblog);
void status_free(status *s);

#endif
