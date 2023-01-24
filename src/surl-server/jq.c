#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <jq.h>
#include "jq.h"

char *jq_get(char *buffer, char *selector) {
  jq_state *jq = NULL;
  jv json_data;
  jv result;
  char *out = NULL;
  size_t out_len = 0;

  jq = jq_init();
  if (jq == NULL) {
    return NULL;
  }

  json_data = jv_parse(buffer);
  if (!jv_is_valid(json_data)) {
    jq_teardown(&jq);
    return NULL;
  }

  if (!jq_compile(jq, selector)) {
    jq_teardown(&jq);
    return NULL;
  }

  jq_start(jq, json_data, 0);
  
  while (jv_is_valid(result = jq_next(jq))) {
    jv str;
    char *tmp;
    if (jv_get_kind(result) == JV_KIND_NULL
     || jv_get_kind(result) == JV_KIND_INVALID) {
       continue;
    }

    if (jv_get_kind(result) != JV_KIND_STRING) {
      str = jv_dump_string(result, 0);
      tmp = strdup(jv_string_value(str));
      jv_free(str);
    } else {
      tmp = strdup(jv_string_value(result));
      jv_free(result);
    }

    out = realloc(out, out_len + strlen(tmp) + 2);
    strcpy(out + out_len, tmp);
    out_len += strlen(tmp);

    free(tmp);

    out[out_len++] = '\n';
    out[out_len] = '\0';
  }

  jv_free(result);
  jq_teardown(&jq);

  return out;
}
