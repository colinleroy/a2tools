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

extern char *instance_url;
extern char *oauth_token;

char *translit_charset = US_CHARSET;
char monochrome = 1;
char arobase = '@';
char gen_buf[BUF_SIZE];
char selector[SELECTOR_SIZE];
char endpoint_buf[ENDPOINT_BUF_SIZE];
char *lines[MAX_LINES_NUM];

void id_copy(char *dst, char *src) {
  strncpy(dst, src, SNOWFLAKE_ID_LEN);
}

const surl_response *get_surl_for_endpoint(char method, char *endpoint) {
  static char *hdrs[1] = {NULL};
  static char h_num = 0;

  if (IS_NULL(hdrs[0]) && IS_NOT_NULL(oauth_token)) {
    hdrs[0] = malloc0(ENDPOINT_BUF_SIZE);
    strcpy(hdrs[0], "Authorization: Bearer ");
    strcat(hdrs[0], oauth_token);
    h_num = 1;
  }

  strcpy(gen_buf, instance_url);
  strcat(gen_buf, endpoint);
  return surl_start_request(hdrs, h_num, gen_buf, method);
}
