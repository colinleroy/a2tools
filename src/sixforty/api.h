#ifndef __api_h__
#define __api_h__

#include "surl.h"

#define NUM_POST_FIELDS 6
typedef struct _post_t {
  char *id;
  char *image_url;
  char *description;
  char *author;
  char *date;
  unsigned int comment_count;
} post_t;

#define NUM_COMMENT_FIELDS 3
typedef struct _comment_t {
  char *text;
  char *author;
  char *date;
} comment_t;

void post_free(post_t *post);
void comment_free(comment_t *comment);

#define translit_charset "US-ASCII"

#define BUF_SIZE 511
extern char gen_buf[BUF_SIZE+1];

#define MAX_LINES_NUM 255
extern char *lines[MAX_LINES_NUM];


char api_login(char *saved_creds);
post_t *api_get_post(signed char offset);
comment_t *api_get_comment(post_t *post, unsigned char index);

char *api_get_creds(void);

char api_delete_post(post_t *post);
char api_post_hgr_image(char *filename, char *description, char x, char y, char w);

char api_post_comment(post_t *post, char *comment);
#endif
