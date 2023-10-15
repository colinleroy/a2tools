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
  if (p == NULL)
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

void poll_fill(poll *p, char from_reblog) {
  int r;
  char n_lines;

  if (from_reblog)
    n_lines = 0;
  else
    n_lines = 7; /* strlen(".reblog") */

  memset(p->own_votes, 0, MAX_POLL_OPTIONS);

  r = surl_get_json(gen_buf, BUF_SIZE, SURL_HTMLSTRIP_NONE,
                    translit_charset, poll_selector + n_lines);

  n_lines = strnsplit_in_place(gen_buf, '\n', lines,
                               NUM_POLL_LINES + (MAX_POLL_OPTIONS * 2));
  /* Check we have at least two options */
  if (r >= 0 && n_lines >= (NUM_POLL_LINES + (2 * 2))) {
    p->multiple = (lines[0][0] == 't');
    p->votes_count = (size_t)atoi(lines[1]);

    for (r = 0; r < MAX_POLL_OPTIONS; r++) {
      p->own_votes[r] = (strchr(lines[2], r + '0') != NULL);
    }

    p->options_count = (n_lines - NUM_POLL_LINES);
    p->options_count /= 2;

    for (r = 0; r < p->options_count; r ++) {
      char i = NUM_POLL_LINES + (r * 2);
      p->options[r].title = strdup(lines[i]);
      p->options[r].votes_count = (size_t)atoi(lines[i + 1]);
    }
  }
}

void poll_update_vote(poll *p) {
  char params[128] = "A|choices\n[";
  char i, empty = 1, *cur;

  cur = params + strlen(params);
  for (i = 0; i < MAX_POLL_OPTIONS; i++) {
    if (p->own_votes[i]) {
      cur += sprintf(cur, "%s\"%d\"", empty ? "":",", i);
      empty = 0;
    }
  }
  strcat(cur, "]\n");

  snprintf(endpoint_buf, ENDPOINT_BUF_SIZE, VOTES_ENDPOINT"/%s/votes", p->id);
  get_surl_for_endpoint(SURL_METHOD_POST, endpoint_buf);

  i = strlen(params);
  surl_send_data_params(i, SURL_DATA_APPLICATION_JSON_HELP);
  surl_send_data(params, i);

  surl_read_response_header();
}
