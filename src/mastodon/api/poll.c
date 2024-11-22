#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "malloc0.h"
#include "surl.h"
#include "simple_serial.h"
#include "strsplit.h"
#include "api.h"

void poll_free(poll *p) {
  char i;
  if (IS_NULL(p))
    return;

  free(p->id);
  for (i = 0; i < p->options_count; i++) {
    free(p->options[i].title);
  }
  free(p);
}

/* expired, multiple, votes_count, voters_count */
#define NUM_POLL_LINES 3
static const char *poll_selector = ".reblog.poll|(.multiple,.votes_count,"
                            "(.own_votes|join(\",\")),"
                            "(.options[]|(.title,.votes_count)))";

void poll_fill(poll *p, char source) {
  int r;
  char n_lines;

  if (source == POLL_FROM_REBLOG)
    n_lines = 0;
  else if (source == POLL_FROM_STATUS)
    n_lines = 7; /* strlen(".reblog") */
  else if (source == POLL_FROM_VOTE)
    n_lines = 13; /* strlen(".reblog.poll|") */

  bzero(p->own_votes, MAX_POLL_OPTIONS);

  r = surl_get_json(gen_buf, poll_selector + n_lines,
                    translit_charset,
                    SURL_HTMLSTRIP_NONE,
                    BUF_SIZE);

  n_lines = strnsplit_in_place(gen_buf, '\n', lines,
                               (NUM_POLL_LINES + (MAX_POLL_OPTIONS * 2)));
  /* Check we have at least two options */
  if (r >= 0 && n_lines >= (NUM_POLL_LINES + (2 * 2))) {
    p->multiple = (lines[0][0] == 't');
    p->votes_count = (size_t)atoi(lines[1]);

    for (r = 0; r < MAX_POLL_OPTIONS; r++) {
      p->own_votes[r] = IS_NOT_NULL(strchr(lines[2], r + '0'));
    }

    p->options_count = (n_lines - NUM_POLL_LINES) / 2;

    for (r = 0; r < p->options_count; r ++) {
      char i = NUM_POLL_LINES + (r * 2);
      if (IS_NULL(p->options[r].title)) {
        p->options[r].title = strdup(lines[i]);
      } /* otherwise, it's a reload from votes, and
         * titles won't have changed. */
      p->options[r].votes_count = (size_t)atoi(lines[i + 1]);
    }
  }
}
