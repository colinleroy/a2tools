#ifndef __print_h
#define __print_h

#include "status.h"

#define CHECK_AND_CRLF() do { \
  if (wherey() == scrh - 1) { \
    return -1;                \
  }                           \
  dputs("\r\n");              \
} while (0)

#define FAST_CHECK_AND_CRLF() do { \
  y++;                        \
  if (y == scrh) {            \
    return -1;                \
  }                           \
  dputs("\r\n");              \
} while (0)

int print_buf(char *w, char hide, char allow_scroll, char *scrolled);
int print_status(status *s, char hide, char full, char *scrolled);

#endif
