
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "malloc0.h"
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

static media *media_new_from_json(char *base_selector, char *description_selector) {
  media *m;
  char **lines, *w;
  char i, j, n_lines;
  int n;

  m = media_new();

  n = surl_get_json(img_buf, IMG_BUF_SIZE, SURL_HTMLSTRIP_NONE, translit_charset, base_selector);

  n_lines = 0;
  lines = NULL;

  if (n >= 0) {
    n_lines = strsplit_in_place(img_buf, '\n', &lines);
    m->n_media = n_lines / 3;
    j = 0;
    for (i = 0; i < n_lines - 1; i += 3) {
      m->media_id[j] = strdup(lines[i]);
      m->media_url[j] = strdup(lines[i + 1]);
      m->media_type[j] = strdup(lines[i + 2]);
      m->media_alt_text[j] = NULL;
      ++j;
    }
  }
  free(lines);

  if (description_selector == NULL) {
    return m;
  }

  for (i = 0; i < m->n_media; i ++) {
    snprintf(gen_buf, BUF_SIZE, description_selector, i);
    if (surl_get_json(img_buf, IMG_BUF_SIZE, SURL_HTMLSTRIP_FULL, translit_charset, gen_buf) > 0) {
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
          img_buf[n-3] =
            img_buf[n-2] =
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
    free(m->media_type[i]);
    free(m->media_alt_text[i]);
  }
  free(m);
}

static media *get_media(char *api_endpoint,
                        char *base_selector,
                        char *description_selector,
                        char *id) {
  media *m;

  m = NULL;

  snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, "%s/%s", api_endpoint, id);
  get_surl_for_endpoint(SURL_METHOD_GET, endpoint_buf);
  
  if (!surl_response_ok())
    return NULL;

  return media_new_from_json(base_selector, description_selector);
}

media *api_get_status_media(char *id) {
  return get_media(STATUS_ENDPOINT, 
                   ".media_attachments|map(. | select(.type==\"image\" or .type==\"video\"))|.[]|(.id,.url,.type)",
                   ".media_attachments|map(. | select(.type==\"image\" or .type==\"video\"))|.[%d]|.description",
                   id);
}

media *api_get_account_media(char *id) {
  return get_media(ACCOUNTS_ENDPOINT, 
                   ".id,.avatar_static,.id,.header_static", NULL, id);
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
