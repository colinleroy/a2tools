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

account *account_new_from_status_json(surl_response *resp, char *id, char is_reblog) {
  account *a;

  a = account_new();
  a->id = malloc(26);
  if (a->id == NULL) {
    printf("No more memory at %s:%d\n",__FILE__, __LINE__);
    return NULL;
  }
  a->username = malloc(26);
  if (a->username == NULL) {
    printf("No more memory at %s:%d\n",__FILE__, __LINE__);
    return NULL;
  }
  a->display_name = malloc(26);
  if (a->display_name == NULL) {
    printf("No more memory at %s:%d\n",__FILE__, __LINE__);
    return NULL;
  }

  snprintf(selector, SELECTOR_SIZE, ".[]|select(.id==\"%s\").%saccount.id", id, is_reblog?"reblog.":"");
  surl_get_json(resp, a->id, 26, 0, 0, selector);

  snprintf(selector, SELECTOR_SIZE, ".[]|select(.id==\"%s\").%saccount.username", id, is_reblog?"reblog.":"");
  surl_get_json(resp, a->username, 26, 0, 0, selector);

  snprintf(selector, SELECTOR_SIZE, ".[]|select(.id==\"%s\").%saccount.display_name", id, is_reblog?"reblog.":"");
  surl_get_json(resp, a->display_name, 26, 1, 1, selector);

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
