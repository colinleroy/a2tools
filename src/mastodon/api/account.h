#ifndef __account_h
#define __account_h

#include "common.h"

typedef struct _account account;

#define RSHIP_SET         (1<<0)
#define RSHIP_FOLLOWING   (1<<1)
#define RSHIP_FOLLOWED_BY (1<<2)
#define RSHIP_BLOCKING    (1<<3)
#define RSHIP_BLOCKED_BY  (1<<4)
#define RSHIP_MUTING      (1<<5)
#define RSHIP_FOLLOW_REQ  (1<<6)

#define MAX_ACCT_FIELDS    4

struct _account {
  signed char displayed_at;
  char id[SNOWFLAKE_ID_LEN];
  char *username;
  char *acct;
  char *display_name;
  /* Optionally filled fields */
  char *created_at;
  long followers_count;
  long following_count;
  char *note;
  char **fields;
  char n_fields;
  char relationship;
};

#define account_new() (account *)malloc0(sizeof(account))

account *account_new_from_json(void);
account *account_new_from_lines(void);

void account_free(account *a);
account *api_get_profile(char *id);

char account_relationship_get(account *a, char f);
#endif
