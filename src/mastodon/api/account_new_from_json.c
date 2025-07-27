#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "malloc0.h"
#include "surl.h"
#include "strsplit.h"
#include "api.h"
#include "atoc.h"
#include "cli.h"

static char *field_selector = ".fields[i]|(.name+\": \"+.value)";
#define FIELD_SELECTOR_NUM 8

#pragma static-locals(push, off)
account *account_new_from_json(void) {
  account *a = NULL;
  static int r;
  static char i, n_lines;
  static char *note;
  
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
      while (i) {
        char len = NUMCOLS - RIGHT_COL_START - 1;
        i--;
        field_selector[FIELD_SELECTOR_NUM] = i+'0';
        a->fields[i] = malloc0(len);
        surl_get_json(a->fields[i], field_selector, translit_charset, SURL_HTMLSTRIP_FULL, len);
      }

      note = malloc0(2048);
      r = surl_get_json(note, ".note", translit_charset, SURL_HTMLSTRIP_FULL, 2048);
      if (r < 0) {
        free(note);
      } else {
        a->note = realloc(note, r + 1);
      }
    }
  }

  return a;
}
#pragma static-locals(pop)
