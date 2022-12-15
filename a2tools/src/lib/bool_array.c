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

#define BOOL_ARRAY_SET_BIT(a,x,y) (a->data[x] |= (1 << (y)))
#define BOOL_ARRAY_CLR_BIT(a,x,y) (a->data[x] &= ~(1 << (y)))
#define BOOL_ARRAY_GET_BIT(a,x,y) (a->data[x] & (1 << (y)))

static long ba_offset;

void __fastcall__ bool_array_set(bool_array *array, int x, int y, int val) {
  ba_offset = (long)((long)(array->y_len) * (long)x) + (long)y;

  if (val == 0)
    BOOL_ARRAY_CLR_BIT(array, ba_offset >> 3, ba_offset & 7);
  else
    BOOL_ARRAY_SET_BIT(array, ba_offset >> 3, ba_offset & 7);
}

int __fastcall__ bool_array_get(bool_array *array, int x, int y) {
  ba_offset = (long)((long)(array->y_len) * (long)x) + (long)y;

  return BOOL_ARRAY_GET_BIT(array, ba_offset >> 3, ba_offset & 7);
}
