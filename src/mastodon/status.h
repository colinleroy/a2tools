#ifndef __status_h
#define __status_h

#include "account.h"

typedef struct _status status;

struct _status {
  char *id;
  char *created_at;
  char *content;
  account *account;
  status *reblog;
};

status *status_new(void);
status *status_new_from_json(surl_response *resp, char *id, char is_reblog);
void status_free(status *s);

#endif
