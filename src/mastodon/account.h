#ifndef __account_h
#define __account_h

typedef struct _account account;

#define RSHIP_SET         (1<<0)
#define RSHIP_FOLLOWING   (1<<1)
#define RSHIP_FOLLOWED_BY (1<<2)
#define RSHIP_BLOCKING    (1<<3)
#define RSHIP_BLOCKED_BY  (1<<4)
#define RSHIP_MUTING      (1<<5)
#define RSHIP_FOLLOW_REQ  (1<<6)

struct _account {
  char *id;
  char *username;
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

account *account_new(void);
account *account_new_from_json(surl_response *resp);
void account_free(account *a);

char account_relationship_get(account *a, char f);
#endif
