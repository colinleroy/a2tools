#ifndef __compose_h
#define __compose_h

#include "common.h"

#define NUM_CHARS 501

signed char api_send_toot(char mode, char *buffer, char *cw, char sensitive_medias,
                          char *ref_toot_id, char **media_ids, char n_medias,
                          char compose_audience);

char *api_send_hgr_image(char *filename, char *description, char **err);

char *compose_get_status_text(char *status_id);

#endif
