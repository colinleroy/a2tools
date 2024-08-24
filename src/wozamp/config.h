#ifndef __config_h
#define __config_h

#ifdef __APPLE2ENH__
#define URL_PASSER_FILE "/RAM/VIDURL"
#else
#define URL_PASSER_FILE "VIDURL"
#endif

extern char *translit_charset;
extern char monochrome;
extern char enable_video;
extern char enable_subtitles;
extern char video_size;
void config(void);
void load_config(void);

#endif
