#ifndef __api_h
#define __api_h

#include "status.h"
#include "account.h"

void api_get_profile(char **public_name, char **handle);

#define HOME_TIMELINE "home"
int api_get_timeline_posts(char *tlid, status ***posts);

#endif
