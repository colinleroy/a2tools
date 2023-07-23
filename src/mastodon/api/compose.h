#ifndef __compose_h
#define __compose_h

#include "common.h"

char api_send_toot(char *buffer, char *in_reply_to_id, char **media_ids, char n_medias, char compose_audience);
char *api_send_hgr_image(char *filename);
#endif
