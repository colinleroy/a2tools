#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "malloc0.h"
#include "extended_conio.h"
#include "surl.h"
#include "simple_serial.h"
#include "strsplit.h"
#include "api.h"
#include "common.h"

#ifdef __CC65__
  #ifdef SURL_TO_LANGCARD
  #pragma code-name (push, "LC")
  #else
  #pragma code-name (push, "LOWCODE")
  #endif
#endif

char *translit_charset = US_CHARSET;
char arobase = '@';

extern char *instance_url;
extern char *oauth_token;

/* shared */
#ifndef __CC65__
char gen_buf[BUF_SIZE];
char selector[SELECTOR_SIZE];
char endpoint_buf[ENDPOINT_BUF_SIZE];
char *lines[MAX_LINES_NUM];
#else
/* Buffers in iobuf, defined in buffers.s */
#endif

const surl_response *get_surl_for_endpoint(char method, char *endpoint) {
  static char *hdrs[1] = {NULL};

  if (IS_NULL(hdrs[0])) {
    hdrs[0] = malloc0(BUF_SIZE);
    snprintf(hdrs[0], BUF_SIZE, "Authorization: Bearer %s", oauth_token);
  }

  strcpy(gen_buf, instance_url);
  strcat(gen_buf, endpoint);
  return surl_start_request(hdrs, 1, gen_buf, method);
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
