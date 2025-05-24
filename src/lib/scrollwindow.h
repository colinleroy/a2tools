#ifndef __scrollwindow_h
#define __scrollwindow_h

#ifndef __CC65__
#define __fastcall__
#endif
void __fastcall__ get_scrollwindow(unsigned char *top, unsigned char *bottom);
void __fastcall__ set_scrollwindow(unsigned char top, unsigned char bottom);
void __fastcall__ get_hscrollwindow(unsigned char *left, unsigned char *width);
void __fastcall__ set_hscrollwindow(unsigned char left, unsigned char width);

#endif
