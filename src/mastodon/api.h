#ifndef __api_h
#define __api_h

#include "status.h"
#include "account.h"

#define SELECTOR_SIZE 255
extern char selector[SELECTOR_SIZE];
#define BUF_SIZE 255
extern char gen_buf[BUF_SIZE];

account *api_get_profile(char *id);

#define ACCOUNTS_ENDPOINT "/api/v1/accounts/"
#define TIMELINE_ENDPOINT "/api/v1/timelines/"
#define STATUS_ENDPOINT   "/api/v1/statuses/"

#define COMPOSE_PUBLIC 0
#define COMPOSE_UNLISTED 1
#define COMPOSE_PRIVATE 2

#define HOME_TIMELINE "home"

int api_get_posts(char *endpoint, char to_load, char *first_to_load, char **post_ids);
int api_get_account_posts(account *a, char to_load, char *first_to_load, char **post_ids);
int api_get_status_and_replies(char to_load, char *root_id, char *root_leaf_id, char *first_to_load, char **post_ids);

status *api_get_status(char *post_id, char full);

void api_favourite_status(status *s);
void api_reblog_status(status *s);
char api_delete_status(status *s);

char api_relationship_get(account *a, char f);
account *api_get_full_account(account *a);
#endif
