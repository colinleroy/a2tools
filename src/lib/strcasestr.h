#ifndef __strcasematch_h
#define __strcasematch_h

#ifndef __CC65__
#define __fastcall__
#endif

#include <string.h>

char * __fastcall__ strcasestr(const char *str, const char *substr);
#endif
