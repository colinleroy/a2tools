#ifndef __tgi_compat_h
#define __tgi_compat_h

#ifdef __CC65__
#include <tgi.h>

#else

#define TGI_COLOR_BLACK         0x00
#define TGI_COLOR_GREEN         0x01
#define TGI_COLOR_VIOLET        0x02
#define TGI_COLOR_WHITE         0x03
#define TGI_COLOR_BLACK2        0x04
#define TGI_COLOR_ORANGE        0x05
#define TGI_COLOR_BLUE          0x06
#define TGI_COLOR_WHITE2        0x07

#define TGI_COLOR_MAGENTA       TGI_COLOR_BLACK2
#define TGI_COLOR_DARKBLUE      TGI_COLOR_WHITE2
#define TGI_COLOR_DARKGREEN     0x08
#define TGI_COLOR_GRAY          0x09
#define TGI_COLOR_CYAN          0x0A
#define TGI_COLOR_BROWN         0x0B
#define TGI_COLOR_GRAY2         0x0C
#define TGI_COLOR_PINK          0x0D
#define TGI_COLOR_YELLOW        0x0E
#define TGI_COLOR_AQUA          0x0F

#define tgi_install(x) do {} while(0)

int tgi_init();
int tgi_done();

void tgi_setcolor(int color);
void tgi_setpixel(int x, int y);
void tgi_line(int start_x, int start_y, int end_x, int end_y);

#endif

#endif
