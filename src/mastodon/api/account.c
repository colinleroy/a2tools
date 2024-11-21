#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "malloc0.h"
#include "surl.h"
#include "simple_serial.h"
#include "strsplit.h"
#include "api.h"
#include "atoc.h"
#include "cli.h"

account *account_new_from_lines(void) {
  account *a = account_new();
  a->id = strdup(lines[0]);
  a->display_name = strdup(lines[1]);
  a->acct = strdup(lines[2]);
  a->username = strdup(lines[3]);

  return a;
}

#ifdef __CC65__
#pragma code-name (push, "LC")
#endif

account *api_get_profile(char *id) {
  char n_lines;

  snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, ACCOUNTS_ENDPOINT"/%s",
              IS_NULL(id) ? "verify_credentials" : id);
  get_surl_for_endpoint(SURL_METHOD_GET, endpoint_buf);

  if (!surl_response_ok()) {
    return NULL;
  }
  if (surl_get_json(gen_buf, ".id,.display_name,.acct,.username", translit_charset, SURL_HTMLSTRIP_NONE, BUF_SIZE) < 0) {
    return NULL;
  }
  n_lines = strnsplit_in_place(gen_buf,'\n', lines, 4);
  if (n_lines < 4) {
    return NULL;
  }
  return account_new_from_lines();
}

void account_free(account *a) {
  char i;
  if (IS_NULL(a))
    return;
  free(a->id);
  free(a->username);
  free(a->acct);
  free(a->display_name);
  free(a->created_at);
  free(a->note);
  i = a->n_fields;
  while (i) {
    free(a->fields[--i]);
  }
  free(a->fields);
  free(a);
}
#ifdef __CC65__
#pragma code-name (pop)
#endif
