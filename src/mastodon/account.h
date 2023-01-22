#ifndef __account_h
#define __account_h

typedef struct _account account;

struct _account {
  char *id;
  char *username;
  char *display_name;
};

account *account_new(void);
account *account_new_from_status_json(surl_response *resp, char is_reblog);
void account_free(account *a);

#endif
