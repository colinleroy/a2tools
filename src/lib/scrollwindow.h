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
#define BASL    "$28"
#define BASH    "$29"
#define RD80VID "$C01F"

#define get_wndtop()       \
    (                      \
        asm("lda "WNDTOP), \
        __A__              \
    )

#define get_wndbtm()       \
    (                      \
        asm("lda "WNDBTM), \
        __A__              \
    )

#define get_wndlft()       \
    (                      \
        asm("lda "WNDLFT), \
        __A__              \
    )

#define get_wndwdth()      \
    (                      \
        asm("lda "WNDWDTH),\
        __A__              \
    )

#define set_wndtop(t)      \
    (                      \
        __A__=(t),         \
        asm("ldx "WNDTOP), \
        asm("sta "WNDTOP), \
        asm("txa"),        \
        __A__              \
    )

#define set_wndbtm(b)      \
    (                      \
        __A__=(b),         \
        asm("ldx "WNDBTM), \
        asm("sta "WNDBTM), \
        asm("txa"),        \
        __A__              \
    )

#define set_wndlft(l)      \
    (                      \
        __A__=(l),         \
        asm("ldx "WNDLFT), \
        asm("sta "WNDLFT), \
        asm("txa"),        \
        __A__              \
    )

#define set_wndwdth(w)     \
    (                      \
        __A__=(w),         \
        asm("ldx "WNDWDTH),\
        asm("sta "WNDWDTH),\
        asm("txa"),        \
        __A__              \
    )

#endif
