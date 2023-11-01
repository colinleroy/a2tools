#ifndef __progress_bar_h
#define __progress_bar_h

/* Pass x = -1, y = -1 if you don't plan on repositioning cursor
 * between calls, for more performance. */
void progress_bar(int x, int y, int width, unsigned long cur, unsigned long end);

#endif
