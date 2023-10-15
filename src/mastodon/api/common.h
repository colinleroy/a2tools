#ifndef __common_h
#define __common_h

typedef struct _item item;
struct _item {
  /* common fields to status, account, notification */
  signed char displayed_at;
  char *id;
};


extern char *tl_endpoints[4];
extern char *tl_filter[4];

/* Refs: https://www.aivosto.com/articles/charsets-7bit.html
 * and https://archive.org/details/Apple_IIgs_Hardware_Reference_HiRes/page/n275/mode/1up?view=theater
 * FIXME make that configurable */
#define US_CHARSET "US-ASCII"
#define FR_CHARSET "ISO646-FR1"
#define ES_CHARSET "ISO646-ES"
#define IT_CHARSET "ISO646-IT"
#define DE_CHARSET "ISO646-DE"

/* Shared buffers */
#define BUF_SIZE 255
extern char gen_buf[BUF_SIZE];

#define SELECTOR_SIZE 128
extern char selector[SELECTOR_SIZE];

#define ENDPOINT_BUF_SIZE 128
extern char endpoint_buf[ENDPOINT_BUF_SIZE];

#define MAX_LINES_NUM 32
extern char *lines[MAX_LINES_NUM];

extern char *instance_url;
extern char *oauth_token;
extern char *translit_charset;
extern char arobase;

#define TIMELINE_ENDPOINT       "/api/v1/timelines"
#define BOOKMARKS_ENDPOINT      "/api/v1/bookmarks"
#define ACCOUNTS_ENDPOINT       "/api/v1/accounts"
#define STATUS_ENDPOINT         "/api/v1/statuses"
#define NOTIFICATION_ENDPOINT   "/api/v1/notifications"
#define SEARCH_ENDPOINT         "/api/v2/search"
#define MEDIA_ENDPOINT          "/api/v1/media"
#define VOTES_ENDPOINT          "/api/v1/polls"

#define COMPOSE_PUBLIC   0
#define COMPOSE_UNLISTED 1
#define COMPOSE_PRIVATE  2
#define COMPOSE_MENTION  3

#define HOME_TIMELINE     "home"
#define PUBLIC_TIMELINE   "public"

const surl_response *get_surl_for_endpoint(char method, char *endpoint);
char *date_format(char *in, char with_time);
#endif
