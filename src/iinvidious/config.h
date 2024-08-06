#ifndef __config_h
#define __config_h

#define URL_PASSER_FILE "/RAM/VIDURL"
#define PAGE_BEGIN 3
#define PAGE_HEIGHT 17

extern char *translit_charset;
extern char video_size;
extern char enable_subtitles;
extern char tmp_buf[80];

void config(void);
void load_config(void);

#endif
