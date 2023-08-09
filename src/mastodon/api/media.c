
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

#define IMG_BUF_SIZE 2048
static char img_buf[IMG_BUF_SIZE];

#ifdef USE_HGR2
 #define NUMCOLS 40
#else
 #define NUMCOLS 80
#endif

media *media_new(void) {
  media *m;
  
  m = malloc(sizeof(media));
  if (m == NULL) {
    return NULL;
  }
  memset(m, 0, sizeof(media));
  return m;
}

static media *media_new_from_json(char *ids_selector, char *urls_selector, char *alt_text_selector) {
  media *m;
  char **lines, *w;
  char i, n_lines;
  int n;

  m = media_new();

  n = surl_get_json(img_buf, IMG_BUF_SIZE, 0, translit_charset, ids_selector);

  n_lines = 0;
  lines = NULL;

  if (n >= 0) {
    n_lines = strsplit_in_place(img_buf, '\n', &lines);
    m->n_media = n_lines;
    m->media_id = malloc(m->n_media * sizeof(char *));
    m->media_url = malloc(m->n_media * sizeof(char *));
    m->media_alt_text = malloc(m->n_media * sizeof(char *));
    memset(m->media_url, 0, m->n_media * sizeof(char *));
    memset(m->media_alt_text, 0, m->n_media * sizeof(char *));

    for (i = 0; i < n_lines; i ++) {
      m->media_id[i] = strdup(lines[i]);
    }
  }
  free(lines);

  for (i = 0; i < n_lines; i ++) {
    snprintf(gen_buf, BUF_SIZE, urls_selector, i);
    if (surl_get_json(img_buf, IMG_BUF_SIZE, 0, translit_charset, gen_buf) > 0) {
      m->media_url[i] = strdup(img_buf);
    }

    if (alt_text_selector == NULL) {
      continue;
    }

    snprintf(gen_buf, BUF_SIZE, alt_text_selector, i);
    if (surl_get_json(img_buf, IMG_BUF_SIZE, 1, translit_charset, gen_buf) > 0) {
      w = img_buf;
      n = 0;
      while (*w != '\0') {
        if (*w == '\n') {
          *w = ' ';
        }
        ++w;
        ++n;
        if (n == (16*NUMCOLS) - 2) {
          /* shorten description, we don't scroll them yet */
          img_buf[n-3] = '.';
          img_buf[n-2] = '.';
          img_buf[n-1] = '.';
          img_buf[n] = '\0';
          break;
        }
      }
      m->media_alt_text[i] = strdup(img_buf);
    }
  }

  return m;
}

void media_free(media *m) {
  int i;
  if (m == NULL)
    return;
  for (i = 0; i < m->n_media; i++) {
    free(m->media_id[i]);
    free(m->media_url[i]);
    free(m->media_alt_text[i]);
  }
  free(m->media_id);
  free(m->media_url);
  free(m->media_alt_text);
  free(m);
}

static media *get_media(char *api_endpoint,
                        char *ids_selector,
                        char *urls_selector,
                        char *alt_text_selector,
                        char *id) {
  surl_response *resp;
  media *m;

  m = NULL;

  snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, "%s/%s", api_endpoint, id);
  resp = get_surl_for_endpoint(SURL_METHOD_GET, endpoint_buf);
  
  if (!surl_response_ok(resp))
    goto err_out;

  m = media_new_from_json(ids_selector, urls_selector, alt_text_selector);

err_out:
  surl_response_free(resp);
  return m;
}

media *api_get_status_media(char *id) {
  return get_media(STATUS_ENDPOINT, 
                   ".media_attachments|map(. | select(.type==\"image\"))|.[]|.id",
                   ".media_attachments|map(. | select(.type==\"image\"))|.[%d]|.url",
                   ".media_attachments|map(. | select(.type==\"image\"))|.[%d]|.description",
                   id);
}

media *api_get_account_media(char *id) {
  return get_media(ACCOUNTS_ENDPOINT, 
                   ".avatar_static,.header_static",
                   ".avatar_static,.header_static", NULL, id);
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
