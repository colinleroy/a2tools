#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "one-point-perspective.h"

void build_x_factor_table(void) {
  int y, x;
  printf(".proc x_factor\n"
         "        .byte ");
  for (y = 0; y < HGR_H; y++) {
    x = x_factor(y);
    printf("%d%s", x < 256 ? x : 0, y < HGR_H-1 ? ", ":"");
  }
  printf("\n.endproc\n");
}

void build_x_shift_table(void) {
  int y, x;
  printf(".proc x_shift\n"
         "        .byte ");
  for (y = 0; y < HGR_H; y++) {
    x = x_shift(y);
    printf("%d%s", x, y < HGR_H-1 ? ", ":"");
  }
  printf("\n.endproc\n");
}

void build_y_factor_table(void) {
  int y, trans_y;
  printf(".proc y_factor\n"
         "        .byte ");
  for (y = 0; y < HGR_H; y++) {
    trans_y = y_factor(y);
    printf("%d%s", trans_y < HGR_H ? trans_y : HGR_H-1, y < HGR_H-1 ? ", ":"");
  }
  printf("\n.endproc\n");
}

int main(int argc, char *argv[]) {
  printf(".export x_shift, x_factor, y_factor\n\n");
  printf(".segment \"SPLC\"\n\n");
  build_x_factor_table();
  build_x_shift_table();
  build_y_factor_table();
}
