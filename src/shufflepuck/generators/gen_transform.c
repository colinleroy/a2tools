#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#define MIDX 138
#define MAXY 191
#define FOVY 78.0
#define LEFT_BORDER 3
#define HGR_H 193

int x_divisor_at_y(int y) {
  return FOVY + MAXY - y;
}

float y_divisor_at_y(int y) {
  return (FOVY + MAXY - y)/2.05f;
}

float x_shift_at_y(int y) {
  return (-MIDX*FOVY)/(FOVY + MAXY - y) + MIDX + LEFT_BORDER;
}

void build_x_factor_table(void) {
  int y, x_factor;
  printf(".proc x_factor\n"
         "        .byte ");
  for (y = 0; y < HGR_H; y++) {
    x_factor = (256*FOVY)/x_divisor_at_y(y);
    printf("%d%s", x_factor < 256 ? x_factor : 0, y < HGR_H-1 ? ", ":"");
  }
  printf("\n.endproc\n");
}

void build_x_shift_table(void) {
  int y, x_shift;
  printf(".proc x_shift\n"
         "        .byte ");
  for (y = 0; y < HGR_H; y++) {
    x_shift = x_shift_at_y(y);
    printf("%d%s", x_shift, y < HGR_H-1 ? ", ":"");
  }
  printf("\n.endproc\n");
}

void build_y_factor_table(void) {
  int y, trans_y;
  printf(".proc y_factor\n"
         "        .byte ");
  for (y = 0; y < HGR_H; y++) {
    trans_y = MAXY-(((MAXY-y) * FOVY) / y_divisor_at_y(y));
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
