#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <iconv.h>
#include "char_convert.h"

static char *do_conv(char *in, char *from, char *to, size_t *new_len) {
  char *out = NULL;
  char *orig_out, *orig_in;
  size_t in_len, out_len;
  iconv_t conv;
  int res;

  if (in == NULL) {
    return NULL;
  }
  orig_in = in;

  *new_len = strlen(in);

  conv = iconv_open(to, from);
  if (conv == (iconv_t)-1) {
    perror("iconv");
    return orig_in;
  }
  in_len = strlen(in) + 1;
  out_len = in_len * 2;
  orig_out = malloc(out_len + 1);
  out = orig_out;
  res = iconv(conv, &in, &in_len, &out, &out_len);
  if (res != (iconv_t)-1) {
    iconv_close(conv);
    *new_len = strlen(orig_out);
  } else {
    perror("iconv");
    iconv_close(conv);
    orig_out = strdup(orig_in);
  }
  return orig_out;
}

char *do_apple_convert(char *in, int way, size_t *new_len) {
  char *out_iso = NULL;  
  char *out_final = NULL;

  if (way == OUTGOING) {
    out_iso = do_conv(in, "UTF-8", "ISO_8859-1//TRANSLIT", new_len);
    out_final = do_conv(out_iso, "ISO_8859-1", "ISO646-FR1//TRANSLIT", new_len);
    free(out_iso);
    return out_final;
  } else {
    out_iso = do_conv(in, "ISO646-FR1", "ISO_8859-1//TRANSLIT", new_len);
    out_final = do_conv(out_iso, "ISO_8859-1", "UTF-8", new_len);
    free(out_iso);
    return out_final;
  }
}
