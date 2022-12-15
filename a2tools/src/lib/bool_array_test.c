#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef __CC65__
#include "extended_conio.h"
#endif
#include "bool_array.h"

char s_xlen[255], s_ylen[255];

int main(void) {
  int x, y;
  int xlen, ylen;
  bool_array *array = NULL;
#ifdef __CC65__
  printf("X len: ");
  cgets(s_xlen, 255);
  
  printf("Y len: ");
  cgets(s_ylen, 255);

  xlen = atoi(s_xlen);
  ylen = atoi(s_ylen);
#else
  xlen = 500;
  ylen = 800;
#endif
  array = bool_array_alloc(xlen, ylen);
  if (array == NULL) {
    printf("couldn't alloc array.\n");
    exit (1);
  }

  for (x = 0; x < xlen; x+=10) {
    for (y = 0; y < ylen; y+=10) {
      bool_array_set(array, x, y, 1);
    }
  }
  
  printf("checking array.\n");
  for (x = 0; x < xlen; x+=10) {
    for (y = 0; y < ylen - 1; y+=10) {
      if (x > 0 && bool_array_get(array, x-1, y)) {
        printf("error a at %d,%d\n", x-1, y);
      }
      if (!bool_array_get(array, x, y)) {
        printf("error b at %d,%d:%d\n", x, y, bool_array_get(array, x, y));
      }
      if (bool_array_get(array, x, y+1)) {
        printf("error c at %d,%d\n", x, y+1);
      }
    }
  }
  bool_array_free(array);
  printf("done.\n");
  
  return 0;
}
