#ifndef __status_h
#define __status_h

#include "common.h"
#include "account.h"
#include "poll.h"

extern unsigned char TL_SPOILER_TEXT_BUF;

typedef struct _status status;

#define FAVOURITED (1<<0)
#define REBLOGGED  (1<<1)
#define BOOKMARKED (1<<2)
#define LAST_FLAGS BOOKMARKED

struct _status {
  signed char displayed_at;
  char id[SNOWFLAKE_ID_LEN];
  char reblog_id[SNOWFLAKE_ID_LEN];
  char *created_at;
  char *spoiler_text;
  char *content;
  account *account;
  char *reblogged_by;
  unsigned char n_replies;
  unsigned char n_reblogs;
  unsigned char n_favourites;
  unsigned char n_medias;
  char media_type;
  char flags;
  char visibility;
  poll *poll;
};

void status_free(status *s);

#endif
