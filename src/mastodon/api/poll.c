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
    free(p->options[i]->title);
    free(p->options[i]);
  }
  free(p->options);
  free(p);
}

/* expired, multiple, votes_count, voters_count */
#define NUM_POLL_LINES 4

void poll_fill(poll *p, const char *poll_sel) {
  int r;
  char n_lines;

  snprintf(selector, SELECTOR_SIZE, "%s|(.expired,.multiple,.votes_count,.voters_count,"
                                        "(.options[]|(.title,.votes_count)))",
                                      poll_sel);

  r = surl_get_json(gen_buf, BUF_SIZE, SURL_HTMLSTRIP_NONE,
                    translit_charset, selector);

  n_lines = strnsplit_in_place(gen_buf, '\n', lines,
                               NUM_POLL_LINES + (MAX_POLL_OPTIONS * 2));
  /* Check we have at least two options */
  if (r >= 0 && n_lines >= (NUM_POLL_LINES + (2 * 2))) {
    p->expired  = (lines[0][0] == 't');
    p->multiple = (lines[1][0] == 't');
    p->votes_count = (size_t)atoi(lines[2]);
    p->voters_count = (size_t)atoi(lines[3]);

    p->options_count = (n_lines - NUM_POLL_LINES) / 2;
    p->options = malloc(p->options_count * sizeof(poll_option *));
    if (p->options == NULL) {
      p->options_count = 0;
      return;
    }

    for (r = 0; r < p->options_count; r ++) {
      p->options[r] = malloc(sizeof(poll_option));
      p->options[r]->title = strdup(lines[NUM_POLL_LINES + (r * 2)]);
      p->options[r]->votes_count = (size_t)atoi(lines[NUM_POLL_LINES + 1 + (r * 2)]);
    }
  }
}
