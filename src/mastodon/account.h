#ifndef __account_h
#define __account_h

typedef struct _account account;

struct _account {
  char *id;
  char *username;
  char *display_name;
};

account *account_new(void);
void account_free(account *a);

#endif
