#include <apple2.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>
// #include <dbg.h>

extern void test(void);
extern void setLine(void);
extern void plotLine(void);
extern void build_hgr_tables(void);
extern void init_hgr(unsigned char a);
void main(void) {
  bzero(0x2000,0x2000);
  build_hgr_tables();
  init_hgr(1);
  // plotLine();
  // cgetc();
  // bzero(0x2000,0x2000);
  test();
  cgetc();
}
