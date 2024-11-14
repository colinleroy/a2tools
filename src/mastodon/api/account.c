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

extern unsigned char scrw;

static char *field_selector = ".fields[i]|(.name+\": \"+.value)";
#define FIELD_SELECTOR_NUM 8

account *account_new_from_json(void) {
  account *a = NULL;
  int r;
  char i, n_lines;
  char *note;
  
  if (surl_get_json(gen_buf,
                    ".id,.display_name,.acct,.username,"
                    ".created_at,.followers_count,"
                    ".following_count,(.fields|length)",
                    translit_charset, SURL_HTMLSTRIP_NONE, BUF_SIZE) >= 0) {
    n_lines = strnsplit_in_place(gen_buf, '\n', lines, 8);
    if (n_lines == 8) {
      a = account_new_from_lines();

      a->created_at = date_format(lines[4], 0);
      a->followers_count = atol(lines[5]);
      a->following_count = atol(lines[6]);
      i = atoc(lines[7]);
      if (i > MAX_ACCT_FIELDS) {
        i = MAX_ACCT_FIELDS;
      }
      a->fields = malloc0(sizeof(char *)*i);
      a->n_fields = i;
      do {
        char len = scrw - RIGHT_COL_START - 1;
        i--;
        field_selector[FIELD_SELECTOR_NUM] = i+'0';
        a->fields[i] = malloc0(len);
        surl_get_json(a->fields[i], field_selector, translit_charset, SURL_HTMLSTRIP_FULL, len);
      } while (i);

      note = malloc0(2048);
      r = surl_get_json(note, ".note", translit_charset, SURL_HTMLSTRIP_FULL, 2048);
      if (r < 0) {
        free(note);
      } else {
        a->note = realloc(note, r + 1);
      }
    }
  }

  /* TODO fields */

  return a;
}

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
              id == NULL ? "verify_credentials" : id);
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
#ifdef __CC65__
#pragma code-name (pop)
#endif
