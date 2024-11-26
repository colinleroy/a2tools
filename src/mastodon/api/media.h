#ifndef __media_h
#define __media_h

#define MEDIA_TYPE_MEDIA 0
#define MEDIA_TYPE_IMAGE 1
#define MEDIA_TYPE_VIDEO 2
#define MEDIA_TYPE_AUDIO 3
#define MEDIA_TYPE_GIFV  4
#define N_MEDIA_TYPE     5

extern char *media_type_str[N_MEDIA_TYPE];

typedef struct _media media;

struct _media {
  char n_media;
  char media_id[4][SNOWFLAKE_ID_LEN];
  char *media_url[4];
  char *media_type[4];
  char *media_alt_text[4];
};

#define media_new() (media *)malloc0(sizeof(media))
void media_free(media *s);

media *api_get_status_media(char *id);
media *api_get_account_media(char *id);

#endif
