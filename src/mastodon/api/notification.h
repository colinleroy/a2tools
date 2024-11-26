#ifndef __notification_h
#define __notification_h

typedef struct _notification notification;

#define NOTIFICATION_MENTION   0
#define NOTIFICATION_FAVOURITE 1
#define NOTIFICATION_REBLOG    2
#define NOTIFICATION_FOLLOW    3
#define N_NOTIFICATIONS_TYPE   4

struct _notification {
  signed char displayed_at;
  char id[SNOWFLAKE_ID_LEN];
  char type;
  char *created_at;
  char status_id[SNOWFLAKE_ID_LEN];
  char account_id[SNOWFLAKE_ID_LEN];
  char *display_name;
  char *excerpt; /* either status or account note */
};

int api_get_notifications(char to_load, char notifications_type, char *load_before, char *load_after, char **notification_ids);
notification *api_get_notification(char *id);

#define notification_new() (notification *)malloc0(sizeof(notification))
void notification_free(notification *n);

extern char *notification_verb[N_NOTIFICATIONS_TYPE];
#endif
