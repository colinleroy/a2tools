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
#define TL_STATUS_MAX_LEN 256

status *status_new(void) {
  status *s = malloc(sizeof(status));
  if (s == NULL) {
    printf("No more memory at %s:%d\n",__FILE__, __LINE__);
    return NULL;
  }
  memset(s, 0, sizeof(status));
  return s;
}

#ifdef __CC65__
#pragma static-locals (push,off) /* need reentrancy */
#endif
status *status_new_from_json(surl_response *resp, char *id, char is_reblog) {
  status *s;
  char *reblog_id;

  s = status_new();
  s->id = strdup(id);
  if (s->id == NULL) {
    printf("No more memory at %s:%d\n",__FILE__, __LINE__);
  }

  s->created_at = malloc(26);
  if (s->created_at == NULL) {
    printf("No more memory at %s:%d\n",__FILE__, __LINE__);
    return NULL;
  }
  s->content = malloc(TL_STATUS_MAX_LEN);
  if (s->content == NULL) {
    printf("No more memory at %s:%d\n",__FILE__, __LINE__);
    return NULL;
  }

  snprintf(selector, SELECTOR_SIZE, ".[]|select(.id==\"%s\").%screated_at", id, is_reblog?"reblog.":"");
  surl_get_json(resp, s->created_at, 26, 0, 0, selector);

  if (!is_reblog) {
    reblog_id = malloc(26);
    if (reblog_id == NULL) {
      printf("No more memory at %s:%d\n",__FILE__, __LINE__);
      return NULL;
    }

    snprintf(selector, SELECTOR_SIZE, ".[]|select(.id==\"%s\").reblog.id", id);
    surl_get_json(resp, reblog_id, BUF_SIZE, 0, 0, selector);
    if (reblog_id[0] != '\0') {
      s->reblog = status_new_from_json(resp, id, 1);
      free(s->reblog->id);
      s->reblog->id = reblog_id;
    } else {
      free(reblog_id);
    }
    reblog_id = NULL;
  }

  if (s->reblog == NULL) {
    snprintf(selector, SELECTOR_SIZE, ".[]|select(.id==\"%s\").%scontent", id, is_reblog?"reblog.":"");
    surl_get_json(resp, s->content, TL_STATUS_MAX_LEN, 1, 1, selector);
    s->content[TL_STATUS_MAX_LEN - 4] = '.';
    s->content[TL_STATUS_MAX_LEN - 3] = '.';
    s->content[TL_STATUS_MAX_LEN - 2] = '.';
  } else {
    free(s->content);
    s->content = NULL;
  }

  s->account = account_new_from_status_json(resp, id, is_reblog);
  return s;
}

#ifdef __CC65__
#pragma static-locals (pop)
#endif

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
