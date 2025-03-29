#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#define MIDX 138
#define MAXY 191
#define FOVY 78.0
#define LEFT_BORDER 3
#define HGR_H 192

int x_divisor_at_y(int y) {
  return FOVY + MAXY - y;
}

float y_divisor_at_y(int y) {
  return (FOVY + MAXY - y)/2.05f;
}

float x_shift_at_y(int y) {
  return (-MIDX*FOVY)/(FOVY + MAXY - y) + MIDX + LEFT_BORDER;
}

void build_x_mult_table(void) {
  int y, x_mult;
  printf("x_mult: .byte ");
  for (y = 0; y < HGR_H; y++) {
    x_mult = (256*FOVY)/x_divisor_at_y(y);
    printf("%d%s", x_mult < 256 ? x_mult : 0, y < HGR_H-1 ? ", ":"");
  }
  printf("\n");
}

void build_x_shift_table(void) {
  int y, x_shift;
  printf("x_shift: .byte ");
  for (y = 0; y < HGR_H; y++) {
    x_shift = x_shift_at_y(y);
    printf("%d%s", x_shift, y < HGR_H-1 ? ", ":"");
  }
  printf("\n");
}

void build_y_table(void) {
  int y, trans_y;
  printf("y_transform: .byte ");
  for (y = 0; y < HGR_H; y++) {
    trans_y = MAXY-(((MAXY-y) * FOVY) / y_divisor_at_y(y));
    printf("%d%s", trans_y, y < HGR_H-1 ? ", ":"");
  }
  printf("\n");
}

int main(int argc, char *argv[]) {
  printf(".export x_shift, x_mult, y_transform\n");
  build_x_mult_table();
  build_x_shift_table();
  build_y_table();
}
