#ifndef __config_h
#define __config_h

extern char *translit_charset;
extern char monochrome;
extern char enable_video;
extern char enable_subtitles;
void config(void);
void load_config(void);

#endif
