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
  long start_secs = 0;
  long start_msecs = 0;
  long secs = 0;
  long msecs = 0;
  struct timespec cur_time;

  out[0] = '\0';

  clock_gettime(CLOCK_REALTIME, &cur_time);
  start_secs = cur_time.tv_sec - 1;
  start_msecs = 1000 + (cur_time.tv_nsec / 1000000);

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
      strcpy(out + out_len, tmp);
      out_len += strlen(tmp);
      jv_free(str);
    } else {
      tmp = jv_string_value(result);
      strcpy(out + out_len, tmp);
      out_len += strlen(tmp);
      jv_free(result);
    }

    out[out_len++] = '\n';
    out[out_len] = '\0';
  }

  jv_free(result);

  clock_gettime(CLOCK_REALTIME, &cur_time);
  secs = cur_time.tv_sec - 1;
  msecs = 1000 + (cur_time.tv_nsec / 1000000);
  
  printf("jq_get %s %lums\n", selector,
      (1000*(secs - start_secs))+(msecs - start_msecs));

  return strdup(out);
}
