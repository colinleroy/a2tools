#ifndef __extended_string_h
#define __extended_string_h

#ifndef _CC65__
#define __fastcall__
#endif

int __fastcall__ strsplit(char *in, char split, char ***out);
int __fastcall__ strsplit_in_place(char *in, char split, char ***out);
char *strndup_ellipsis(char *in, int len);
char *ellipsis(char *in, int len);
char * __fastcall__ trim(const char *in);

#endif
