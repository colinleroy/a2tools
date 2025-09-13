#ifndef __compose_h
#define __compose_h

#include "account.h"
#include "common.h"
#include "poll.h"

extern unsigned int NUM_CHARS;

#define NUM_POLL_DURATIONS 6
extern const char compose_poll_durations_hours[];
extern const char *compose_poll_durations_seconds[];

void compose_set_num_chars(void);

char *compose_audience_str(char compose_audience);
char *quote_policy_str(char quote_policy);
signed char api_send_toot(char mode, char *buffer, char *cw, char sensitive_medias,
                          char *ref_toot_id, char **media_ids, char n_medias,
                          poll *toot_poll, char compose_audience,
                          char quote_policy, char **err);

char *api_send_hgr_image(char *filename, char *description, char **err, char x, char y, char w);

char *compose_get_status_text(char *status_id);
void compose_sanitize_str(char *s);

#endif
