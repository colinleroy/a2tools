#ifndef __status_h
#define __status_h

#include "account.h"

typedef struct _status status;

#define FAVOURITED (1<<0)
#define REBLOGGED  (1<<1)

struct _status {
  char *id;
  char *created_at;
  char *content;
  account *account;
  status *reblog;
  signed char displayed_at;
  signed char stats_line;
  char n_replies;
  char n_reblogs;
  char n_favourites;
  char n_images;
  char favorited_or_reblogged;
};

status *status_new(void);
status *status_new_from_json(surl_response *resp, char *id, char full, char is_reblog);
void status_free(status *s);

#endif
