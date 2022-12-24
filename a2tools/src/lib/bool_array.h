#ifndef __bool_array_h
#define __bool_array_h

#ifndef _CC65__
#define __fastcall__
#endif

typedef struct _bool_array bool_array;

bool_array *bool_array_alloc(int xlen, int ylen);
void bool_array_free(bool_array *array);

void __fastcall__ bool_array_set(bool_array *array, int x, int y, int val);

int __fastcall__ bool_array_get(bool_array *array, int x, int y);

#endif
