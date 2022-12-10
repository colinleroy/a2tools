#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bool_array.h"
#include "extended_conio.h"

char s_xlen[255], s_ylen[255];

int main(void) {
  int x, y;
  int xlen, ylen;
  bool_array *array = NULL;

  printf("X len: ");
  cgets(s_xlen, 255);
  
  printf("Y len: ");
  cgets(s_ylen, 255);
  
  xlen = atoi(s_xlen);
  ylen = atoi(s_ylen);

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
    for (y = 0; y < ylen; y+=10) {
      if (bool_array_get(array, x, y) != 1) {
        printf("error at %d,%d\n", x, y);
      }
    }
  }
  bool_array_free(array);
  printf("done.\n");
  
  return 0;
}
