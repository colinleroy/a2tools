#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "surl.h"
#include "simple_serial.h"
#include "strsplit.h"
#include "compose.h"
#include "common.h"

static char *compose_audience_str(char compose_audience) {
  switch(compose_audience) {
    case COMPOSE_PUBLIC:   return "public";
    case COMPOSE_UNLISTED: return "unlisted";
    case COMPOSE_PRIVATE:  return "private";
    case COMPOSE_MENTION:
    default:               return "direct";
  }
}

char api_send_toot(char *buffer, char *in_reply_to_id, char compose_audience) {
  surl_response *resp;
  char *body = malloc(1024);
  char r;
  int i, o, len;
  char *in_reply_to_buf;

  r = -1;

  if (in_reply_to_id) {
    in_reply_to_buf = malloc(48);
    snprintf(in_reply_to_buf, 48, "in_reply_to_id\n%s\n", in_reply_to_id);
  } else {
    in_reply_to_buf = NULL;
  }

  snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, "%s", STATUS_ENDPOINT);
  resp = get_surl_for_endpoint("POST", endpoint_buf);

  /* Start of status */
  snprintf(body, 1024, "%s"
                       "visibility\n%s\n"
                       "status|TRANSLIT|%s\n",
                        in_reply_to_buf ? in_reply_to_buf : "",
                        compose_audience_str(compose_audience),
                        translit_charset);
  free(in_reply_to_buf);
  /* Escaped buffer */
  len = strlen(buffer);
  o = strlen(body);
  for (i = 0; i < len; i++) {
    if (buffer[i] != '\n') {
      body[o++] = buffer[i];
    } else {
      body[o++] = '\\';
      body[o++] = 'r';
      body[o++] = '\\';
      body[o++] = 'n';
    }
  }
  /* End of status */
  body[o++] = '\n';
  len = o - 1;

  surl_send_data_params(resp, len, 0);
  surl_send_data(resp, body, len);

  surl_read_response_header(resp);

  if (resp == NULL || resp->code < 200 || resp->code >= 300)
    goto err_out;

  r = 0;
err_out:
  surl_response_free(resp);
  return r;
}