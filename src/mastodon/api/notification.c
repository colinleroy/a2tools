
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "malloc0.h"
#include "surl.h"
#include "simple_serial.h"
#include "strsplit.h"
#include "api.h"

void notification_free(notification *n) {
  if (IS_NULL(n)) {
    return;
  }
  free(n->created_at);
  free(n->excerpt);
  free(n->display_name);
  free(n);
}

int api_get_notifications(char to_load, char notifications_type, char *load_before, char *load_after, char **notification_ids) {
  int n_notifications;

  n_notifications = 0;
  snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, NOTIFICATION_ENDPOINT"?limit=%d" "&types[]=mention", to_load);
  if (notifications_type == NOTIFICATION_FAVOURITE) {
    strcat(endpoint_buf, "&types[]=follow&types[]=favourite&types[]=reblog");
  }
  if (IS_NOT_NULL(load_before)) {
    strcat(endpoint_buf, "&min_id=");
    strcat(endpoint_buf, load_before);
  }
  if (IS_NOT_NULL(load_after)) {
    strcat(endpoint_buf, "&max_id=");
    strcat(endpoint_buf, load_after);
  }

  get_surl_for_endpoint(SURL_METHOD_GET, endpoint_buf);
  
  if (!surl_response_ok())
    return 0;

  if (surl_get_json(gen_buf, ".[]|.id", NULL, SURL_HTMLSTRIP_NONE, BUF_SIZE) >= 0) {
    n_notifications = strnsplit(gen_buf, '\n', notification_ids, to_load);
  }

  return n_notifications;
}

notification *api_get_notification(char *id) {
  notification *n = NULL;
  char c;

  snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, NOTIFICATION_ENDPOINT"/%s", id);
  get_surl_for_endpoint(SURL_METHOD_GET, endpoint_buf);
  
  if (!surl_response_ok())
    return NULL;

  if (surl_get_json(gen_buf,
                    ".id,.type,.created_at,"
                    ".status.id//\"-\","
                    ".account.id,"
                    ".account.display_name,"
                    ".account.username",
                    translit_charset, SURL_HTMLSTRIP_NONE, BUF_SIZE) >= 0) {
    char n_lines;

    n_lines = strnsplit_in_place(gen_buf, '\n', lines, 7);
    n = notification_new();
    if (n_lines < 6) {
err_out:
      notification_free(n);
      return NULL;
    }

    id_copy(n->id, lines[0]);
    c = lines[1][2];
    if (c == 'l' /* foLlow */) {
      n->type = NOTIFICATION_FOLLOW;
    } else if (c == 'v' /* faVourite */) {
      n->type = NOTIFICATION_FAVOURITE;
    } else if (c == 'b' /* reBlog */) {
      n->type = NOTIFICATION_REBLOG;
    } else if (c == 'n' /* meNtion */) {
      n->type = NOTIFICATION_MENTION;
    }
    n->created_at = date_format(lines[2], 1);
    if (lines[3][0] != '-') {
      id_copy(n->status_id, lines[3]);
    }
    id_copy(n->account_id, lines[4]);
    /* if display_name is null or non-existent, 
     * we'll use username */
    n->display_name = (lines[5][0] == '\0' && n_lines == 7) ? strdup(lines[6]) : strdup(lines[5]);
  } else {
      goto err_out;
  }
  if (surl_get_json(gen_buf,
                    n->type != NOTIFICATION_FOLLOW ? ".status.content":".account.note",
                    translit_charset, SURL_HTMLSTRIP_FULL, BUF_SIZE) >= 0) {
    n->excerpt = strdup(gen_buf);
  }

  return n;
}

char *notification_verb[N_NOTIFICATIONS_TYPE] = {
  "mentioned you",
  "faved",
  "shared",
  "followed you"
};
