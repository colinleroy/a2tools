#ifndef __poll_h
#define __poll_h

typedef struct _poll poll;
typedef struct _poll_option poll_option;

struct _poll {
  char *id;
  char expired;
  char multiple;
  size_t votes_count;
  size_t voters_count;
  char options_count;
  poll_option **options;
};

struct _poll_option {
  char *title;
  size_t votes_count;
};

poll *poll_new(void);
void poll_free(poll *p);

#endif
