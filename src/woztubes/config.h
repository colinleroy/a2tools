#ifndef __config_h
#define __config_h

#define URL_PASSER_FILE "/RAM/VIDURL"
#define PAGE_BEGIN 3
#define PAGE_HEIGHT 17

extern char *translit_charset;
extern char video_size;
extern char enable_subtitles;

#define TMP_BUF_SIZE 128
extern char tmp_buf[TMP_BUF_SIZE];
extern char sub_language[3];

void config(void);
void load_config(void);

#endif
