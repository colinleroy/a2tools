#ifndef __clean_rt_once_h
#define __clean_rt_once_h

#ifdef __CC65__
extern char _runtime_once_cleaned;
void __fastcall__ runtime_once_clean(void);
#else
#define _runtime_once_cleaned 0
#define runtime_once_clean()
#endif

#endif
