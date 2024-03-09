#ifndef __strcasematch_h
#define __strcasematch_h

#ifndef __CC65__
#define __fastcall__
#endif

#include <string.h>

char *strcasestr(register char *str, register char *substr);
#endif
