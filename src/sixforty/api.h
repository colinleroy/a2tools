#ifndef __api_h__
#define __api_h__

#include "surl.h"

#define NUM_POST_FIELDS 5

typedef struct _post_t {
  char *id;
  char *image_url;
  char *description;
  char *author;
  char *date;
} post_t;

void post_free(post_t *post);


#define translit_charset "US-ASCII"

#define BUF_SIZE 511
extern char gen_buf[BUF_SIZE+1];

#define MAX_LINES_NUM 255
extern char *lines[MAX_LINES_NUM];


char api_login(char *saved_creds);
post_t *api_get_post(signed char offset);
char *api_get_creds(void);

char api_delete_post(post_t *post);
#endif
