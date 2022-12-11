#include <stdlib.h>

void bubble_sort_array(void **array, size_t n_elts, int (*sort_func)( void *, void * )) {
  int i;
  int swapped_anything;

again:
  swapped_anything = 0;
  for (i = 0; i < n_elts - 1; i++) {
    if (sort_func(array[i], array[i + 1])) {
      void *tmp = array[i];
      array[i] = array[i + 1];
      array[i + 1] = tmp;
      swapped_anything = 1;
    }
  }
  if (swapped_anything)
    goto again;
}
