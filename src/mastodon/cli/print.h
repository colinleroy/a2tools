#ifndef __print_h
#define __print_h

#include "status.h"

#define CHECK_AND_CRLF() do { \
  if (--writable_lines == 0)  \
    return -1;                \
  dputs("\r\n");              \
} while (0)

#define CHECK_NO_CRLF() do {  \
  if (--writable_lines == 0)  \
    return -1;                \
} while (0)

#define CHLINE_SAFE() do {            \
  chline(scrw - LEFT_COL_WIDTH - 2);  \
  if (writable_lines > 1)             \
    dputc('_');                       \
  else                                \
    cputc('_');                       \
  CHECK_NO_CRLF();                    \
} while (0)

int print_buf(char *buffer, char hide, char allow_scroll);
int print_status(status *s, char hide, char full);

#endif
