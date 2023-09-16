#ifndef __clean_rt_once_h
#define __clean_rt_once_h

#ifdef __CC65__
void __fastcall__ clean_rt_once(void);
#else
#define clean_rt_once()
#endif

#endif
