#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int main(void) {
  int a = 196, b = 33, c = 4;
  long result;

  result = (long)(a * 1000L) + (long)(b * 4L) + (long)(c);
  printf("(%d*1000 + %d*4 + %d) = %ld\n", a, b, c, result);
  
  result = (long)((long)(a * 1000L) + (long)(b * 4L) + (long)(c));
  printf("(%d*1000 + %d*4 + %d) = %ld\n", a, b, c, result);

}
