#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "surl.h"
#include "simple_serial.h"
#include "extended_conio.h"
#include "extended_string.h"
#include "api.h"

#define TL_STATUS_SHORT_BUF 256
#define TL_STATUS_LARGE_BUF 4096

status *status_new(void) {
  status *s = malloc(sizeof(status));
  if (s == NULL) {
    printf("No more memory at %s:%d\n",__FILE__, __LINE__);
    return NULL;
  }
  memset(s, 0, sizeof(status));
  s->displayed_at = -1;
  return s;
}

#ifdef __CC65__
#pragma static-locals (push,off) /* need reentrancy */
#endif
status *status_new_from_json(surl_response *resp, char *id, char full, char is_reblog) {
  status *s;
  char **lines;
  char r, n_lines;

  s = status_new();
  s->id = strdup(id);
  if (s->id == NULL) {
    printf("No more memory at %s:%d\n",__FILE__, __LINE__);
  }

  s->content = malloc(full ? TL_STATUS_LARGE_BUF : TL_STATUS_SHORT_BUF);
  s->account = account_new();

  if (s->content == NULL || s->account == NULL) {
    printf("No more memory at %s:%d\n",__FILE__, __LINE__);
    return NULL;
  }

  /* .reblog.id is the only one that can be null (and hence not there),
   * so put it at the end */
  if (is_reblog) {
    r = surl_get_json(resp, gen_buf, BUF_SIZE, 0, 1, ".reblog.created_at,.reblog.account.display_name,.reblog.reblog.id");
  } else {
    r = surl_get_json(resp, gen_buf, BUF_SIZE, 0, 1, ".created_at,.account.display_name,.reblog.id");
  }

  n_lines = strsplit_in_place(gen_buf, '\n', &lines);
  if (r == 0 && n_lines >= 2) {
    s->created_at = strdup(lines[0]);
    s->account->display_name = strdup(lines[1]);
    if (!is_reblog && n_lines > 2)
      s->reblog = status_new_from_json(resp, lines[2], full, 1);
  }
  free(lines);
  
  if (s->reblog == NULL) {
    /* Get details of original toot */
    if (is_reblog) {
      r = surl_get_json(resp, gen_buf, BUF_SIZE, 0, 0, "(.reblog.media_attachments|map(. | select(.type==\"image\"))|length),"
                                        ".reblog.replies_count,.reblog.reblogs_count,.reblog.favourites_count,.reblog.reblogged,.reblog.favourited,"
                                        ".reblog.account.id,.reblog.account.username");
    } else {
      r = surl_get_json(resp, gen_buf, BUF_SIZE, 0, 0, "(.media_attachments|map(. | select(.type==\"image\"))|length),"
                                        ".replies_count,.reblogs_count,.favourites_count,.reblogged,.favourited,"
                                        ".account.id,.account.username");
    }

    n_lines = strsplit_in_place(gen_buf, '\n', &lines);
    if (r == 0 && n_lines == 8) {
      s->n_images = atoi(lines[0]);
      s->n_replies = atoi(lines[1]);
      s->n_reblogs = atoi(lines[2]);
      s->n_favourites = atoi(lines[3]);
      if (!strcmp(lines[4], "true")) 
        s->favorited_or_reblogged |= REBLOGGED;
      if (!strcmp(lines[5], "true")) 
        s->favorited_or_reblogged |= FAVOURITED;
      s->account->id = strdup(lines[6]);
      s->account->username = strdup(lines[7]);
    }
    free(lines);

    if (is_reblog) {
      r = surl_get_json(resp, s->content, full ? TL_STATUS_LARGE_BUF : TL_STATUS_SHORT_BUF, 1, 1, ".reblog.content");
    } else {
      r = surl_get_json(resp, s->content, full ? TL_STATUS_LARGE_BUF : TL_STATUS_SHORT_BUF, 1, 1, ".content");
    }
    if (!full && strlen(s->content) == TL_STATUS_SHORT_BUF - 1) {
      s->content[TL_STATUS_SHORT_BUF - 4] = '.';
      s->content[TL_STATUS_SHORT_BUF - 3] = '.';
      s->content[TL_STATUS_SHORT_BUF - 2] = '.';
      s->content[TL_STATUS_SHORT_BUF - 1] = '\0';
    } else {
      s->content = realloc(s->content, strlen(s->content) + 1);
    }
  } else {
    free(s->content);
    s->content = NULL;
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
  free(s->content);
  status_free(s->reblog);
  account_free(s->account);
  free(s);
}
