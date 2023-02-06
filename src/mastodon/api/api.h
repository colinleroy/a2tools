#ifndef __api_h
#define __api_h

#include "status.h"
#include "media.h"
#include "account.h"
#include "notification.h"
#include "common.h"

int api_get_posts(char *endpoint, char to_load, char *load_before, char *load_after, char *filter, char *sel, char **post_ids);
int api_get_account_posts(account *a, char to_load, char *load_before, char *load_after, char **post_ids);
int api_get_status_and_replies(char to_load, char *root_id, char *root_leaf_id, char *load_before, char *load_after, char **post_ids);
int api_search(char to_load, char *search, char *load_before, char *load_after, char **post_ids);

status *api_get_status(char *post_id, char full);

void api_favourite_status(status *s);
void api_reblog_status(status *s);
char api_delete_status(status *s);

char api_relationship_get(account *a, char f);
account *api_get_full_account(char *id);
#endif
