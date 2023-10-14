#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "malloc0.h"
#include "surl.h"
#include "simple_serial.h"
#include "strsplit.h"
#include "api.h"

#define BUF_SIZE 255

#ifdef __CC65__
  #ifdef SURL_TO_LANGCARD
  #pragma code-name (push, "LC")
  #else
  #pragma code-name (push, "LOWCODE")
  #endif
#endif

account *account_new_from_json(void) {
  account *a = account_new();
  int r;
  char n_lines;
  char *note;
  
  if (surl_get_json(gen_buf, BUF_SIZE, SURL_HTMLSTRIP_NONE, translit_charset,
                    ".id,.username,.acct,.display_name,"
                    ".created_at,.followers_count,"
                    ".following_count") >= 0) {
    n_lines = strnsplit_in_place(gen_buf, '\n', lines, 7);
    if (n_lines > 5) {
      a->id = strdup(lines[0]);
      a->username = strdup(lines[1]);
      a->acct = strdup(lines[2]);
      a->display_name = strdup(lines[3]);
      a->created_at = date_format(lines[4], 0);
      a->followers_count = atol(lines[5]);
      a->following_count = atol(lines[6]);
    }
    if (n_lines < 6)
      goto err_out;

    note = malloc0(2048);
    r = surl_get_json(note, 2048, SURL_HTMLSTRIP_FULL, translit_charset, ".note");
    if (r < 0) {
      free(note);
    } else {
      a->note = realloc(note, r + 1);
    }
  } else {
    goto err_out;
  }

  /* TODO fields */

  return a;
err_out:
  account_free(a);
  return NULL;
}

#ifdef __CC65__
#pragma code-name (pop)
#endif

account *api_get_profile(char *id) {
  account *a;
  char n_lines;

  snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, ACCOUNTS_ENDPOINT"/%s",
              id == NULL ? "verify_credentials" : id);
  get_surl_for_endpoint(SURL_METHOD_GET, endpoint_buf);

  if (!surl_response_ok()) {
    return NULL;
  }

  a = account_new();

  if (surl_get_json(gen_buf, BUF_SIZE, SURL_HTMLSTRIP_NONE, translit_charset, ".id,.display_name,.acct,.username") >= 0) {
    n_lines = strnsplit_in_place(gen_buf,'\n', lines, 4);
    if (n_lines < 4) {
      account_free(a);
      a = NULL;
      return NULL;
    }
    a->id = strdup(lines[0]);
    a->display_name = strdup(lines[1]);
    a->acct = strdup(lines[2]);
    a->username = strdup(lines[3]);
  }
  return a;
}

void account_free(account *a) {
  char i;
  if (a == NULL)
    return;
  free(a->id);
  free(a->username);
  free(a->acct);
  free(a->display_name);
  free(a->created_at);
  free(a->note);
  for (i = 0; i < a->n_fields; i++) {
    free(a->fields[i]);
  }
  free(a->fields);
  free(a);
}
