typedef struct _bool_array bool_array;

bool_array *bool_array_alloc(int xlen, int ylen);

int bool_array_set(bool_array *array, int x, int y, int val);

int bool_array_get(bool_array *array, int x, int y);
