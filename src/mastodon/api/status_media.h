#ifndef __status_media_h
#define __status_media_h

typedef struct _status_media status_media;

struct _status_media {
  char n_media;
  char **media_url;
  char **media_alt_text;
};

status_media *status_media_new_from_json(surl_response *resp);
void status_media_free(status_media *s);
status_media *api_get_status_media(char *status_id);

#endif
