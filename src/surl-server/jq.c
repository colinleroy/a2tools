#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include <jq.h>
#include "jq.h"

char out[8192];
char *jq_get(jv json_data, char *selector) {
  static jq_state *jq = NULL;
  jv result;
  size_t out_len = 0;
  size_t rem = sizeof(out) - 1;

  out[0] = '\0';

  if (jq == NULL) {
    jq = jq_init();
  }
  if (jq == NULL) {
    return NULL;
  }

  if (!jv_is_valid(json_data)) {
    return NULL;
  }

  if (!jq_compile(jq, selector)) {
    return NULL;
  }

  jq_start(jq, json_data, 0);
  
  while (jv_is_valid(result = jq_next(jq))) {
    jv str;
    const char *tmp;
    if (jv_get_kind(result) == JV_KIND_NULL
     || jv_get_kind(result) == JV_KIND_INVALID) {
       continue;
    }

    if (jv_get_kind(result) != JV_KIND_STRING) {
      str = jv_dump_string(result, 0);
      tmp = jv_string_value(str);
      strncpy(out + out_len, tmp, rem);
      rem -= strlen(tmp);
      out_len += strlen(tmp);
      jv_free(str);
    } else {
      tmp = jv_string_value(result);
      strncpy(out + out_len, tmp, rem);
      rem -= strlen(tmp);
      out_len += strlen(tmp);
      jv_free(result);
    }

    out[out_len++] = '\n';
    rem--;
    out[out_len] = '\0';
  }

  jv_free(result);

  return strdup(out);
}
