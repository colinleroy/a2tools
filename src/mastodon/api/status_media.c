#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "surl.h"
#include "simple_serial.h"
#include "strsplit.h"
#include "api.h"

#define IMG_BUF_SIZE 2048
static char img_buf[IMG_BUF_SIZE];

status_media *status_media_new(void) {
  status_media *s;
  
  s = malloc(sizeof(status_media));
  if (s == NULL) {
    return NULL;
  }
  memset(s, 0, sizeof(status_media));
  return s;
}
status_media *status_media_new_from_json(surl_response *resp) {
  status_media *s;
  char **lines;
  char i, r, n_lines;

  s = status_media_new();

  r = surl_get_json(resp, img_buf, IMG_BUF_SIZE, 0, translit_charset, ".media_attachments[]|.url,.description//\"-\"");

  if (r == 0) {
    n_lines = strsplit_in_place(img_buf, '\n', &lines);
    s->n_media = n_lines / 2;
    if (n_lines % 2 != 0) {
      /* read error, probably */
      free(lines);
      status_media_free(s);
      return NULL;
    }
    s->media_url = malloc(s->n_media * sizeof(char *));
    s->media_alt_text = malloc(s->n_media * sizeof(char *));
    for (i = 0; i < n_lines; i += 2) {
      r = i / 2;
      s->media_url[r] = lines[i];
      s->media_alt_text[r] = lines[i+1];
    }
  }
  free(lines);

  return s;
}

void status_media_free(status_media *s) {
  if (s == NULL)
    return;
  free(s);
}

status_media *api_get_status_media(char *status_id) {
  surl_response *resp;
  status_media *s;

  s = NULL;

  snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, "%s/%s", STATUS_ENDPOINT, status_id);
  resp = get_surl_for_endpoint("GET", endpoint_buf);
  
  if (resp == NULL || resp->code < 200 || resp->code >= 300)
    goto err_out;

  s = status_media_new_from_json(resp);

err_out:
  surl_response_free(resp);
  return s;
}
