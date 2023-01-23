#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "surl.h"
#include "simple_serial.h"
#include "extended_conio.h"
#include "extended_string.h"
#include "api.h"

#define BUF_SIZE 255

account *account_new(void) {
  account *a = malloc(sizeof(account));
  if (a == NULL) {
    printf("No more memory at %s:%d\n",__FILE__, __LINE__);
    return NULL;
  }
  memset(a, 0, sizeof(account));
  return a;
}

static char gen_buf[BUF_SIZE];
account *account_new_from_status_json(surl_response *resp, char is_reblog) {
  account *a;
  char **lines;
  int n_lines;

  a = account_new();
  if (a == NULL) {
    return NULL;
  }

  snprintf(selector, SELECTOR_SIZE, ".%saccount.id,.%saccount.username,.%saccount.display_name",
                                    is_reblog?"reblog.":"",
                                    is_reblog?"reblog.":"",
                                    is_reblog?"reblog.":"");
  surl_get_json(resp, gen_buf, BUF_SIZE, 0, 1, selector);
  n_lines = strsplit(gen_buf, '\n', &lines);

  if (n_lines < 3) {
    while (n_lines > 0) {
      free(lines[--n_lines]);
    }
    free(lines);
    free(a);
    return NULL;
  }

  a->id = lines[0];
  a->username = lines[1];
  a->display_name = lines[2];
  free(lines);
  return a;
}

void account_free(account *a) {
  if (a == NULL)
    return;
  free(a->id);
  free(a->username);
  free(a->display_name);
  free(a);
}
