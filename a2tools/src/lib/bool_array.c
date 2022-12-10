#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bool_array.h"

struct _bool_array {
  int y_len;
  char *data;
};

bool_array *bool_array_alloc(int xlen, int ylen) {
    size_t allocsize;
    bool_array *array = malloc(sizeof(void *));
    char *data = NULL;

    allocsize = 1 + (xlen / 8);
    allocsize = allocsize * ylen;

    data = malloc(allocsize);

    if (data == NULL) {
      return NULL;
    }
    memset(data, 0, allocsize);

    array->y_len = ylen;
    array->data = data;

    return array;
}

void bool_array_free(bool_array *array) {
  if (array != NULL) {
    if (array->data != NULL) {
      free(array->data);
    }
    free(array);
  }
}

static int bool_array_access(bool_array *array, int x, int y, int set, int val) {
  long offset = (long)((long)(array->y_len) * (long)x) + (long)y;
  size_t byte = (long)offset / (long)8;
  size_t bit = (long)offset % (long)8;

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
