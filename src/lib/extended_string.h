#ifndef __extended_string_h
#define __extended_string_h

#ifndef _CC65__
#define __fastcall__
#endif

char *strndup_ellipsis(char *in, int len);
char *ellipsis(char *in, int len);
char * __fastcall__ trim(const char *in);

#endif
