#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "surl.h"
#include "simple_serial.h"
#include "extended_conio.h"
#include "extended_string.h"
#include "api.h"
#include "common.h"

#define ENDPOINT_BUF_SIZE 128
char endpoint_buf[ENDPOINT_BUF_SIZE];

char *translit_charset = US_CHARSET;
char arobase = '@';
extern char *instance_url;
extern char *oauth_token;

/* shared */
char gen_buf[BUF_SIZE];
char selector[SELECTOR_SIZE];

surl_response *get_surl_for_endpoint(char *method, char *endpoint) {
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

account *api_get_profile(char *id) {
  surl_response *resp;
  account *a = account_new();
  int r = -1;
  char n_lines, **lines;

  if (a == NULL) {
    return NULL;
  }

  snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, "%s/%s", ACCOUNTS_ENDPOINT,
              id == NULL ? "verify_credentials" : id);
  resp = get_surl_for_endpoint("GET", endpoint_buf);

  if (resp == NULL || resp->code < 200 || resp->code >= 300)
    goto err_out;

  if (surl_get_json(resp, gen_buf, BUF_SIZE, 0, translit_charset, ".id,.display_name,.username") == 0) {
    n_lines = strsplit_in_place(gen_buf,'\n',&lines);
    if (n_lines < 3) {
      account_free(a);
      free(lines);
      return NULL;
    }
    a->id = strdup(lines[0]);
    a->display_name = strdup(lines[1]);
    a->username = strdup(lines[2]);
    free(lines);
  }

err_out:
  surl_response_free(resp);
  return a;
}
