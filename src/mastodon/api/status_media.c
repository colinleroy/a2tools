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
  char **lines, *w;
  char i, r, n_lines;
  int n;

  s = status_media_new();

  r = surl_get_json(resp, img_buf, IMG_BUF_SIZE, 0, translit_charset, ".media_attachments|map(. | select(.type==\"image\"))|.[]|.url");

  if (r == 0) {
    n_lines = strsplit_in_place(img_buf, '\n', &lines);
    s->n_media = n_lines;
    s->media_url = malloc(s->n_media * sizeof(char *));
    s->media_alt_text = malloc(s->n_media * sizeof(char *));
    for (i = 0; i < n_lines; i ++) {
      s->media_url[i] = strdup(lines[i]);
    }
  }
  free(lines);
  for (i = 0; i < n_lines; i ++) {
    snprintf(gen_buf, BUF_SIZE, ".media_attachments|map(. | select(.type==\"image\"))|.[%d]|.description", i);
    r = surl_get_json(resp, img_buf, IMG_BUF_SIZE, 0, translit_charset, gen_buf);
    w = img_buf;
    n = 0;
    while (*w != '\0') {
      if (*w == '\n') {
        *w = ' ';
      }
      ++w;
      ++n;
      if (n == (80*4) - 2) {
        /* shorten description, we don't scroll them yet */
        img_buf[n-3] = '.';
        img_buf[n-2] = '.';
        img_buf[n-1] = '.';
        img_buf[n] = '\0';
        break;
      }
    }
    s->media_alt_text[i] = strdup(img_buf);
  }

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
