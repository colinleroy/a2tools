#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bool_array.h"

struct _bool_array {
  int y_len;
  char *data;
};

bool_array *bool_array_alloc(int xlen, int ylen) {
    size_t tmp, allocsize;
    bool_array *array = malloc(2*sizeof(int));
    char *data = NULL;

    tmp = 1 + (xlen+2)/8;
    allocsize = tmp*(ylen+3);

    data = malloc(allocsize);
    if (data == NULL) {
      return NULL;
    }
    memset(data, 0, allocsize);
    array->data = data;
    array->y_len = ylen;
    return array;
}

static int bool_array_access(bool_array *array, int x, int y, int set, int val) {
  /* FIXME The offsets should be in the callers */
  long offset = ((array->y_len + 1) * x) + y;
  size_t byte = 1 + (offset / 8);
  size_t bit = offset % 8;

  if (set) {
    if (val) {
      array->data[byte] |= (1 << bit);
    } else {
      array->data[byte] &= ~(1 << bit);
    }
  }
  return (array->data[byte] & (1 << bit)) != 0;
}

int bool_array_set(bool_array *array, int x, int y, int val) {
  return bool_array_access(array, x, y, 1, val);
}

int bool_array_get(bool_array *array, int x, int y) {
  return bool_array_access(array, x, y, 0, 0);
}
