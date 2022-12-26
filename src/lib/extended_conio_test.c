#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "extended_conio.h"
#include "bool_array.h"

char s_xlen[255], s_ylen[255];

int main(void) {
  int x = 0, y = 0;
  
  while (1) {
    cgetc();
    printfat(x, y, 1, "%d,%d-Printing at.", x, y);
    x++;
    y++;
    if (y > 22)
      y = 22;
  clrzone(1,1,7,22);

  }

  return (0);
}
