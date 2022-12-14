#include <stdlib.h>

#ifndef __array_sort_h
#define __array_sort_h

typedef int (*sort_func) (void *, void *);

void bubble_sort_array(void **array, size_t n_elts, sort_func func);
#endif
