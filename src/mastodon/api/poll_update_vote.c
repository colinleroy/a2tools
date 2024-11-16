#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "malloc0.h"
#include "surl.h"
#include "strsplit.h"
#include "api.h"

#pragma code-name (push, "LC")

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
  surl_send_data_params((uint32)i, SURL_DATA_APPLICATION_JSON_HELP);
  surl_send_data_chunk(params, i);

  surl_read_response_header();
  if (surl_response_ok()) {
    poll_fill(p, POLL_FROM_VOTE);
  }
}

#pragma code-name(pop)
