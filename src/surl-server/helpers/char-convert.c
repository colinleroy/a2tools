#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <iconv.h>
#include <langinfo.h>
#include <locale.h>
#include "char-convert.h"
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
    return strdup(orig_in);
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
    free(orig_out);
    orig_out = strdup(orig_in);
  }
  return orig_out;
}

char *do_apple_convert(char *in, int way, char *a2charset, size_t *new_len) {
  char *out_final = NULL;
  char *out_ascii = NULL;
  char translit_charset[50];
  
  size_t ascii_len, i;

  if (in == NULL) {
    return NULL;
  }
  snprintf(translit_charset, 50, "%s//TRANSLIT", a2charset);
  if (way == OUTGOING) {
    out_final = do_conv(in, "UTF-8", translit_charset, new_len);
    out_ascii = do_conv(in, "UTF-8", "US-ASCII//TRANSLIT", &ascii_len);
    
    if (!strcmp(a2charset, "ISO646-FR1")) {
      /* Adapt for legibility */
      for (i = 0; i < min(*new_len, ascii_len); i++) {
        if (out_final[i] == '?' && out_ascii[i] != '?') {
          switch(out_ascii[i]) {
            case '[': out_final[i] = '('; break;
            case ']': out_final[i] = ')'; break;
            case '{': out_final[i] = '('; break;
            case '}': out_final[i] = ')'; break;
            case '|': out_final[i] = '!'; break;
            case '@': out_final[i] = ']'; break;
            case '#': out_final[i] = out_ascii[i]; break;
          }
        }
      }
    }
    free(out_ascii);
    return out_final;
  } else {
    if (!strcmp(a2charset, "ISO646-FR1")) {
      /* Do a step to ISO8859 to keep § and £ chars */
      char *out;
      char *tmp = do_conv(in, a2charset, "ISO-8859-1//TRANSLIT", new_len);
      for (i = 0; i < strlen(tmp); i++) {
        switch(tmp[i]) {
          case '\247': tmp[i] = '@'; break;
          case '\243': tmp[i] = '#'; break;
        }
      }
      out = do_conv(tmp, "ISO-8859-1", "UTF-8//TRANSLIT", new_len);
      free(tmp);
      return out;
    } else {
      return do_conv(in, a2charset, "UTF-8//TRANSLIT", new_len);
    }
  }
  return NULL;
}
