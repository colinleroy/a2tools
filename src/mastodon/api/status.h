#ifndef __status_h
#define __status_h

#include "account.h"

#ifdef __APPLE2ENH__
#define TL_SPOILER_TEXT_BUF 54
#else
#define TL_SPOILER_TEXT_BUF 34
#endif

typedef struct _status status;

#define FAVOURITED (1<<0)
#define REBLOGGED  (1<<1)
#define BOOKMARKED (1<<2)
#define LAST_FLAGS BOOKMARKED

struct _status {
  signed char displayed_at;
  char *id;
  char *reblog_id;
  char *created_at;
  char *spoiler_text;
  char *content;
  account *account;
  char *reblogged_by;
  unsigned char n_replies;
  unsigned char n_reblogs;
  unsigned char n_favourites;
  unsigned char n_images;
  char flags;
  char visibility;
};

status *status_new(void);
void status_free(status *s);

#endif
