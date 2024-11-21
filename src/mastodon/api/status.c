#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "malloc0.h"
#include "atoc.h"
#include "surl.h"
#include "simple_serial.h"
#include "strsplit.h"
#include "api.h"

#ifdef __CC65__
#pragma code-name (push, "LC")
#endif

#define TL_STATUS_SHORT_BUF 512
#define TL_STATUS_LARGE_BUF 4096

/* Not in media.c because we in fact need that to display statuses */
char *media_type_str[N_MEDIA_TYPE] = {
#ifdef __APPLE2ENH__
  "media",
  "image",
  "video",
  "audio",
  "gif"
#else
  "img",
  "img",
  "vid",
  "snd",
  "gif"
#endif
};

static status *status_new(void) {
  status *s = malloc0(sizeof(status));
  s->displayed_at = -1;
  return s;
}

#ifdef __CC65__
#pragma code-name (pop)
#pragma code-name (push, "LOWCODE")
#endif

const char *basic_selector   = ".reblog|(.account.id,.account.display_name,.account.acct,.account.username,"
                                      ".created_at,.reblog.id//\"-\",.spoiler_text//\"\","
                                      "(.media_attachments|length),"
                                      ".media_attachments[0].type//\"-\","
                                      ".replies_count,.reblogs_count,.favourites_count,"
                                      ".visibility,.reblogged,.favourited,.bookmarked,.poll.id"
                                      ")";
const char *content_selector = ".reblog|(.content)";

#pragma register-vars(push, on)
static __fastcall__ char status_fill_from_json(register status *s, char *id, char full) {
  char c, n_lines;
  int r;
  char *content;
  char is_reblog = 0, reblog_offset = 8 /* strlen(".reblog|") */;

  s->id = strdup(id);
  if (IS_NULL(s->id)) {
    return -1;
  }

again:
  r = surl_get_json(gen_buf, basic_selector + reblog_offset,
                    translit_charset, SURL_HTMLSTRIP_NONE,
                    BUF_SIZE);

  n_lines = strnsplit_in_place(gen_buf, '\n', lines, 17);
  if (r >= 0 && n_lines >= 16) {
    if (!is_reblog && lines[5][0] != '-') {
      s->reblogged_by = strdup(lines[1]);
      s->reblog_id = s->id;
      s->id = strdup(lines[5]);
      is_reblog = 1;
      reblog_offset = 0;
      goto again;
    }

    s->created_at = date_format(lines[4], 1);
    if (lines[6][0] != '\0') {
      s->spoiler_text = malloc0(TL_SPOILER_TEXT_BUF);
      strncpy(s->spoiler_text, lines[6], TL_SPOILER_TEXT_BUF - 1);
      s->spoiler_text[TL_SPOILER_TEXT_BUF - 1] = '\0';
    }

    s->n_medias = atoc(lines[7]);
    c = lines[8][0];
    if (c == 'g') /* gif */
      s->media_type = MEDIA_TYPE_GIFV;
    else if (c == 'v') /* video */
      s->media_type = MEDIA_TYPE_VIDEO;
    else if (c == 'a') /* audio */
      s->media_type = MEDIA_TYPE_AUDIO;
    else if (c == 'i')
      s->media_type = MEDIA_TYPE_IMAGE;

    s->n_replies = atoc(lines[9]);
    s->n_reblogs = atoc(lines[10]);
    s->n_favourites = atoc(lines[11]);

    s->account = account_new_from_lines();

    c = lines[12][1];
    if (c == 'u') /* pUblic */
      s->visibility = COMPOSE_PUBLIC;
    else if (c == 'n') /* uNlisted */
      s->visibility = COMPOSE_UNLISTED;
    else if (c == 'r') /* pRivate */
      s->visibility = COMPOSE_PRIVATE;
    else
      s->visibility = COMPOSE_MENTION;

    if (lines[13][0] == 't') /* true */
      s->flags |= REBLOGGED;
    if (lines[14][0] == 't') /* true */
      s->flags |= FAVOURITED;
    if (lines[15][0] == 't') /* true */
      s->flags |= BOOKMARKED;

    /* Poll */
    if (n_lines == 17) {
      s->poll = poll_new();
      s->poll->id = strdup(lines[16]);
      poll_fill(s->poll, is_reblog /* POLL_FROM_REBLOG == 1, POLL_FROM_STATUS == 0 */);
    }
  } else {
    return -1;
  }

  r = full ? TL_STATUS_LARGE_BUF : TL_STATUS_SHORT_BUF;
  content = malloc0(r);

  r = surl_get_json(content, content_selector + reblog_offset,
                    translit_charset, SURL_HTMLSTRIP_FULL,
                    r);

  if (!full && r == TL_STATUS_SHORT_BUF - 1) {
    strcpy(content + TL_STATUS_SHORT_BUF - 4, "...");
    s->content = content;
  } else if (r >= 0) {
    s->content = realloc(content, r + 1);
  }

  return 0;
}

void status_free(register status *s) {
  if (IS_NULL(s))
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
#pragma register-vars(pop)

status *api_get_status(char *status_id, char full) {
  snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, STATUS_ENDPOINT"/%s", status_id);
  get_surl_for_endpoint(SURL_METHOD_GET, endpoint_buf);
  
  if (surl_response_ok()) {
    status *s = status_new();
    if (status_fill_from_json(s, status_id, full) == 0)
      return s;
    else
      status_free(s);
  }

  return NULL;
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
