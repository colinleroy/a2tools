#ifndef __scrollwindow_h
#define __scrollwindow_h

#ifndef __CC65__
#define __fastcall__
#endif
void __fastcall__ get_scrollwindow(unsigned char *top, unsigned char *bottom);
void __fastcall__ set_scrollwindow(unsigned char top, unsigned char bottom);
void __fastcall__ get_hscrollwindow(unsigned char *left, unsigned char *width);
void __fastcall__ set_hscrollwindow(unsigned char left, unsigned char width);

#define WNDLFT  "$20"
#define WNDWDTH "$21"
#define WNDTOP  "$22"
#define WNDBTM  "$23"
#define CH      "$24"
#define CV      "$25"
#define OURCH   "$057B"
#define OURCV   "$05FB"
#define BASL    "$28"
#define BASH    "$29"
#define RD80VID "$C01F"

#endif
