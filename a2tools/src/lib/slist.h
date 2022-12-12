#include <stdlib.h>

#ifndef __slist_h
#define __slist_h

typedef struct _slist slist;

struct _slist {
  void *data;
  slist *next;
};

void slist_free(slist *list);
slist *slist_append(slist *list, void *data);
slist *slist_prepend(slist *list, void *data);
slist *slist_reverse(slist *list);

slist *slist_copy(slist *list);

slist *slist_find(slist *list, void *data);

slist *slist_remove(slist *list, slist *elt);
slist *slist_remove_data(slist *list, void *data);

#endif
