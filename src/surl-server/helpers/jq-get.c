#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include <jq.h>
#include "jq-get.h"
#include "../log.h"

char out[8192];

typedef struct _precompiled_jq {
  char *selector;
  jq_state *jq;
} precompiled_jq;

#define N_JQS_MAX 512

static precompiled_jq jqs[N_JQS_MAX];
static int n_jqs = 0;

jq_state *jq_get_for_selector(char *selector) {
  int i;
  static jq_state *fallback_jq = NULL;
  if (n_jqs == 0) {
    memset(jqs, 0, sizeof(jqs));
  }

  for (i = 0; i < n_jqs; i++) {
    if (!strcmp(selector, jqs[i].selector)) {
      return jqs[i].jq;
    }
  }

  if (i < N_JQS_MAX) {
    jq_state *new_jq = jq_init();
    if (new_jq == NULL) {
      return NULL;
    }
    if (!jq_compile(new_jq, selector)) {
      return NULL;
    }
    jqs[i].selector = strdup(selector);
    jqs[i].jq = new_jq;
    n_jqs = i + 1;
    LOG("Adding precompiled jq %p for selector %s [%d/%d]\n", jqs[i].jq, selector, i, N_JQS_MAX);

    return new_jq;
  } else {
    LOG("out of precompiled jqs, using static one\n");
    if (fallback_jq == NULL) {
      fallback_jq = jq_init();
      if (fallback_jq == NULL) {
        return NULL;
      }
    }
    if (!jq_compile(fallback_jq, selector)) {
      return NULL;
    }
    return fallback_jq;
  }
}

char *jq_get(jv json_data, char *selector) {
  jq_state *jq = NULL;
  jv result;
  size_t out_len = 0;
  size_t rem = sizeof(out) - 1;

  out[0] = '\0';

  if (!jv_is_valid(json_data)) {
    return NULL;
  }

  jq = jq_get_for_selector(selector);
  if (jq == NULL) {
    return NULL;
  }

  jq_start(jq, json_data, 0);
  
  while (jv_is_valid(result = jq_next(jq))) {
    jv str;
    size_t len;

    const char *tmp;
    if (jv_get_kind(result) == JV_KIND_NULL
     || jv_get_kind(result) == JV_KIND_INVALID) {
       continue;
    }

    if (jv_get_kind(result) != JV_KIND_STRING) {
      str = jv_dump_string(result, 0);
      tmp = jv_string_value(str);
      strncpy(out + out_len, tmp, rem);
      len = strlen(tmp);
      rem -= len;
      out_len += len;
      jv_free(str);
    } else {
      tmp = jv_string_value(result);
      strncpy(out + out_len, tmp, rem);
      len = strlen(tmp);
      rem -= len;
      out_len += len;
      jv_free(result);
    }

    out[out_len++] = '\n';
    rem--;
    out[out_len] = '\0';
  }

  jv_free(result);

  return strdup(out);
}
