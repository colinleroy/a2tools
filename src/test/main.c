#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <conio.h>
#include <unistd.h>
#include "clrzone.h"

int main(int argc, char *argv[]) {
  char buf[100];
  getcwd(buf, 100);
  printf("%s, %zuB free\n", buf, _heapmemavail());
  cgetc();
}
