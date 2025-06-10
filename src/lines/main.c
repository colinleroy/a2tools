#include <apple2.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>
// #include <dbg.h>

extern void plot_algorithms(void);
extern void build_hgr_tables(void);

extern void init_hgr(unsigned char a);
void main(void) {
  plot_algorithms();
}
