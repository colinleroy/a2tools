/*
 * Copyright (C) 2022 Colin Leroy-Mira <colin@colino.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stddef.h>
#include "slist.h"

static slist *slist_new(void *data) {
  slist *l = malloc(sizeof(slist));

  l->data = data;
  l->next = NULL;

  return l;
}

void slist_free(slist *list) {
  slist *n;
  
  if (list == NULL)
    return;
  
  n = list->next;

  free(list);
  if (n != NULL) {
    slist_free(n);
  }
}

slist *slist_append(slist *list, void *data) {
  slist *new = slist_new(data);
  slist *orig = list;
  
  if (list == NULL) {
    return new;
  }

  orig = list;
  while (list->next != NULL) {
    list = list->next;
  }
  
  list->next = new;
  return orig;
}

slist *slist_prepend(slist *list, void *data) {
  slist *new = slist_new(data);

  new->next = list;
  
  return new;
}

slist *slist_reverse(slist *list) {
  slist *rev = NULL, *orig = list;
  if (list == NULL) {
    return NULL;
  }
  
  while (list->next != NULL) {
    rev = slist_prepend(rev, list->data);
    list = list->next;
  }
  rev = slist_prepend(rev, list->data);
  slist_free(orig);

  return rev;
}

long slist_length(slist *list) {
  int count = 0;
  while (list != NULL) {
    count++;
    list = list->next;
  }
  return count;
}

slist *slist_remove(slist *list, slist *elt) {
  slist *orig = list;
  slist *prev = NULL;

  if (orig == NULL) {
    return NULL;
  }

  while (list != NULL) {
    if (list == elt) {
      if (prev) {
        /* we remove an element in the middle
         * or at the end */
        prev->next = list->next;
        free(list);
        break;
      } else {
        /* we remove the first element */
        orig = list->next;
        free(list);
        break;
      }
    }
    prev = list;
    list = list->next;
  }
  return orig;
}

slist *slist_copy(slist *list) {
  slist *new = NULL;
  if (list == NULL) {
    return NULL;
  }

  for (; list; list = list->next) {
    new = slist_prepend(new, list->data);
  }
  return slist_reverse(new);
}

slist *slist_find(slist *list, void *data) {
  while (list != NULL) {
    if (list->data == data)
      return list;
    list = list->next;
  }
  return NULL;
}

slist *slist_remove_data(slist *list, void *data) {
  slist *elt;
  while ((elt = slist_find(list, data)) != NULL) {
    list = slist_remove(list, elt);
  }
  return list;
}
