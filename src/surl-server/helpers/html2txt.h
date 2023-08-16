#ifndef __html2txt_h
#define __html2txt_h

#define HTML2TEXT_EXCLUDE_LINKS 1
#define HTML2TEXT_DISPLAY_LINKS 2

char *html2text(char *html, int strip_level);

#endif
