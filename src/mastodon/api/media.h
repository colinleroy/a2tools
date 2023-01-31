#ifndef __media_h
#define __media_h

typedef struct _media media;

struct _media {
  char n_media;
  char **media_url;
  char **media_alt_text;
};

void media_free(media *s);
media *api_get_status_media(char *id);
media *api_get_account_media(char *id);

#endif
