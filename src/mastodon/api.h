#ifndef __api_h
#define __api_h

#include "status.h"
#include "account.h"

#define SELECTOR_SIZE 128
extern char selector[SELECTOR_SIZE];

int api_get_profile(char **public_name, char **handle);

#define HOME_TIMELINE "home"
int api_get_timeline_posts(char *tlid, char to_load, char *last_to_load, char *first_to_load, char **post_ids);
status *api_get_status(char *post_id);
#endif
