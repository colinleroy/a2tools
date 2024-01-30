#ifndef __progress_bar_h
#define __progress_bar_h

#ifndef __fastcall__
#define __fastcall__
#endif

/* Pass x = -1, y = -1 if you don't plan on repositioning cursor
 * between calls, for more performance. */
void __fastcall__ progress_bar(int x, int y, int width, unsigned long cur, unsigned long end);

#endif
