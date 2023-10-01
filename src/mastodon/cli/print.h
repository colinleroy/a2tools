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

#ifdef __APPLE2ENH__
#define CHLINE_CHAR '_'
#else
#define CHLINE_CHAR '-'
#endif
#define CHLINE_SAFE() do {            \
  chline(scrw - RIGHT_COL_START - 1); \
  if (writable_lines > 1)             \
    dputc(CHLINE_CHAR);               \
  else                                \
    cputc(CHLINE_CHAR);               \
  CHECK_NO_CRLF();                    \
} while (0)

int print_buf(char *buffer, char hide, char allow_scroll);
int print_status(status *s, char hide, char full);

#endif
