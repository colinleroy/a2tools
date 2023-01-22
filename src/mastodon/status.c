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
  s->displayed_at = -1;
  return s;
}

#ifdef __CC65__
#pragma static-locals (push,off) /* need reentrancy */
#endif
status *status_new_from_json(surl_response *resp, char *id, char full, char is_reblog) {
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
  s->content = malloc(full ? 2048 : TL_STATUS_MAX_LEN);
  if (s->content == NULL) {
    printf("No more memory at %s:%d\n",__FILE__, __LINE__);
    return NULL;
  }

  snprintf(selector, SELECTOR_SIZE, ".%screated_at", is_reblog?"reblog.":"");
  surl_get_json(resp, s->created_at, 26, 0, 0, selector);

  if (!is_reblog) {
    reblog_id = malloc(26);
    if (reblog_id == NULL) {
      printf("No more memory at %s:%d\n",__FILE__, __LINE__);
      return NULL;
    }

    snprintf(selector, SELECTOR_SIZE, ".reblog.id");
    surl_get_json(resp, reblog_id, BUF_SIZE, 0, 0, selector);
    if (reblog_id[0] != '\0') {
      s->reblog = status_new_from_json(resp, reblog_id, full, 1);
    }
    free(reblog_id);
    reblog_id = NULL;
  }

  if (s->reblog == NULL) {
    snprintf(selector, SELECTOR_SIZE, ".%scontent", is_reblog?"reblog.":"");
    surl_get_json(resp, s->content, full ? 2048 : TL_STATUS_MAX_LEN, 1, 1, selector);
    if (!full) {
      s->content[TL_STATUS_MAX_LEN - 4] = '.';
      s->content[TL_STATUS_MAX_LEN - 3] = '.';
      s->content[TL_STATUS_MAX_LEN - 2] = '.';
    }
  } else {
    free(s->content);
    s->content = NULL;
  }

  s->account = account_new_from_status_json(resp, is_reblog);
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
