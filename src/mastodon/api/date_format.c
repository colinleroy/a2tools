#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "date_format.h"

#ifdef __APPLE2__
#include <apple2enh.h>
#endif

#ifdef __CC65__
#pragma code-name (push, "LOWCODE")
#endif

char *date_format(char *in, char with_time) {
  char *out = strdup(in);
  char *sep = strchr(out, 'T');
  if (sep) {
    if (!with_time) {
      *sep = '\0';
      return out;
    }
    *sep = ' ';
  }
  sep = strchr(out, '.');
  if (sep) {
    *sep = '\0';
  }
  return out;
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
