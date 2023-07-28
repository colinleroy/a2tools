#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "surl.h"
#include "simple_serial.h"
#include "strsplit.h"
#include "api.h"

#define BUF_SIZE 255

#ifdef __CC65__
#pragma code-name (push, "LOWCODE")
#endif

account *account_new(void) {
  account *a = malloc(sizeof(account));
  if (a == NULL) {
    nomem_msg(__FILE__, __LINE__);
    return NULL;
  }
  memset(a, 0, sizeof(account));
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

account *account_new_from_json(void) {
  account *a = account_new();
  int r;
  char n_lines, **lines;
  char *note;
  
  if (a == NULL) {
    return NULL;
  }

  if (surl_get_json(gen_buf, BUF_SIZE, 0, translit_charset,
                    ".id,.username,.acct,.display_name,"
                    ".created_at,.followers_count,"
                    ".following_count") >= 0) {
    n_lines = strsplit_in_place(gen_buf, '\n', &lines);
    if (n_lines >= 6) {
      a->id = strdup(lines[0]);
      a->username = strdup(lines[1]);
      a->acct = strdup(lines[2]);
      a->display_name = strdup(lines[3]);
      a->created_at = date_format(lines[4], 0);
      a->followers_count = atol(lines[5]);
      a->following_count = atol(lines[6]);
    }
    free(lines);
    if (n_lines < 6)
      goto err_out;

    note = malloc(2048);
    r = surl_get_json(note, 2048, 1, translit_charset, ".note");
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

account *api_get_profile(char *id) {
  surl_response *resp;
  account *a;
  char n_lines, **lines;

  a = NULL;
  snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, "%s/%s", ACCOUNTS_ENDPOINT,
              id == NULL ? "verify_credentials" : id);
  resp = get_surl_for_endpoint(SURL_METHOD_GET, endpoint_buf);

  if (!surl_response_ok(resp)) {
    goto err_out;
  }

  a = account_new();
  if (a == NULL) {
    goto err_out;
  }

  if (surl_get_json(gen_buf, BUF_SIZE, 0, translit_charset, ".id,.display_name,.acct,.username") >= 0) {
    n_lines = strsplit_in_place(gen_buf,'\n',&lines);
    if (n_lines < 4) {
      account_free(a);
      a = NULL;
      free(lines);
      goto err_out;
    }
    a->id = strdup(lines[0]);
    a->display_name = strdup(lines[1]);
    a->acct = strdup(lines[2]);
    a->username = strdup(lines[3]);
    free(lines);
  }

err_out:
  surl_response_free(resp);
  return a;
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
