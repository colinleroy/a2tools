
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "surl.h"
#include "simple_serial.h"
#include "strsplit.h"
#include "api.h"

#ifdef __CC65__
#pragma code-name (push, "LOWCODE")
#endif

notification *notification_new(void) {
  notification *n;
  
  n = malloc(sizeof(notification));
  if (n == NULL) {
    return NULL;
  }
  memset(n, 0, sizeof(notification));
  return n;
}

void notification_free(notification *n) {
  if (n == NULL) {
    return;
  }
  free(n->id);
  free(n->created_at);
  free(n->status_id);
  free(n->account_id);
  free(n->excerpt);
  free(n->display_name);
  free(n);
}

int api_get_notifications(char to_load, char *first_to_load, char **notification_ids) {
  surl_response *resp;
  int n_notifications;

  n_notifications = 0;
  snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, "%s?limit=%d&%s=%s&%s=%s&%s=%s&%s=%s%s%s", NOTIFICATION_ENDPOINT, to_load,
            "types[]", "follow",
            "types[]", "mention",
            "types[]", "favourite",
            "types[]", "reblog",
            first_to_load ? "&max_id=" : "",
            first_to_load ? first_to_load : ""
          );
  resp = get_surl_for_endpoint("GET", endpoint_buf);
  
  if (!surl_response_ok(resp))
    goto err_out;

  if (surl_get_json(resp, gen_buf, 512, 0, NULL, ".[]|.id") == 0) {
    char **tmp;
    char i;
    n_notifications = strsplit(gen_buf, '\n', &tmp);
    for (i = 0; i < n_notifications; i++) {
      notification_ids[i] = tmp[i];
    }
    free(tmp);
  }

err_out:
  surl_response_free(resp);
  return n_notifications;
}

notification *api_get_notification(char *id) {
  surl_response *resp;
  notification *n;

  n = NULL;

  snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, "%s/%s", NOTIFICATION_ENDPOINT, id);
  resp = get_surl_for_endpoint("GET", endpoint_buf);
  
  if (!surl_response_ok(resp))
    goto err_out;

  if (surl_get_json(resp, gen_buf, BUF_SIZE, 0, translit_charset,
                    ".id,.type,.created_at,"
                    ".status.id//\"-\","
                    ".account.id,"
                    ".account.display_name") == 0) {
    char **lines;
    char n_lines;

    n_lines = strsplit_in_place(gen_buf, '\n', &lines);
    if (n_lines < 6) {
      free(lines);
      goto err_out;
    }
    n = notification_new();
    if (n == NULL) {
      goto err_out;
    }

    n->id = strdup(lines[0]);
    if (!strcmp(lines[1], "follow")) {
      n->type = NOTIFICATION_FOLLOW;
    } else if (!strcmp(lines[1], "favourite")) {
      n->type = NOTIFICATION_FAVOURITE;
    } else if (!strcmp(lines[1], "reblog")) {
      n->type = NOTIFICATION_REBLOG;
    } else if (!strcmp(lines[1], "mention")) {
      n->type = NOTIFICATION_MENTION;
    }
    n->created_at = strdup(lines[2]);
    if (!strcmp(lines[3], "-")) {
      n->status_id = NULL;
    } else {
      n->status_id = strdup(lines[3]);
    }
    n->account_id = strdup(lines[4]);
    n->display_name = strdup(lines[5]);
    free(lines);
  } else {
    notification_free(n);
    n = NULL;
    goto err_out;
  }
  if (surl_get_json(resp, gen_buf, BUF_SIZE, 1, translit_charset,
                    n->type != NOTIFICATION_FOLLOW ? ".status.content":".account.note") == 0) {
    n->excerpt = strdup(gen_buf);
  } else {
    n->excerpt = NULL;
  }

err_out:
  surl_response_free(resp);
  return n;
}

char *notification_verb(notification *n) {
  switch(n->type) {
    case NOTIFICATION_FOLLOW:
      return("followed you");
    case NOTIFICATION_FAVOURITE:
      return("liked");
    case NOTIFICATION_REBLOG:
      return("shared");
    case NOTIFICATION_MENTION:
      return("mentioned you");
  }
  return "???";
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
