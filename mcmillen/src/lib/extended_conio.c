#include "extended_conio.h"

char *cgets(char *buf, size_t size) {
  char c;
  size_t i = 0;
  while (i < size - 1) {

    while(!kbhit());
    c = cgetc();

    if (c == '\n') {
      break;
    }

    buf[i] = c;
    i++;
  }
  
  buf[i] = '\0';

  return buf;
}
