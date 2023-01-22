#ifndef __scroll_h
#define __scroll_h

#ifdef __CC65__
void __fastcall__ scrolldn (void);
void __fastcall__ scrollup (void);
#else
#define scrolldn() do{} while (0)
#define scrollup() do{} while (0)
#endif
#endif
