#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <iconv.h>
#include <langinfo.h>
#include <locale.h>
#include "char_convert.h"
#include "math.h"

static char *do_conv(char *in, char *from, char *to, size_t *new_len) {
  char *out = NULL;
  char *orig_out, *orig_in;
  size_t in_len, out_len;
  iconv_t conv;
  size_t res;

  setlocale (LC_ALL, "");

  if (in == NULL) {
    return NULL;
  }
  if (strlen(in) == 0) {
    *new_len = 0;
    return strdup(in);
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
  if (res != (size_t)-1) {
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
  char *out_final = NULL;
  char *out_ascii = NULL;
  size_t ascii_len, i;

  if (way == OUTGOING) {
    out_final = do_conv(in, "UTF-8", "ISO646-FR1//TRANSLIT", new_len);
    out_ascii = do_conv(in, "UTF-8", "US-ASCII//TRANSLIT", &ascii_len);
    
    for (i = 0; i < min(*new_len, ascii_len); i++) {
      if (out_final[i] == '?' && out_ascii[i] != '?')
      out_final[i] = out_ascii[i];
    }
    free(out_ascii);
    printf("iconv %s => %s\n", in, out_final);
    return out_final;
  } else {
    out_final = do_conv(in, "ISO646-FR1", "UTF-8//TRANSLIT", new_len);
    return out_final;
  }
}
