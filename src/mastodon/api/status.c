#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <conio.h>
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

extern char MEDIA_STR[];
extern char IMAGE_STR[];
extern char VIDEO_STR[];
extern char AUDIO_STR[];
extern char GIF_STR[];

/* Not in media.c because we in fact need that to display statuses */
char *media_type_str[N_MEDIA_TYPE] = {
  MEDIA_STR,
  IMAGE_STR,
  VIDEO_STR,
  AUDIO_STR,
  GIF_STR
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

const char *basic_selector = ".reblog|(.account.id,.account.display_name,.account.acct,.account.username,"
                                      ".created_at,.reblog.id//\"-\",.spoiler_text//\"\","
                                      "(.media_attachments|length),"
                                      ".media_attachments[0].type//\"-\","
                                      ".replies_count,.reblogs_count,.favourites_count,"
                                      ".visibility,.reblogged,.favourited,.bookmarked,.poll.id//\"-\","
                                      ".quote.status.id//.quote.quoted_status.id//\"-\""
                                      ")";
#define BASIC_SELECTOR_NLINES 18

const char *content_selector       = ".reblog|(.content)";

static __fastcall__ char status_fill_from_json(status *s, char *id, char full) {
  char c, n_lines;
  int r;
  char *content;
  char is_reblog = 0, reblog_offset = 8 /* strlen(".reblog|") */;

  id_copy(s->id, id);

again:
  r = surl_get_json(gen_buf, basic_selector + reblog_offset,
                    translit_charset, SURL_HTMLSTRIP_NONE,
                    BUF_SIZE);

  n_lines = strnsplit_in_place(gen_buf, '\n', lines, BASIC_SELECTOR_NLINES);
  if (r >= 0 && n_lines == BASIC_SELECTOR_NLINES) {
    if (!is_reblog && lines[5][0] != '-') {
      s->reblogged_by = strdup(lines[1][0] ? lines[1]:lines[2]);
      id_copy(s->reblog_id, s->id);
      id_copy(s->id, lines[5]);
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
    if (lines[16][0] != '-') {
      s->poll = poll_new();
      id_copy(s->poll->id, lines[16]);
      poll_fill(s->poll, is_reblog /* POLL_FROM_REBLOG == 1, POLL_FROM_STATUS == 0 */);
    }

    /* Quote */
    if (lines[17][0] != '-') {
      id_copy(s->quote_id, lines[17]);
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
  free(s->created_at);
  free(s->spoiler_text);
  free(s->content);
  free(s->reblogged_by);
  account_free(s->account);
  poll_free(s->poll);
  status_free(s->quote);
  free(s);
}

static char rec_full;
static char rec_level;

/* Function is recursive */
#pragma static-locals(push, off)
static status *api_get_status_rec(char *status_id) {
  if (--rec_level == 0)
    return NULL;

  snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, STATUS_ENDPOINT"/%s", status_id);
  get_surl_for_endpoint(SURL_METHOD_GET, endpoint_buf);

  if (surl_response_ok()) {
    status *s = status_new();
    if (status_fill_from_json(s, status_id, rec_full) == 0) {
      if (s->quote_id[0] && strcmp(s->quote_id, s->id)) {
        s->quote = api_get_status_rec(s->quote_id);
        if (IS_NOT_NULL(s->quote)) {
          char *re = strstr(s->content, "\nRE: https://");
          if (IS_NOT_NULL(re)) {
            re[0] = '\0';
          }
        }
      }
      return s;
    }
    else {
      status_free(s);
    }
  }
  return NULL;
}
#pragma static-locals(pop)

status *api_get_status(char *status_id, char full) {
  rec_level = 3;
  rec_full  = full;
  return api_get_status_rec(status_id);
}

#ifdef __CC65__
#pragma code-name (pop)
#endif
