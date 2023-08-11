#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <conio.h>

#pragma code-name (push, "LC")

static signed char ref_x = -1;
static signed char ref_y = -1;

static void __fastcall__ test(int on) {
  char x, y;
  x = wherex();
  y = wherey();

  if (ref_x == -1) {
    ref_x = x;
    ref_y = y;
  }

  gotoxy(ref_x, ref_y);
  cputc(on ? '*' : ' ');
  gotoxy(x, y);
}
#pragma code-name (pop)

int main(int argc, char *argv[]) {
  char on = 0;
  clrscr();
  ref_x = 39;
  ref_y = 0;
  while (1) {
    test(on);
    on = !on;
  }

  return 0;
}
