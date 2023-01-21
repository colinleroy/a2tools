#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "surl.h"
#include "simple_serial.h"
#include "extended_conio.h"
#include "extended_string.h"
#include "status.h"

#define BUF_SIZE 255

status *status_new(void) {
  status *s = malloc(sizeof(status));
  memset(s, 0, sizeof(status));
  return s;
}

status *status_new_from_json(surl_response *resp, char *id, char is_reblog) {
  char *selector;
  status *s;
  char *reblog_id;

  s = status_new();
  s->id = strdup(id);
  s->created_at = malloc(26);
  s->content = malloc(1024);
  
  selector = malloc(BUF_SIZE);
  snprintf(selector, BUF_SIZE, ".[]|select(.id==\"%s\").%screated_at", id, is_reblog?"reblog.":"");
  surl_get_json(resp, s->created_at, 26, selector);

  if (!is_reblog) {
    reblog_id = malloc(BUF_SIZE);
    snprintf(selector, BUF_SIZE, ".[]|select(.id==\"%s\").reblog.id", id);
    surl_get_json(resp, reblog_id, BUF_SIZE, selector);
    if (reblog_id[0] != '\0') {
      s->reblog = status_new_from_json(resp, reblog_id, 1);
    } else {
      
    }
    free(reblog_id);
    reblog_id = NULL;
  }

  if (s->reblog == NULL) {
    snprintf(selector, BUF_SIZE, ".[]|select(.id==\"%s\").%scontent", id, is_reblog?"reblog.":"");
    surl_get_json(resp, s->content, 1024, selector);
  } else {
    free(s->content);
    s->content = NULL;
  }

  free(selector);

  s->account = account_new_from_status_json(resp, id, is_reblog);
  return s;
}

void status_free(status *s) {
  if (s == NULL)
    return;
  free(s->id);
  free(s->created_at);
  free(s->content);
  status_free(s->reblog);
  account_free(s->account);
  free(s);
}
