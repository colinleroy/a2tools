#ifndef __list_h
#define __list_h

#include <string.h>

typedef struct _list list;

#define L_HOME_TIMELINE   0
#define L_LOCAL_TIMELINE  1
#define L_PUBLIC_TIMELINE 2
#define L_FULL_STATUS     3

struct _list {
  char kind;
  char *root;
  char **ids;
  signed char first_displayed_post;
  status **displayed_posts;
  char last_displayed_post;
  signed char *post_height;
  char n_posts;
  char eof;
  char scrolled;
};
#endif
