#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "surl.h"
#include "simple_serial.h"
#include "extended_conio.h"
#include "extended_string.h"
#include "account.h"

#define BUF_SIZE 255

account *account_new(void) {
  account *a = malloc(sizeof(account));
  memset(a, 0, sizeof(account));
  return a;
}

account *account_new_from_status_json(surl_response *resp, char *id, char is_reblog) {
  char *selector;
  account *a;

  a = account_new();
  a->id = malloc(26);
  a->username = malloc(BUF_SIZE);
  a->display_name = malloc(BUF_SIZE);
  
  selector = malloc(BUF_SIZE);
  snprintf(selector, BUF_SIZE, ".[]|select(.id==\"%s\").%saccount.id", id, is_reblog?"reblog.":"");
  surl_get_json(resp, a->id, 26, selector);

  selector = malloc(BUF_SIZE);
  snprintf(selector, BUF_SIZE, ".[]|select(.id==\"%s\").%saccount.username", id, is_reblog?"reblog.":"");
  surl_get_json(resp, a->username, 26, selector);

  selector = malloc(BUF_SIZE);
  snprintf(selector, BUF_SIZE, ".[]|select(.id==\"%s\").%saccount.display_name", id, is_reblog?"reblog.":"");
  surl_get_json(resp, a->display_name, 26, selector);

  free(selector);
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
