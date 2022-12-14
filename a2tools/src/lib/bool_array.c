#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bool_array.h"

struct _bool_array {
  int x_len;
  int y_len;
  char *data;
};

static size_t alloc_size(int xlen, int ylen) {
  size_t allocsize = 1 + (xlen / 8);

  return allocsize * ylen;
}

bool_array *bool_array_alloc(int xlen, int ylen) {
    size_t allocsize = alloc_size(xlen, ylen);
    bool_array *array = malloc(sizeof(struct _bool_array));
    char *data = NULL;

    data = malloc(allocsize);

    if (data == NULL) {
      return NULL;
    }
    memset(data, 0, allocsize);

    array->x_len = xlen;
    array->y_len = ylen;
    array->data = data;

    return array;
}

size_t bool_array_get_storage_size(bool_array *array) {
  return sizeof(struct _bool_array)
    + alloc_size(array->x_len, array->y_len);
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
