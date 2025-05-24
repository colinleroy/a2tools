#ifndef __config_h
#define __config_h

#define RAM_URL_PASSER_FILE "/RAM/VIDURL"
#define URL_PASSER_FILE RAM_URL_PASSER_FILE+5

extern char *translit_charset;
extern char monochrome;
extern char enable_video;
extern char enable_subtitles;
extern char video_size;
void text_config(void);
void config(void);
void load_config(void);

#endif
