#ifndef __scroll_h
#define __scroll_h

#ifdef __CC65__
void __fastcall__ scrolldn (char count);
void __fastcall__ scrollup (char count);
#else
#define scrolldn(a) do{} while (0)
#define scrollup(a) do{} while (0)
#endif
#endif
