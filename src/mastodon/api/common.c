#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "surl.h"
#include "simple_serial.h"
#include "strsplit.h"
#include "api.h"
#include "common.h"

#ifdef __CC65__
#ifdef SERIAL_TO_LANGCARD
#pragma code-name (push, "LC")
#else
#pragma code-name (push, "LOWCODE")
#endif
#endif

#define ENDPOINT_BUF_SIZE 128
char endpoint_buf[ENDPOINT_BUF_SIZE];

char *translit_charset = US_CHARSET;
char arobase = '@';
char *tl_endpoints[3] = { TIMELINE_ENDPOINT "/" HOME_TIMELINE,
                          TIMELINE_ENDPOINT "/" PUBLIC_TIMELINE,
                          TIMELINE_ENDPOINT "/" PUBLIC_TIMELINE};
char *tl_filter[3] = { NULL,
                       "&local=true",
                       NULL};

extern char *instance_url;
extern char *oauth_token;

/* shared */
char gen_buf[BUF_SIZE];
char selector[SELECTOR_SIZE];

void nomem_msg(char *file, int line) {
    printf("No more memory (%s:%d)", file, line);
}

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

surl_response *get_surl_for_endpoint(char method, char *endpoint) {
  static char *hdrs[1] = {NULL};
  surl_response *resp;

  if (hdrs[0] == NULL) {
    hdrs[0] = malloc(BUF_SIZE);
    snprintf(hdrs[0], BUF_SIZE, "Authorization: Bearer %s", oauth_token);
  }

  snprintf(gen_buf, BUF_SIZE, "%s%s", instance_url, endpoint);
  resp = surl_start_request(method, gen_buf, hdrs, 1);
  
  return resp;
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
