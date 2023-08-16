#ifndef __scroll_h
#define __scroll_h

#ifdef __CC65__
void __fastcall__ scrolldown_n (char count);
void __fastcall__ scrollup_n (char count);
void __fastcall__ scrolldown_one (void);
void __fastcall__ scrollup_one (void);
#else
#define scrolldown_n(a) do{} while (0)
#define scrollup_n(a) do{} while (0)
#define scrolldown_one(a) do{} while (0)
#define scrollup_one(a) do{} while (0)
#endif
#endif
