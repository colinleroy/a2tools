
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "surl.h"
#include "simple_serial.h"
#include "strsplit.h"
#include "api.h"

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

int api_get_notifications(char to_load, char notifications_type, char *load_before, char *load_after, char **notification_ids) {
  int n_notifications;

  n_notifications = 0;
  snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, NOTIFICATION_ENDPOINT"?limit=%d" "&types[]=mention%s" "&max_id=%s&min_id=%s",
            to_load,
            notifications_type == NOTIFICATION_FAVOURITE ? 
              "&types[]=follow&types[]=favourite&types[]=reblog" : "",
            load_after ? load_after : "",
            load_before ? load_before : ""
          );
  get_surl_for_endpoint(SURL_METHOD_GET, endpoint_buf);
  
  if (!surl_response_ok())
    return 0;

  if (surl_get_json(gen_buf, 512, SURL_HTMLSTRIP_NONE, NULL, ".[]|.id") >= 0) {
    n_notifications = strnsplit(gen_buf, '\n', notification_ids, to_load);
  }

  return n_notifications;
}

notification *api_get_notification(char *id) {
  notification *n = NULL;

  snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, NOTIFICATION_ENDPOINT"/%s", id);
  get_surl_for_endpoint(SURL_METHOD_GET, endpoint_buf);
  
  if (!surl_response_ok())
    return NULL;

  if (surl_get_json(gen_buf, BUF_SIZE, SURL_HTMLSTRIP_NONE, translit_charset,
                    ".id,.type,.created_at,"
                    ".status.id//\"-\","
                    ".account.id,"
                    ".account.display_name,"
                    ".account.username") >= 0) {
    char n_lines;

    n_lines = strnsplit_in_place(gen_buf, '\n', lines, 7);
    n = notification_new();
    if (n == NULL || n_lines < 6) {
      notification_free(n);
      return NULL;
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
    n->created_at = date_format(lines[2], 1);
    if (lines[3][0] == '-') {
      n->status_id = NULL;
    } else {
      n->status_id = strdup(lines[3]);
    }
    n->account_id = strdup(lines[4]);
    /* if display_name is null or non-existent, 
     * we'll use username */
    n->display_name = (lines[5][0] == '\0' && n_lines == 7) ? strdup(lines[6]) : strdup(lines[5]);
  } else {
      notification_free(n);
      return NULL;
  }
  if (surl_get_json(gen_buf, BUF_SIZE, SURL_HTMLSTRIP_FULL, translit_charset,
                    n->type != NOTIFICATION_FOLLOW ? ".status.content":".account.note") >= 0) {
    n->excerpt = strdup(gen_buf);
  } else {
    n->excerpt = NULL;
  }

  return n;
}

char *notification_verb(notification *n) {
  switch(n->type) {
    case NOTIFICATION_FOLLOW:
      return("followed you");
    case NOTIFICATION_FAVOURITE:
      return("faved");
    case NOTIFICATION_REBLOG:
      return("shared");
    case NOTIFICATION_MENTION:
      return("mentioned you");
  }
  return "???";
}
