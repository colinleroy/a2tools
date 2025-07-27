#ifndef __print_h
#define __print_h

#include "status.h"

#define CHECK_AND_CRLF() do { \
  if (--writable_lines == 0)  \
    return -1;                \
  clrnln();                   \
} while (0)

#define CHECK_NO_CRLF() do {  \
  if (--writable_lines == 0)  \
    return -1;                \
} while (0)

#define CHLINE_SAFE() do {    \
  chline(RIGHT_COL_WIDTH);    \
  CHECK_NO_CRLF();            \
} while (0)

void __fastcall__ clrnln(void);
int print_buf(char *buffer, char hide, char allow_scroll);
int print_status(status *s, char hide, char full);

#endif
