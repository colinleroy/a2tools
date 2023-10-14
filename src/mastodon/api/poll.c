#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "surl.h"
#include "simple_serial.h"
#include "strsplit.h"
#include "api.h"

poll *poll_new(void) {
  poll *p = malloc(sizeof(poll));
  if (p == NULL) {
    return NULL;
  }
  memset(p, 0, sizeof(poll));
  return p;
}

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
#define NUM_POLL_LINES 2

void poll_fill(poll *p, char from_reblog) {
  int r;
  char n_lines;

  if (from_reblog) {
    r = surl_get_json(gen_buf, BUF_SIZE, SURL_HTMLSTRIP_NONE,
                      translit_charset, ".reblog.poll|(.multiple,.votes_count,"
                                        "(.options[]|(.title,.votes_count)))");
  } else {
    r = surl_get_json(gen_buf, BUF_SIZE, SURL_HTMLSTRIP_NONE,
                      translit_charset, ".poll|(.multiple,.votes_count,"
                                        "(.options[]|(.title,.votes_count)))");
  }

  n_lines = strnsplit_in_place(gen_buf, '\n', lines,
                               NUM_POLL_LINES + (MAX_POLL_OPTIONS * 2));
  /* Check we have at least two options */
  if (r >= 0 && n_lines >= (NUM_POLL_LINES + (2 * 2))) {
    p->multiple = (lines[0][0] == 't');
    p->votes_count = (size_t)atoi(lines[1]);

    p->options_count = (n_lines - NUM_POLL_LINES);
    p->options_count /= 2;

    for (r = 0; r < p->options_count; r ++) {
      char i = NUM_POLL_LINES + (r * 2);
      p->options[r].title = strdup(lines[i]);
      p->options[r].votes_count = (size_t)atoi(lines[i + 1]);
    }
  }
}
