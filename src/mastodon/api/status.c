#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "malloc0.h"
#include "surl.h"
#include "simple_serial.h"
#include "strsplit.h"
#include "api.h"

#ifdef __CC65__
  #ifdef SURL_TO_LANGCARD
  #pragma code-name (push, "LC")
  #else
  #pragma code-name (push, "LOWCODE")
  #endif
#endif

#define TL_STATUS_SHORT_BUF 512
#define TL_STATUS_LARGE_BUF 4096

status *status_new(void) {
  status *s = malloc0(sizeof(status));
  s->displayed_at = -1;
  return s;
}

static __fastcall__ char atoc(const char *str) {
  static int i;
  i = atoi(str);
  if (i > 255) {
    return 255;
  }
  return i;
}

#ifdef __CC65__
  #ifdef SURL_TO_LANGCARD
  #pragma code-name (pop)
  #pragma code-name (push, "LOWCODE")
  #endif
#endif

#ifdef __CC65__
#pragma static-locals (push,off) /* need reentrancy */
#endif
static __fastcall__ char status_fill_from_json(status *s, char *id, char full, char is_reblog) {
  char c, n_lines;
  int r;
  char *content;

  s->id = strdup(id);
  if (s->id == NULL) {
    return -1;
  }

  /* .reblog.id is the only one that can be null (and hence not there),
   * so put it at the end */
  if (is_reblog) {
    r = surl_get_json(gen_buf, BUF_SIZE, SURL_HTMLSTRIP_NONE, translit_charset,
                      ".reblog|(.created_at,.account.display_name,\"-\",.spoiler_text)");
  } else {
    r = surl_get_json(gen_buf, BUF_SIZE, SURL_HTMLSTRIP_NONE, translit_charset,
                      ".created_at,.account.display_name,.reblog.id//\"-\",.spoiler_text");
  }

  n_lines = strnsplit_in_place(gen_buf, '\n', lines, 4);
  if (r >= 0 && n_lines >= 3) {
    if (!is_reblog && lines[2][0] != '-') {
      s->reblogged_by = strdup(lines[1]);
      s->reblog_id = s->id;
      s->id = NULL;
      return status_fill_from_json(s, lines[2], full, 1);
    }

    s->account = account_new();

    s->created_at = date_format(lines[0], 1);
    s->account->display_name = strdup(lines[1]);
    if (n_lines > 3) {
      s->spoiler_text = malloc0(TL_SPOILER_TEXT_BUF);
      strncpy(s->spoiler_text, lines[3], TL_SPOILER_TEXT_BUF - 1);
      s->spoiler_text[TL_SPOILER_TEXT_BUF - 1] = '\0';
    }
  }
  
  /* Get details of original toot */
  if (is_reblog) {
    r = surl_get_json(gen_buf, BUF_SIZE, SURL_HTMLSTRIP_NONE, NULL,
                      ".reblog|((.media_attachments|map(. | select(.type==\"image\"))|length),"
                      ".replies_count,.reblogs_count,.favourites_count,.reblogged,.favourited,"
                      ".bookmarked,.account.id,.account.acct,.account.username,.visibility,"
                      ".poll.id)");
  } else {
    r = surl_get_json(gen_buf, BUF_SIZE, SURL_HTMLSTRIP_NONE, NULL,
                      "(.media_attachments|map(. | select(.type==\"image\"))|length),"
                      ".replies_count,.reblogs_count,.favourites_count,.reblogged,.favourited,"
                      ".bookmarked,.account.id,.account.acct,.account.username,.visibility,"
                      ".poll.id");
  }

  n_lines = strnsplit_in_place(gen_buf, '\n', lines, 12);
  if (r >= 0 && n_lines >= 11) {
    s->n_images = atoc(lines[0]);
    s->n_replies = atoc(lines[1]);
    s->n_reblogs = atoc(lines[2]);
    s->n_favourites = atoc(lines[3]);
    if (lines[4][0] == 't') /* true */
      s->flags |= REBLOGGED;
    if (lines[5][0] == 't') /* true */
      s->flags |= FAVOURITED;
    if (lines[6][0] == 't') /* true */
      s->flags |= BOOKMARKED;
    s->account->id = strdup(lines[7]);
    s->account->acct = strdup(lines[8]);
    s->account->username = strdup(lines[9]);
    c = lines[10][1];
    if (c == 'u') /* pUblic */
      s->visibility = COMPOSE_PUBLIC;
    else if (c == 'n') /* uNlisted */
      s->visibility = COMPOSE_UNLISTED;
    else if (c == 'r') /* pRivate */
      s->visibility = COMPOSE_PRIVATE;
    else
      s->visibility = COMPOSE_MENTION;

    /* Poll */
    if (n_lines == 12) {
      s->poll = poll_new();
      s->poll->id = strdup(lines[11]);
      poll_fill(s->poll, is_reblog);
    }
  }

  r = full ? TL_STATUS_LARGE_BUF : TL_STATUS_SHORT_BUF;
  content = malloc0(r);

  if (is_reblog) {
    r = surl_get_json(content, r, SURL_HTMLSTRIP_FULL, translit_charset, ".reblog.content");
  } else {
    r = surl_get_json(content, r, SURL_HTMLSTRIP_FULL, translit_charset, ".content");
  }
  if (!full && r == TL_STATUS_SHORT_BUF - 1) {
    strcpy(content + TL_STATUS_SHORT_BUF - 4, "...");
    s->content = content;
  } else if (r >= 0) {
    s->content = realloc(content, r + 1);
  }

  return 0;
}

#ifdef __CC65__
#pragma static-locals (pop)
#endif

void status_free(status *s) {
  if (s == NULL)
    return;
  free(s->id);
  free(s->reblog_id);
  free(s->created_at);
  free(s->spoiler_text);
  free(s->content);
  free(s->reblogged_by);
  account_free(s->account);
  poll_free(s->poll);
  free(s);
}

status *api_get_status(char *status_id, char full) {
  snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, STATUS_ENDPOINT"/%s", status_id);
  get_surl_for_endpoint(SURL_METHOD_GET, endpoint_buf);
  
  if (surl_response_ok()) {
    status *s = status_new();
    if (status_fill_from_json(s, status_id, full, 0) == 0)
      return s;
    else
      status_free(s);
  }

  return NULL;
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
