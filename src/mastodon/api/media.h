#ifndef __media_h
#define __media_h

typedef struct _media media;

struct _media {
  char n_media;
  char *media_id[4];
  char *media_url[4];
  char *media_type[4];
  char *media_alt_text[4];
};

#define media_new() (media *)malloc0(sizeof(media))
void media_free(media *s);

media *api_get_status_media(char *id);
media *api_get_account_media(char *id);

#endif
