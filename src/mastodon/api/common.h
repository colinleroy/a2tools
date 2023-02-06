#ifndef __common_h
#define __common_h

typedef struct _item item;
struct _item {
  /* common fields to status, account, notification */
  signed char displayed_at;
  char *id;
};

#define ENDPOINT_BUF_SIZE 128
extern char endpoint_buf[ENDPOINT_BUF_SIZE];

extern char *tl_endpoints[3];
extern char *tl_filter[3];

/* FIXME make that configurable */
#define US_CHARSET "US-ASCII"
#define FR_CHARSET "ISO646-FR1"

#define SELECTOR_SIZE 255
extern char selector[SELECTOR_SIZE];
#define BUF_SIZE 255
extern char gen_buf[BUF_SIZE];

extern char *instance_url;
extern char *oauth_token;
extern char *translit_charset;
extern char arobase;

#define TIMELINE_ENDPOINT       "/api/v1/timelines"
#define ACCOUNTS_ENDPOINT       "/api/v1/accounts"
#define STATUS_ENDPOINT         "/api/v1/statuses"
#define NOTIFICATION_ENDPOINT   "/api/v1/notifications"
#define SEARCH_ENDPOINT         "/api/v2/search"

#define COMPOSE_PUBLIC 0
#define COMPOSE_UNLISTED 1
#define COMPOSE_PRIVATE 2
#define COMPOSE_MENTION 3

#define HOME_TIMELINE     "home"
#define PUBLIC_TIMELINE   "public"

void nomem_msg(char *file, int line);
surl_response *get_surl_for_endpoint(char method, char *endpoint);
char *date_format(char *in, char with_time);
#endif
