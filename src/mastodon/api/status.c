#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "surl.h"
#include "simple_serial.h"
#include "strsplit.h"
#include <stdlib.h>
#include "api.h"

#ifdef __CC65__
#pragma code-name (push, "LOWCODE")
#endif

#define TL_SPOILER_TEXT_BUF 54
#define TL_STATUS_SHORT_BUF 512
#define TL_STATUS_LARGE_BUF 4096

status *status_new(void) {
  status *s = malloc(sizeof(status));
  if (s == NULL) {
    return NULL;
  }
  memset(s, 0, sizeof(status));
  s->displayed_at = -1;
  return s;
}

#ifdef __CC65__
#pragma static-locals (push,off) /* need reentrancy */
#endif
status *status_new_from_json(char *id, char full, char is_reblog) {
  status *s;
  char **lines;
  char n_lines;
  int r;
  char *content;

  s = status_new();
  if (s == NULL) {
    nomem_msg(__FILE__, __LINE__);
    return NULL;
  }
  s->id = strdup(id);
  if (s->id == NULL) {
    free(s);
    nomem_msg(__FILE__, __LINE__);
    return NULL;
  }

  s->account = account_new();

  if (s->account == NULL) {
    status_free(s);
    nomem_msg(__FILE__, __LINE__);
    return NULL;
  }

  /* .reblog.id is the only one that can be null (and hence not there),
   * so put it at the end */
  if (is_reblog) {
    r = surl_get_json(gen_buf, BUF_SIZE, 0, translit_charset,
                      ".reblog.created_at,.reblog.account.display_name,.reblog.reblog.id//\"-\",.reblog.spoiler_text");
  } else {
    r = surl_get_json(gen_buf, BUF_SIZE, 0, translit_charset,
                      ".created_at,.account.display_name,.reblog.id//\"-\",.spoiler_text");
  }

  n_lines = strsplit_in_place(gen_buf, '\n', &lines);
  if (r >= 0 && n_lines >= 3) {
    s->created_at = date_format(lines[0], 1);
    s->account->display_name = strdup(lines[1]);
    if (!is_reblog && lines[2][0] != '-') {
      s->reblog = status_new_from_json(lines[2], full, 1);
    }
    if (n_lines > 3) {
      s->spoiler_text = malloc(TL_SPOILER_TEXT_BUF);
      if (!s->spoiler_text) {
        goto err_mem;
      }
      strncpy(s->spoiler_text, lines[3], TL_SPOILER_TEXT_BUF - 1);
      s->spoiler_text[TL_SPOILER_TEXT_BUF - 1] = '\0';
    }
  }
  free(lines);
  
  if (s->reblog == NULL) {
    /* Get details of original toot */
    if (is_reblog) {
      r = surl_get_json(gen_buf, BUF_SIZE, 0, NULL,
                        "(.reblog.media_attachments|map(. | select(.type==\"image\"))|length),"
                        ".reblog.replies_count,.reblog.reblogs_count,.reblog.favourites_count,.reblog.reblogged,.reblog.favourited,"
                        ".reblog.account.id,.reblog.account.acct,.reblog.account.username");
    } else {
      r = surl_get_json(gen_buf, BUF_SIZE, 0, NULL,
                        "(.media_attachments|map(. | select(.type==\"image\"))|length),"
                        ".replies_count,.reblogs_count,.favourites_count,.reblogged,.favourited,"
                        ".account.id,.account.acct,.account.username");
    }

    n_lines = strsplit_in_place(gen_buf, '\n', &lines);
    if (r >= 0 && n_lines == 9) {
      s->n_images = atoi(lines[0]);
      s->n_replies = atoi(lines[1]);
      s->n_reblogs = atoi(lines[2]);
      s->n_favourites = atoi(lines[3]);
      if (lines[4][0] == 't') /* true */
        s->favorited_or_reblogged |= REBLOGGED;
      if (lines[5][0] == 't') /* true */
        s->favorited_or_reblogged |= FAVOURITED;
      s->account->id = strdup(lines[6]);
      s->account->acct = strdup(lines[7]);
      s->account->username = strdup(lines[8]);
    }
    free(lines);

    content = malloc(full ? TL_STATUS_LARGE_BUF : TL_STATUS_SHORT_BUF);
    if (!content) {
err_mem:
      status_free(s);
      nomem_msg(__FILE__, __LINE__);
      return NULL;
    }

    if (is_reblog) {
      r = surl_get_json(content, full ? TL_STATUS_LARGE_BUF : TL_STATUS_SHORT_BUF, 1, translit_charset, ".reblog.content");
    } else {
      r = surl_get_json(content, full ? TL_STATUS_LARGE_BUF : TL_STATUS_SHORT_BUF, 1, translit_charset, ".content");
    }
    if (!full && r == TL_STATUS_SHORT_BUF - 1) {
      strcpy(content + TL_STATUS_SHORT_BUF - 4, "...");
      s->content = content;
    } else if (r >= 0) {
      s->content = realloc(content, r + 1);
    }
  }

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
  free(s->spoiler_text);
  free(s->content);
  status_free(s->reblog);
  account_free(s->account);
  free(s);
}

status *api_get_status(char *status_id, char full) {
  surl_response *resp;
  status *s;

  s = NULL;

  snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, "%s/%s", STATUS_ENDPOINT, status_id);
  resp = get_surl_for_endpoint(SURL_METHOD_GET, endpoint_buf);
  
  if (resp && surl_response_ok(resp)) {
    s = status_new_from_json(status_id, full, 0);
  }

  surl_response_free(resp);
  return s;
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
